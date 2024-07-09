#!/bin/sh
#**********************************************************************
# Copyright (C) 2024 - The Geeqie Team
#
# Author: Omari Stephens
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
## @brief Isolates the test from the rest of the environment.  The goal is to
##        make the test more reliable, and to avoid interrupting other processes
##        that might be running on the host.  Passes all args through and passes
##        the return code back.
##
## $1 Test executable
##
##

set -e

TEST_HOME=$(mktemp -d)

if [ -z "$TEST_HOME" ]; then
    echo "Failed to create temporary home directory." >&2
    exit 1
fi

if [ "$TEST_HOME" = "$HOME" ]; then
    # This both breaks isolation, and makes automatic cleanup extremely dangerous.
    echo "Temporary homedir ($TEST_HOME) is the same as the actual homedir ($HOME)" >&2
    exit 1
fi

# Automatically clean up the temporary home directory on exit.
teardown() {
    # echo "Cleaning up temporary homedir $TEST_HOME" >&2
    rm -rf "$TEST_HOME"
}
trap teardown EXIT

export HOME="$TEST_HOME"
export XDG_CONFIG_HOME="${HOME}/.config"

# Change to temporary homedir and ensure that XDG_CONFIG_HOME exists.
cd
mkdir -p "$XDG_CONFIG_HOME"

# This will automatically pass the command name and args in the expected order.
# And `set -e` (above) means that we'll automatically exit with the same return
# code as our sub-command.
env -i G_DEBUG="fatal-warnings" HOME="$HOME" XDG_CONFIG_HOME="$XDG_CONFIG_HOME" "$@"
