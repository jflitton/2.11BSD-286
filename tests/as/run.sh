#!/bin/sh
# Test the system's assembler (usr/src/bin/as/i286) against NASM, whose
# syntax it reads: each is given the same files, and what the one makes
# of a file is to be what the other makes of it (objcmp.py), the text
# and the data to the byte and the relocation and the symbols to the
# same effect.  Nothing is run, and no emulator is needed.
#
# The files are all.s, which has every instruction the assembler knows
# in each of the ways its operands may be written, and its directives;
# the boot blocks, made images by each (as -b, nasm -f bin), and
# /boot's start-up; the C library's own assembly, for each model, with
# the start-up
# code; the system call stubs as the library's mkstubs writes them, for
# each; the kernel's assembly; and what the system's C compiler writes
# for the programs of tests/ccom, in each model, as its second pass
# leaves it and as the object code improver does.
#
# Where a jump is one byte too far for the short form as NASM lays the
# code out and not as the assembler does, NASM keeps the long form and
# the assembler does not, and the two differ by that: no file here has
# such a jump.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/as
out=$top/obj/tests/as
libc=$top/usr/src/lib/libc/i286
as=$top/obj/host/ccom/as
mkdir -p "$out/stubs"
make -s -C "$top/usr/src/lib/ccom/i286" host

n=0
bad=0
# both FILE [FLAG ...]: the file through each assembler, with the flags
both() {
    src=$1
    shift
    n=$((n + 1))
    rm -f "$out/cmp"
    if nasm -f obj "$@" -o "$out/n.o" "$src" &&
        "$as" "$@" -o "$out/a.o" "$src" &&
        python3 -I "$here/objcmp.py" "$out/n.o" "$out/a.o" > "$out/cmp" 2>&1
    then
        return
    fi
    bad=$((bad + 1))
    echo "BAD $src $*"
    [ ! -f "$out/cmp" ] || cat "$out/cmp"
}

# image FILE: the file made an image by each, nasm -f bin and as -b,
# which are to be the same to the byte
image() {
    n=$((n + 1))
    if nasm -f bin -o "$out/n.bin" "$1" && "$as" -b -o "$out/a.bin" "$1" &&
        cmp "$out/n.bin" "$out/a.bin"
    then
        return
    fi
    bad=$((bad + 1))
    echo "BAD $1 -b"
}

both "$here/all.s"
# the boot blocks, and /boot's start-up
for f in fdboot mbr hdboot; do
    image "$top/usr/src/sys/i286stand/$f.s"
done
both "$top/usr/src/sys/i286stand/srt0.s"
for f in $(find "$libc" -name '*.s' | sort); do
    both "$f" -I "$libc/crt/"
    both "$f" -I "$libc/crt/" -DFAR
    both "$f" -I "$libc/crt/" -DFAR -DFARDATA
done
# and ve's, the one program with assembly of its own
both "$top/usr/src/i286/ve/fmem.s" -I "$libc/crt/"
both "$top/usr/src/i286/ve/fmem.s" -I "$libc/crt/" -DFAR
${HOSTCC:-cc} -std=gnu89 -w -o "$out/mkstubs" "$libc/sys/mkstubs.c"
for m in s m l; do
    rm -rf "$out/stubs/$m"
    mkdir -p "$out/stubs/$m"
    "$out/mkstubs" -m $m -d "$out/stubs/$m" \
        "$libc/sys/syscalls.tab" "$top/usr/include/syscall.h"
    for f in "$out/stubs/$m"/*.s; do
        both "$f"
    done
done
# the kernel's, which has segments of its own beyond the three, and
# groups of them (NET_GROUP), and what NASM's preprocessor has most of
for f in "$top"/usr/src/sys/i286/mch_*.s; do
    both "$f" -I "$top/usr/src/sys/i286/" -DFAR
done
for f in "$top"/tests/ccom/t_*.c "$top/tests/ccom/prog.c"; do
    b=$(basename "$f" .c)
    for m in s m l; do
        for o in "" -O; do
            "$top/tools/ncc" -S $o -m$m -DMEDIUM=0 -o "$out/$b.s" "$f"
            both "$out/$b.s"
        done
    done
done
echo "as: $n files, $bad not as NASM has them"
[ $bad = 0 ]
