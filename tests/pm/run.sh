#!/bin/sh
# Test the kernel's start-up and trap entry (usr/src/sys/i286) in an
# emulator: boot a small medium-model program in place of the kernel.
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
out=$top/obj/tests/pm
mkdir -p "$out"
cd "$out"

objs=
for f in "$sys/mch_start.s" "$sys/mch_trap.s" "$sys/mch_xxx.s" \
    "$top/tests/pm/pm_asm.s" "$crt/i4m.s" "$crt/i4d.s" "$crt/chp.s"; do
    o=$(basename "$f" .s).o
    nasm -f obj -DFAR -I "$sys/" -I "$crt/" -o "$o" "$f"
    objs="$objs,$o"
done
for f in "$sys/cons.c" "$top/tests/harness/printf.c" "$top/tests/pm/pm_test.c" \
    "$top/tests/pm/pm_far.c"; do
    o=$(basename "$f" .c).o
    "$top/tools/cc286" -mm -wcd=131 -wcd=107 -I"$sys" -o "$o" "$f"
    objs="$objs,$o"
done

# packcode=1: every module's code in a segment of its own, so that calls
# between modules change CS as they will in a kernel of several segments.
# Message 1014 is the lack of a stack segment; mch_start.s has the stack.
wlink op q format os2 name pm.ne disable 1014 \
    op nodefaultlibs,packcode=1,map=pm.map file "${objs#,}"
python3 -I "$top/tools/ne2aout.py" --model m --gdt 16 --map pm.map pm.ne pm
exec python3 -I "$top/tests/harness/runtest.py" --boot pm "$out/pm"
