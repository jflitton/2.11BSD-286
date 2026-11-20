#!/bin/sh
# Test the system's own C compiler (usr/src/lib/ccom, and for the 286
# usr/src/lib/ccom/i286): compile the tests here with it, as built for
# this machine (tools/ncc), and run what it generated in an emulator
# (tests/harness/runtest.py).
#
# Each t_NAME.c has a function t_NAME that works values out and gives
# each, with what it ought to be, to check or lcheck in support.c, which
# the cross compiler compiles, as it does the printing.  A check is
# known by the line it is on.  They run in real mode, with nothing
# under them.
#
# Then init.c, a program that uses the C library and the system, is
# compiled with it, linked with the library, and run by the kernel as
# /etc/init on a RAM disk, as tests/libc runs its own.
#
# usage: run.sh [NAME ...]	those tests alone; without, all of them
#				and then init.c
#
# With REF=1 the tests are compiled by the cross compiler instead, which
# says whether a test is itself right.
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/ccom
out=$top/obj/tests/ccom
crt=$top/usr/src/lib/libc/i286/crt
mkdir -p "$out/src" "$out/boot"
make -s -C "$top/usr/src/lib/ccom/i286" host

all=
if [ $# -eq 0 ]; then
    all=yes
    set -- $(cd "$here" && ls t_*.c | sed 's/^t_//; s/\.c$//')
fi
srcs=
{
    for t; do echo "extern int t_$t();"; done
    echo "struct test { char *name; int (*fn)(); } tests[] = {"
    for t; do echo "	{ \"$t\", t_$t },"; done
    echo "	{ 0, 0 }"
    echo "};"
} > "$out/src/tests.c"
for t; do
    if [ -n "$REF" ]; then
        cp "$here/t_$t.c" "$out/src/t_$t.c"
        srcs="$srcs $out/src/t_$t.c"
    else
        "$top/tools/ncc" -S -o "$out/src/t_$t.s" "$here/t_$t.c"
        srcs="$srcs $out/src/t_$t.s"
    fi
done
python3 -I "$top/tests/harness/runtest.py" ccom \
    "$top/tests/harness/printf.c" "$here/support.c" "$out/src/tests.c" \
    $srcs "$crt/i4m.s" "$crt/i4d.s"

if [ -z "$all" ] || [ -n "$REF" ]; then
    exit 0
fi
if [ "$EMU" = qemu ]; then
    echo "tests/ccom: init.c needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
make -s -C "$top/usr/src/sys/PC" | grep -v '^cc \|^as ' || true
make -s -C "$top/usr/src/lib/libc/i286" MODEL=s | grep -v '^cc \|^as \|^wlib ' || true
cd "$out/boot"
"$top/tools/ncc" -o init.o "$here/init.c"
"$top/tools/cc" -o init init.o
cp "$here/proto" proto
python3 -I "$top/tools/mkfs.py" -s 400 proto root.fs
python3 -I "$top/tests/harness/runtest.py" --until halting \
    --boot ccom-init "$top/obj/PC/unix" root.fs
