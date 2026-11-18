#!/bin/sh
# Test processes: ring 3, system calls, and copies between the kernel and
# a process (usr/src/sys/i286).  u_start.s and u_test.c are a small-model
# program; k_main.c and the kernel's own start-up are a medium-model
# kernel that carries that program as data, loads it and runs it.
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/user needs 86Box: QEMU runs 386 instructions, and does not" >&2
    echo "enforce segment limits or write protection, all of which it checks" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
sys=$top/usr/src/sys/i286
crt=$top/usr/src/lib/libc/i286/crt
here=$top/tests/user
out=$top/obj/tests/user
mkdir -p "$out/u" "$out/k"

# The process.
cd "$out/u"
objs=u_start.o
nasm -f obj -o u_start.o "$here/u_start.s"
for f in "$crt/i4m.s" "$crt/i4d.s" "$crt/chp.s"; do
    o=$(basename "$f" .s).o
    nasm -f obj -I "$crt/" -o "$o" "$f"
    objs="$objs,$o"
done
for f in "$here/u_test.c" "$top/tests/harness/printf.c"; do
    o=$(basename "$f" .c).o
    "$top/tools/cc286" -ms -wcd=131 -wcd=107 -o "$o" "$f"
    objs="$objs,$o"
done
wlink op q format os2 name user.ne disable 1014 op nodefaultlibs,stack=1024 file "$objs"
python3 -I "$top/tools/ne2aout.py" --model s user.ne "$out/k/user"

# The kernel.
cd "$out/k"
objs=
for f in "$sys/mch_start.s" "$sys/mch_trap.s" "$sys/mch_xxx.s" "$sys/mch_copy.s" \
    "$top/tests/pm/pm_asm.s" "$here/k_image.s" "$crt/i4m.s" "$crt/i4d.s"; do
    o=$(basename "$f" .s).o
    nasm -f obj -DFAR -I "$sys/" -I "$crt/" -o "$o" "$f"
    objs="$objs,$o"
done
for f in "$sys/cons.c" "$top/tests/harness/printf.c" "$here/k_main.c"; do
    o=$(basename "$f" .c).o
    "$top/tools/cc286" -mm -wcd=131 -wcd=107 -I"$sys" -o "$o" "$f"
    objs="$objs,$o"
done
# Message 1014 is the lack of a stack segment; mch_start.s has the stack.
wlink op q format os2 name kernel.ne disable 1014 \
    op nodefaultlibs,map=kernel.map file "${objs#,}"
python3 -I "$top/tools/ne2aout.py" --model m --gdt 16 --map kernel.map kernel.ne kernel
exec python3 -I "$top/tests/harness/runtest.py" --boot user "$out/k/kernel"
