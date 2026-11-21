#!/bin/sh
# Check that the host tools that write a structure of the system's in a
# file, for the root file system and the disks, tools/mkfs.py and the
# rest, lay it out as the system's own compiler does, and as the cross
# compiler does if it is there (tools/layouts.py tools).
# Run from the top of the repository after `. tools/env.sh`; needs no
# emulator.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
make -s -C "$top/usr/src/lib/ccom/i286" host
exec python3 -I "$top/tools/layouts.py" tools
