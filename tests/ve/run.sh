#!/bin/sh
# Check ve, this port's editor (usr/src/local/ve), on the machine the
# system is built on.  It is built there from the same sources, with
# that machine's C compiler (cc, or HOSTCC) and the system's own termcap
# library, and with tests/ve/host in place of what it has from the 286:
# its segments are from malloc, and it has no terminal.  Each check in
# tests/ve/checks types keys at it and looks at the file it wrote, or
# at the screen it drew (check.py).  What is checked is the editor's
# own work, which is nearly all of it; that it runs on the system, with
# a terminal and segments from the kernel, is tests/programs's to say.
#
# usage: run.sh [NAME ...]	those checks alone
# Run from the top of the repository; needs no emulator.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/ve
src=$top/usr/src/local/ve
lib=$top/usr/src/usr.lib/libtermlib
out=$top/obj/tests/ve
mkdir -p "$out"
cc=${HOSTCC:-cc}

# The termcap library is old C: no prototypes, int where nothing is
# said, a return with no value, and in termcap.c two functions declared
# before they are said to be static, which the word is taken out for.
# __far means nothing here.
old="-std=gnu89 -g -w -Wno-return-type -Wno-error=return-mismatch"
$cc $old -Dstatic= -c -o "$out/termcap.o" "$lib/termcap.c"
$cc $old -D__far= -I"$here/host" -I"$src" -o "$out/ve" \
    "$src/main.c" "$src/term.c" "$src/screen.c" "$src/text.c" \
    "$src/edit.c" "$src/normal.c" "$src/insert.c" "$src/search.c" \
    "$src/ex.c" "$here/host/shim.c" \
    "$out/termcap.o" "$lib/tgoto.c" "$lib/tputs.c"
python3 -I "$here/check.py" "$out/ve" "$here/checks" "$@"
