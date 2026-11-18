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

# The root file system, as the RAM disk of the first boot.
cp "$here/proto" proto
echo "The kernel's test file system." > motd
python3 -I "$top/tools/mkfs.py" -s 200 proto root.fs

# A swap area that takes most of the machine's 4M, so that the test's
# processes do not all fit in what is left.
cp "$top/obj/PC/unix" unix
python3 -I "$top/tools/kpatch.py" unix rdswap=5000

# Each boot is over when the kernel has stopped, not when the test has
# given its result: the kernel writes what it still holds to the disk
# after that.
python3 -I "$top/tests/harness/runtest.py" --until halting \
    --boot kernel unix root.fs

# Then the same from a hard disk, which has the root file system, with
# room in it this time, a swap area, and a partition for the test to
# write on.  The kernel is told to use little of the machine's memory,
# again so that it must swap.  Afterwards, what the kernel left on the
# disk: a file system with nothing wrong in it, the file the test wrote
# last, and something in the swap area.
#
#       diskboot NAME "MKDISK OPTIONS" VARIABLE=VALUE ...
(cat "$here/proto"; echo "f /etc/disk 644 motd") > proto.wd
python3 -I "$top/tools/mkfs.py" -s 2000 proto.wd root.wd
diskboot() {
    name=$1 options=$2
    shift 2
    geometry=$(python3 -I "$top/tools/mkdisk.py" $options $name.img \
        a=root.wd b=swap:6000 d=2000 | tail -1)
    cp "$top/obj/PC/unix" unix.$name
    python3 -I "$top/tools/kpatch.py" unix.$name extlimit=384 "$@"
    python3 -I "$top/tests/harness/runtest.py" --hd $name.img,$geometry \
        --until halting --boot kernel-$name unix.$name
    python3 -I "$top/tools/fsck.py" $name.img
    python3 -I "$top/tools/fsck.py" -x /persist $name.img |
        grep -q "written by init, to be found on the disk"
    python3 -I "$here/swapused.py" $name.img
}

# A drive the size of a PC/AT's, shared by a partition table.  It names
# sectors by cylinder, head and sector, and moves 16 to an interrupt.
diskboot wd "-p"

# A bigger one that is all the system's and names sectors by number,
# moving one to an interrupt.
diskboot wdlba "-g 1024,2,17" wdmulti=1

echo "kernel: all three boots passed"
