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
# under them, and as one program: a name that is not static is one
# test's alone.  They are run twice, as the compiler's two passes leave
# them and as the object code improver does (tools/ncc -O).  t_nd.c is
# compiled with -nd=ND, so that its DS is not SS, as the kernel's
# network's is not, and is entered by way of nd.s, which sets its DS.
#
# ssnear.c is not run: it is compiled with -nd=ND for what the compiler
# says of a pointer into SS made near, on the lines marked so.
#
# Then prog.c, a program that uses the C library and the system, is
# compiled with it for the small model, the medium and the large, assembled by
# the system's assembler and linked by its linker (tools/cc -ccom) with
# the library as those made it (obj/nlib); and the kernel is booted
# from a RAM disk on which /etc/init is init.c, which runs the three,
# and the tests above as one program of the large model, support.c
# compiled with them by the same compiler and printing with the C
# library's printf, since there is no harness under them there.  It
# runs the compiler there as well, as the compiler compiled it: the
# two passes on prog.c as the preprocessor left it, and the object code
# improver on the assembly for prog.c.  What each writes on the system
# is to be what it wrote here.
#
# usage: run.sh [NAME ...]	those tests alone; without, all of them
#				and then init.c
#
# With REF=1 the tests are compiled by the cross compiler instead, which
# says whether a test is itself right.  init.c is booted with the
# kernel that TOOLS and TOOLS_KERNEL choose (tests/harness/system.sh),
# in obj/tests/ccom-init-n or -nk if it is the system's own tools'.
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/ccom
out=$top/obj/tests/ccom
crt=$top/usr/src/lib/libc/i286/crt
mkdir -p "$out/src" "$out/boot"
make -s -C "$top/usr/src/lib/ccom/i286" host

# What the compiler says where SS is not DS of a pointer into SS made
# near, which is not run: each line of ssnear.c that ends with NEAR is
# to draw it once, and no other line.
(cd "$here" && "$top/tools/ncc" -S -nd=ND -o "$out/src/ssnear.s" ssnear.c) \
    2> "$out/src/ssnear.log"
got=$(sed -n 's/^ssnear.c:\([0-9]*\): warning: pointer into SS made near.*/\1/p' \
    "$out/src/ssnear.log" | tr '\n' ' ')
want=$(grep -n 'NEAR \*/$' "$here/ssnear.c" | cut -d: -f1 | tr '\n' ' ')
if [ "$got" != "$want" ]; then
    echo "ccom: ssnear.c: pointers into SS made near on lines $got," \
        "not $want" >&2
    exit 1
fi

all=
if [ $# -eq 0 ]; then
    all=yes
    set -- $(cd "$here" && ls t_*.c | sed 's/^t_//; s/\.c$//')
fi
{
    for t; do echo "extern int t_$t();"; done
    echo "struct test { char *name; int (*fn)(); } tests[] = {"
    for t; do echo "	{ \"$t\", t_$t },"; done
    echo "	{ 0, 0 }"
    echo "};"
} > "$out/src/tests.c"
for o in "" -O; do
    srcs=
    for t; do
        if [ -n "$REF" ]; then
            cp "$here"/*.h "$out/src"
            cp "$here/t_$t.c" "$out/src/t_$t.c"
            srcs="$srcs $out/src/t_$t.c"
        elif [ $t = nd ]; then
            # (Its pointers into SS made near are on purpose: ssnear.c
            # is for what the compiler says of them.)
            "$top/tools/ncc" -S $o -nd=ND -DND -I"$here" \
                -o "$out/src/t_$t.s" "$here/t_$t.c" 2> "$out/src/t_nd.log" ||
                { cat "$out/src/t_nd.log" >&2; exit 1; }
            grep -v 'pointer into SS made near' "$out/src/t_nd.log" >&2 ||
                true
            srcs="$srcs $out/src/t_$t.s $here/nd.s"
        else
            "$top/tools/ncc" -S $o -o "$out/src/t_$t.s" "$here/t_$t.c"
            srcs="$srcs $out/src/t_$t.s"
        fi
    done
    python3 -I "$top/tests/harness/runtest.py" ccom$o \
        "$top/tests/harness/printf.c" "$here/support.c" "$out/src/tests.c" \
        $srcs "$crt/i4m.s" "$crt/i4d.s" "$crt/chp.s"
    [ -z "$REF" ] || break
done

if [ -z "$all" ] || [ -n "$REF" ]; then
    exit 0
fi
if [ "$EMU" = qemu ]; then
    echo "tests/ccom: init.c needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
. "$top/tests/harness/system.sh"
if [ "$TOOLS_KERNEL" = native ]; then
    mk -C "$top/usr/src/sys/PC" TOOLS=native
else
    mk -C "$top/usr/src/sys/PC" TOOLS=cross
    case=
fi
mk -C "$top/usr/src/lib/libc/i286" TOOLS=native
cd "$out/boot"
"$top/tools/cc" -ccom -o init "$here/init.c"
"$top/tools/cc" -ccom -DMEDIUM=0 -o small "$here/prog.c"
"$top/tools/cc" -ccom -mm -DMEDIUM=1 -o medium "$here/prog.c"
"$top/tools/cc" -ccom -ml -DMEDIUM=1 -DLARGE=1 -o large "$here/prog.c"
"$top/tools/cc" -ccom -ml -Dputc=putchar -I"$here" -o tlarge \
    "$here/support.c" "$out/src/tests.c" $(for t; do echo "$here/t_$t.c"; done)
c2=$top/usr/src/lib/c2/i286
"$top/tools/cc" -ccom -o c2 "$c2/c20.c" "$c2/c21.c"
# The compiler's two passes, compiled by themselves: the first is all
# upstream's here, its own outcode and error with it.
c0=$top/usr/src/lib/ccom
"$top/tools/cc" -ccom -Di286 -I"$c0" -o c0 "$c0/c00.c" "$c0/c01.c" \
    "$c0/c02.c" "$c0/c03.c" "$c0/c04.c" "$c0/c05.c" "$c0/ansi.c"
"$top/tools/cc" -ccom -Di286 -I"$c0/i286" -o c1 "$c0/i286/c10.c" \
    "$c0/i286/c11.c" "$c0/i286/c12.c" "$c0/i286/c13.c" "$c0/i286/c14.c" \
    "$c0/i286/c15.c"
"$top/tools/ncc" -E -DMEDIUM=0 -o prog.i "$here/prog.c"
"$top/tools/ncc" -S -DMEDIUM=0 -o prog.s "$here/prog.c"
"$top/obj/host/ccom/c2" prog.s prog.want
cp "$here/proto" proto
python3 -I "$top/tools/mkfs.py" -s 800 proto root.fs
python3 -I "$top/tests/harness/runtest.py" --until halting \
    --boot ccom-init$case "$kernel" root.fs
