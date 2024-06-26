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

#ifndef UI_TREE_EDIT_H
#define UI_TREE_EDIT_H

#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>

struct TreeEditData
{
	GtkWidget *window;
	GtkWidget *entry;

	gchar *old_name;
	gchar *new_name;

	gint (*edit_func)(TreeEditData *ted, const gchar *oldname, const gchar *newname, gpointer data);
	gpointer edit_data;

	GtkTreeView *tree;
	GtkTreePath *path;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
};

gboolean tree_edit_by_path(GtkTreeView *tree, GtkTreePath *tpath, gint column, const gchar *text,
		           gboolean (*edit_func)(TreeEditData *, const gchar *, const gchar *, gpointer), gpointer data);

gint tree_view_row_get_visibility(GtkTreeView *widget, GtkTreeIter *iter, gboolean fully_visible);

gint tree_view_row_make_visible(GtkTreeView *widget, GtkTreeIter *iter, gboolean center);

gboolean tree_view_move_cursor_away(GtkTreeView *widget, GtkTreeIter *iter, gboolean only_selected);

void shift_color(GdkRGBA *src, gshort val, gint direction);

/**
 * @def STYLE_SHIFT_STANDARD
 * The standard shift percent for alternating list row colors
 */
#define STYLE_SHIFT_STANDARD 10

gint widget_auto_scroll_start(GtkWidget *widget, GtkAdjustment *v_adj, gint scroll_speed, gint region_size,
			      gint (*notify_func)(GtkWidget *widget, gint x, gint y, gpointer data), gpointer notify_data);
void widget_auto_scroll_stop(GtkWidget *widget);


/*
 * Various g_list utils, do not really fit anywhere, so they are here.
 */
GList *uig_list_insert_link(GList *list, GList *link, gpointer data);
GList *uig_list_insert_list(GList *parent, GList *insert_link, GList *list);


#endif
/* vim: set shiftwidth=8 softtabstop=0 cindent cinoptions={1s: */
