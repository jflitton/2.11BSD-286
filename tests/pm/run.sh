#!/bin/sh
# Test the kernel's start-up and trap entry (usr/src/sys/i286) in an
# emulator: boot a small medium-model program in place of the kernel.
# With NATIVE=1, or NATIVE_KERNEL=1, it is the work of the system's own
# compiler, assembler and linker, in obj/tests/pm-n, and not the cross
# compiler's (tests/harness/system.sh).
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/pm needs 86Box: QEMU runs 386 instructions, and does not" >&2
    echo "enforce segment limits or write protection, all of which it checks" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
sys=$top/usr/src/sys/i286
crt=$top/usr/src/lib/libc/i286/crt
. "$top/tests/harness/system.sh"
name=pm
host=$top/obj/host/ccom
if [ "$NATIVE_KERNEL" = 1 ]; then
    mk -C "$top/usr/src/lib/ccom/i286" host
    name=pm-n
fi
out=$top/obj/tests/$name
mkdir -p "$out"
cd "$out"

objs=
for f in "$sys/mch_start.s" "$sys/mch_trap.s" "$sys/mch_xxx.s" \
    "$top/tests/pm/pm_asm.s" "$crt/i4m.s" "$crt/i4d.s" "$crt/chp.s"; do
    o=$(basename "$f" .s).o
    if [ "$NATIVE_KERNEL" = 1 ]; then
        "$host/as" -mm -DFAR -I"$sys/" -I"$crt/" -o "$o" "$f"
    else
        nasm -f obj -DFAR -I "$sys/" -I "$crt/" -o "$o" "$f"
    fi
    objs="$objs,$o"
done
for f in "$top/tests/harness/cons.c" "$top/tests/harness/kern.c" \
    "$top/tests/harness/printf.c" "$top/tests/pm/pm_test.c" \
    "$top/tests/pm/pm_far.c"; do
    o=$(basename "$f" .c).o
    if [ "$NATIVE_KERNEL" = 1 ]; then
        "$top/tools/ncc" -mm -I"$sys" -o "$o" "$f"
    else
        "$top/tools/cc286" -mm -wcd=131 -wcd=107 -I"$sys" -o "$o" "$f"
    fi
    objs="$objs,$o"
done

# packcode=1: every module's code in a segment of its own, so that calls
# between modules change CS as they will in a kernel of several segments.
# Message 1014 is the lack of a stack segment; mch_start.s has the stack.
# The system's linker packs files in the order they are given, and the
# kernel's assembly, which is first, calls itself near and is under 2K:
# with 2K to a segment it is in the first, and the C in three more.
if [ "$NATIVE_KERNEL" = 1 ]; then
    "$host/ld" -mm -gdt 16 -stack 0 -packcode 2048 -M -o pm \
        $(echo "${objs#,}" | tr , ' ') > pm.map
else
    wlink op q format os2 name pm.ne disable 1014 \
        op nodefaultlibs,packcode=1,map=pm.map file "${objs#,}"
    python3 -I "$top/tools/ne2aout.py" --model m --gdt 16 --map pm.map pm.ne pm
fi
exec python3 -I "$top/tests/harness/runtest.py" --boot $name "$out/pm"
