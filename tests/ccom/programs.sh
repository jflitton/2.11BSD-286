#!/bin/sh
# Test the system's own C compiler (usr/src/lib/ccom, with the 286's
# second pass) on the system's programs: compile them with it, as
# tools/cc -ccom does, make a root file system of them, and have
# tests/programs boot that and run its checks, which are of what the
# programs write, to the byte.
#
# The programs are made in obj/nbin, and the C library and the others
# they use in obj/nlib, in the model each is of: all of it the
# compiler's work but the assembly, which is the system's assembler's.
# (make CCOM=1, in the library's directory and the programs'.)
# Whatever the compiler cannot compile is then made in obj/nbin by the
# cross compiler, with its own libraries, so that the system is whole:
# obj/nbin/native lists the programs that are the system's compiler's
# own work, and obj/nbin/cross the others, which there are to be none
# of.
#
# The kernel is the one NATIVE and NATIVE_KERNEL choose
# (tests/harness/system.sh); tests/programs is run as NATIVE_PROGRAMS=1
# has it, with the root made here, in obj/tests/programs-np, or -n.
# NATIVE_PROGRAMS=1 tests/programs/run.sh does the same but that it does
# not go on past a program the system's compiler cannot make.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/ccom/programs.sh needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
NATIVE_PROGRAMS=1
. "$top/tests/harness/system.sh"

buildbase
mk -C "$top/usr/src/lib/libc/i286"

# Each program that the system's compiler can make; make goes on past
# those it cannot.  What the cross compiler made the last time is tried
# again: nothing else puts the cross compiler's work in obj/nbin.
names() { ls "$bin" | grep -v -e '^log$' -e '^native$' -e '^cross$'; }
mkdir -p "$bin/log"
if [ -f "$bin/cross" ]; then
    (cd "$bin" && rm -f $(cat cross))
fi
: > "$bin/cross"
make -s -k -C "$top/usr/src/i286" CCOM=1 all > "$bin/log/ccom" 2>&1 || true
names > "$bin/native"
# And the rest: a program that could not be made is not there.
rest=
for p in $(make -s -C "$top/usr/src/i286" list); do
    [ -f "$bin/$p" ] || rest="$rest $bin/$p"
done
if [ -n "$rest" ]; then
    mk -C "$top/usr/src/i286" O="$bin" $rest
fi
names | comm -13 "$bin/native" - > "$bin/cross"
echo "ccom: $(wc -l < "$bin/native" | tr -d ' ') programs compiled by" \
    "the system's compiler, $(wc -l < "$bin/cross" | tr -d ' ') by the" \
    "cross compiler"

# The root file system is made where it always is, obj/root: the
# Makefile makes it again when the programs are from somewhere else.
# CCOM=1 has it take those of obj/nbin as the system's compiler made
# them, and make again none that the cross compiler did.
mk -C "$top/usr/src/i286" CCOM=1 root KERNEL="$kernel" STAND="$stand"
NATIVE_KERNEL=$NATIVE_KERNEL NATIVE_PROGRAMS=1 ROOTFS=$top/obj/root/root.fs \
    KERNEL=$kernel "$top/tests/programs/run.sh"
