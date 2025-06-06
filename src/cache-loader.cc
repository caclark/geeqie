/*
 * Copyright (C) 2004 John Ellis
 * Copyright (C) 2008 - 2016 The Geeqie Team
 *
 * Author: John Ellis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "cache-loader.h"

#include <ctime>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib-object.h>

#include "cache.h"
#include "filedata.h"
#include "image-load.h"
#include "metadata.h"
#include "options.h"
#include "similar.h"
#include "typedefs.h"
#include "ui-fileops.h"


static gboolean cache_loader_phase2_process(gpointer data);

static void cache_loader_phase1_done_cb(ImageLoader *, gpointer data)
{
	auto cl = static_cast<CacheLoader *>(data);

	cl->idle_id = g_idle_add(cache_loader_phase2_process, cl);
}

static void cache_loader_phase1_error_cb(ImageLoader *, gpointer data)
{
	auto cl = static_cast<CacheLoader *>(data);

	cl->error = TRUE;
	cl->idle_id = g_idle_add(cache_loader_phase2_process, cl);
}

static gboolean cache_loader_phase1_process(gpointer data)
{
	auto *cl = static_cast<CacheLoader *>(data);

	if (cl->todo_mask & CACHE_LOADER_SIMILARITY && !cl->cd->similarity)
		{
		if (!cl->il && !cl->error)
			{
			cl->il = image_loader_new(cl->fd);
			g_signal_connect(G_OBJECT(cl->il), "error", (GCallback)cache_loader_phase1_error_cb, cl);
			g_signal_connect(G_OBJECT(cl->il), "done", (GCallback)cache_loader_phase1_done_cb, cl);
			if (image_loader_start(cl->il))
				{
				return G_SOURCE_REMOVE;
				}

			cl->error = TRUE;
			}
		}

	cl->idle_id = g_idle_add(cache_loader_phase2_process, cl);

	return G_SOURCE_REMOVE;
}

static gboolean cache_loader_phase2_process(gpointer data)
{
	auto *cl = static_cast<CacheLoader *>(data);

	if (cl->todo_mask & CACHE_LOADER_SIMILARITY && !cl->cd->similarity && cl->il)
		{
		GdkPixbuf *pixbuf;
		pixbuf = image_loader_get_pixbuf(cl->il);
		if (pixbuf)
			{
			if (!cl->error)
				{
				ImageSimilarityData *sim;

				sim = image_sim_new_from_pixbuf(pixbuf);
				cache_sim_data_set_similarity(cl->cd, sim);
				image_sim_free(sim);

				cl->todo_mask = static_cast<CacheDataType>(cl->todo_mask & ~CACHE_LOADER_SIMILARITY);
				cl->done_mask = static_cast<CacheDataType>(cl->done_mask | CACHE_LOADER_SIMILARITY);
				}

			/* we have the dimensions via pixbuf */
			if (!cl->cd->dimensions)
				{
				cache_sim_data_set_dimensions(cl->cd, gdk_pixbuf_get_width(pixbuf),
								      gdk_pixbuf_get_height(pixbuf));
				if (cl->todo_mask & CACHE_LOADER_DIMENSIONS)
					{
					cl->todo_mask = static_cast<CacheDataType>(cl->todo_mask & ~CACHE_LOADER_DIMENSIONS);
					cl->done_mask = static_cast<CacheDataType>(cl->done_mask | CACHE_LOADER_DIMENSIONS);
					}
				}
			}

		image_loader_free(cl->il);
		cl->il = nullptr;

		cl->todo_mask = static_cast<CacheDataType>(cl->todo_mask & ~CACHE_LOADER_SIMILARITY);
		}
	else if (cl->todo_mask & CACHE_LOADER_DIMENSIONS &&
		 !cl->cd->dimensions)
		{
		if (!cl->error &&
		    image_load_dimensions(cl->fd, &cl->cd->width, &cl->cd->height))
			{
			cl->cd->dimensions = TRUE;
			cl->done_mask = static_cast<CacheDataType>(cl->done_mask | CACHE_LOADER_DIMENSIONS);
			}
		else
			{
			cl->error = TRUE;
			}

		cl->todo_mask = static_cast<CacheDataType>(cl->todo_mask & ~CACHE_LOADER_DIMENSIONS);
		}
	else if (cl->todo_mask & CACHE_LOADER_MD5SUM &&
		 !cl->cd->have_md5sum)
		{
		if (md5_get_digest_from_file_utf8(cl->fd->path, cl->cd->md5sum))
			{
			cl->cd->have_md5sum = TRUE;
			cl->done_mask = static_cast<CacheDataType>(cl->done_mask | CACHE_LOADER_MD5SUM);
			}
		else
			{
			cl->error = TRUE;
			}

		cl->todo_mask = static_cast<CacheDataType>(cl->todo_mask & ~CACHE_LOADER_MD5SUM);
		}
	else if (cl->todo_mask & CACHE_LOADER_DATE &&
		 !cl->cd->have_date)
		{
		static const auto get_date = [](FileData *fd) -> time_t
		{
			g_autofree gchar *text = metadata_read_string(fd, "Exif.Image.DateTime", METADATA_FORMATTED);
			if (!text) return -1;

			std::tm t{};
			if (!strptime(text, "%Y:%m:%d %H:%M:%S", &t)) return -1;

			t.tm_isdst = -1;
			return mktime(&t);
		};

		cl->cd->date = get_date(cl->fd);
		cl->cd->have_date = TRUE;

		cl->done_mask = static_cast<CacheDataType>(cl->done_mask | CACHE_LOADER_DATE);
		cl->todo_mask = static_cast<CacheDataType>(cl->todo_mask & ~CACHE_LOADER_DATE);
		}
	else
		{
		/* done, save then call done function */
		if (options->thumbnails.enable_caching &&
		    cl->done_mask != CACHE_LOADER_NONE)
			{
			g_autofree gchar *base = cache_create_location(CACHE_TYPE_SIM, cl->fd->path);
			if (base)
				{
				g_free(cl->cd->path);
				cl->cd->path = cache_get_location(CACHE_TYPE_SIM, cl->fd->path);
				if (cache_sim_data_save(cl->cd))
					{
					filetime_set(cl->cd->path, filetime(cl->fd->path));
					}
				}
			}

		cl->idle_id = 0;

		if (cl->done_func)
			{
			cl->done_func(cl, cl->error, cl->done_data);
			}

		return G_SOURCE_REMOVE;
		}

	return G_SOURCE_CONTINUE;
}

