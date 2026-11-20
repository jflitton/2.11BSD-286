#!/bin/sh
# Test the system's own C compiler (usr/src/lib/ccom, with the 286's
# second pass) on the system's programs: compile them with it, as
# tools/cc -ccom does, make a root file system of them, and have
# tests/programs boot that and run its checks, which are of what the
# programs write, to the byte.
#
# The programs are made in obj/nbin, and the libraries they use besides
# the C library with them, in the small model and the medium as each
# is.  Whatever the compiler cannot compile is then made there by the
# cross compiler, so that the system is whole: obj/nbin/native lists
# the programs that are the system's compiler's own work, and
# obj/nbin/cross the others, of which there are now none.  The C
# library is the cross compiler's.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/ccom/programs.sh needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
bin=$top/obj/nbin

quiet() { grep -v '^cc \|^as \|^wlib \|Warning!\|Note!\|^unix:' || true; }
make -s -C "$top/usr/src/lib/ccom/i286" host
make -s -C "$top/usr/src/sys/PC" | quiet
make -s -C "$top/usr/src/sys/i286stand" | quiet
make -s -C "$top/usr/src/lib/libc/i286" | quiet

# Each program that the system's compiler can make; make goes on past
# those it cannot.  What the cross compiler made the last time is tried
# again.
names() { ls "$bin" | grep -v -e '^log$' -e '^native$' -e '^cross$'; }
mkdir -p "$bin/log"
if [ -f "$bin/cross" ]; then
    (cd "$bin" && rm -f $(cat cross))
elif [ -n "$(names)" ]; then
    echo "tests/ccom/programs.sh: $bin has no list of whose programs" \
        "they are: remove it" >&2
    exit 1
fi
: > "$bin/cross"
make -s -k -C "$top/usr/src/i286" O="$bin" CC="$top/tools/cc -ccom" all \
    > "$bin/log/ccom" 2>&1 || true
names > "$bin/native"
# And the rest.
make -s -C "$top/usr/src/i286" O="$bin" all | quiet
names | comm -13 "$bin/native" - > "$bin/cross"
echo "ccom: $(wc -l < "$bin/native" | tr -d ' ') programs compiled by" \
    "the system's compiler, $(wc -l < "$bin/cross" | tr -d ' ') by the" \
    "cross compiler"

# The root file system is made where it always is, obj/root: the
# Makefile makes it again when the programs are from somewhere else.
make -s -C "$top/usr/src/i286" O="$bin" root | quiet
ROOTFS=$top/obj/root/root.fs "$top/tests/programs/run.sh"
