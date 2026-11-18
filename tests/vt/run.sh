#!/bin/sh
# Check the console's terminal emulator, usr/src/sys/i286at/vt.c, on the
# machine the system is built on: it touches no hardware, so it builds
# with that machine's C compiler (cc, or HOSTCC) and runs there.  Run
# from the top of the repository; needs no emulator.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
out=$top/obj/tests/vt
mkdir -p "$out"
cc=${HOSTCC:-cc}

# vt.c is the kernel's C: old-style definitions, and int where nothing
# is said.
$cc -std=gnu89 -g -Wno-implicit-int -Wno-deprecated-non-prototype \
    -Wno-parentheses -Wno-return-type -I"$top/tests/vt" \
    -c -o "$out/vt.o" "$top/usr/src/sys/i286at/vt.c"
$cc -std=c99 -g -Wall -Wno-deprecated-non-prototype \
    -I"$top/tests/vt" -I"$top/usr/src/sys/i286at" \
    -c -o "$out/vtcheck.o" "$top/tests/vt/vtcheck.c"
$cc -o "$out/vtcheck" "$out/vt.o" "$out/vtcheck.o"
"$out/vtcheck"