CacheLoader *cache_loader_new(FileData *fd, CacheDataType load_mask,
			      CacheLoader::DoneFunc done_func, gpointer done_data)
{
	CacheLoader *cl;

	if (!fd || !isfile(fd->path)) return nullptr;

	cl = g_new0(CacheLoader, 1);
	cl->fd = file_data_ref(fd);

	cl->done_func = done_func;
	cl->done_data = done_data;

	g_autofree gchar *found = cache_find_location(CACHE_TYPE_SIM, cl->fd->path);
	if (found && filetime(found) == filetime(cl->fd->path))
		{
		cl->cd = cache_sim_data_load(found);
		}

	if (!cl->cd) cl->cd = cache_sim_data_new();

	cl->todo_mask = load_mask;
	cl->done_mask = CACHE_LOADER_NONE;

	cl->il = nullptr;
	cl->idle_id = g_idle_add(cache_loader_phase1_process, cl);

	cl->error = FALSE;

	return cl;
}

void cache_loader_free(CacheLoader *cl)
{
	if (!cl) return;

	if (cl->idle_id)
		{
		g_source_remove_by_user_data(cl);
		cl->idle_id = 0;
		}

	image_loader_free(cl->il);
	cache_sim_data_free(cl->cd);

	file_data_unref(cl->fd);
	g_free(cl);
}
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
