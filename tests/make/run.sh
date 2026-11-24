#!/bin/sh
# Check that the system's Makefiles make the root they are to make:
# that what upstream's Makefiles and etc's distribution install where
# the system is built (make -C usr/src root, tools/build/build.sh) is,
# name for name, the root that `list` beside this says, as NetBSD's
# checkflist checks its sets: each file, directory, device and link,
# with its mode, owner and group, a device's numbers, what a link
# names and which names are one file.  Not the contents, which the
# tests that boot the system look at.  No machine is booted.
#
#	sh tests/make/run.sh
#
# TOOLS=cross checks the cross compiler's build, which is to install
# the same.  When the root is to have something it had not, or not
# have what it had, the list is made again:
#
#	sh tests/make/run.sh -u
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
start=$(date +%s)
buildsys
python3 -I "$top/tools/fsck.py" -l "$top/obj/root/root.fs" |
    python3 -I "$top/tests/make/check.py" "$top/tests/make/list" \
        $(($(date +%s) - start)) "$@"
