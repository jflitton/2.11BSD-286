#!/bin/sh
# Test the kernel: build it, build a root file system with a test program
# as /etc/init, and boot the two in the emulator (usr/src/sys/PC,
# tests/kernel/init.c).
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/kernel needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
crt=$top/usr/src/lib/libc/i286/crt
here=$top/tests/kernel
out=$top/obj/tests/kernel
mkdir -p "$out"

make -s -C "$top/usr/src/sys/PC" | grep -v '^cc \|^as ' || true

# The test programs: small model.
cd "$out"
python3 -I "$top/tools/mkstubs.py" "$here/syscalls.tab" > stubs.s
lib=crt0.o
nasm -f obj -o crt0.o "$here/crt0.s"
for f in stubs.s "$crt/i4m.s" "$crt/i4d.s"; do
    o=$(basename "$f" .s).o
    nasm -f obj -I "$crt/" -o "$o" "$f"
    lib="$lib,$o"
done
for f in "$here/lib.c" "$top/tests/harness/printf.c"; do
    o=$(basename "$f" .c).o
    "$top/tools/cc286" -ms -wcd=131 -wcd=107 -I"$here" -o "$o" "$f"
    lib="$lib,$o"
done
for p in init child; do
    "$top/tools/cc286" -ms -wcd=131 -wcd=107 -I"$here" -o $p.o "$here/$p.c"
    wlink op q format os2 name $p.ne disable 1014 \
        op nodefaultlibs,stack=2048 file "$lib,$p.o"
    python3 -I "$top/tools/ne2aout.py" --model s $p.ne $p
done

# The root file system.
cp "$here/proto" proto
echo "The kernel's test file system." > motd
python3 -I "$top/tools/mkfs.py" -s 200 proto root.fs

# A swap area that takes most of the machine's 4M, so that the test's
# processes do not all fit in what is left.
cp "$top/obj/PC/unix" unix
python3 -I "$top/tools/kpatch.py" unix rdswap=5000

exec python3 -I "$top/tests/harness/runtest.py" --boot kernel unix root.fs
