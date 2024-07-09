#!/bin/sh
#**********************************************************************
# Copyright (C) 2023 - The Geeqie Team
#
# Author: Colin Clark
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#**********************************************************************

## @file
## @brief Run image tests
##
## $1 Geeqie executable
## $2 Full path to image
##
##

geeqie_exe="$1"
test_image="$2"

xvfb-run --auto-servernum "$geeqie_exe" "$test_image" &

xvfb_pid="$!"

if [ -z "$XDG_CONFIG_HOME" ]
then
    config_home="${HOME}/.config"
else
    config_home="$XDG_CONFIG_HOME"
fi
command_fifo="${config_home}/geeqie/.command"

# Wait for remote to initialize
while [ ! -e "$command_fifo" ] ;
do
    sleep 1
done

# We make sure Geeqie can stay alive for 2 seconds after initialization.
sleep 2

# Check if Geeqie crashed (which would cause xvfb-run to terminate)
if ! ps "$xvfb_pid" >/dev/null 2>&1
then
    echo "geeqie crashed during initialization"
    exit 1
fi

result=$(xvfb-run --auto-servernum "$geeqie_exe" --remote --get-file-info)

# Check if Geeqie crashed (which would cause xvfb-run to terminate)
if ! ps "$xvfb_pid" >/dev/null 2>&1
then
    echo "geeqie crashed during remote command procesing"
    exit 1
fi

xvfb-run --auto-servernum "$geeqie_exe" --remote --quit &

# Timeout is not required. The Meson test timeout will eventually kill the process.
wait "$xvfb_pid"
exit_status=$?

if [ "$exit_status" -eq 0 ]
then
    if echo "$result" | grep -q "Class: Unknown"
    then
        exit 1
    else
        exit 0
    fi
fi

echo "geeqie crashed during shutdown"
exit "$exit_status"
