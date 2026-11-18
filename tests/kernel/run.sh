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
python3 -I "$top/tools/mkstubs.py" --sysent "$top/usr/src/sys/sys/init_sysent.c" \
    "$here/syscalls.tab" "$top/usr/include/syscall.h" > stubs.s
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
#       diskboot NAME DRIVER BUS FROM "MKDISK OPTIONS" VARIABLE=VALUE ...
#
# FROM is where the kernel comes from:
#
#   diskette    a diskette, as on the first boot
#   disk        the disk itself, with no diskette in the machine: the
#               BIOS starts what is on the disk (usr/src/sys/i286stand),
#               and /boot is left to load /unix by itself
#   disk:NAME   the same, but the kernel is the file /sys/unix and NAME
#               is typed at /boot, after a name that is no file
make -s -C "$top/usr/src/sys/i286stand" > /dev/null
stand=$top/obj/stand
cp "$stand/boot" boot
# The boot block finds the blocks of a file past its fourth another way,
# and that way runs only if /boot has more than four.
[ $(wc -c < boot) -gt 4096 ]

diskboot() {
    name=$1 driver=$2 bus=$3 from=$4 options=$5
    shift 5
    printf $driver > disk
    cp "$top/obj/PC/unix" unix.$name
    python3 -I "$top/tools/kpatch.py" unix.$name extlimit=384 "$@"
    (cat "$here/proto"; echo "f /etc/disk 644 disk") > proto.$name
    if [ $driver = sd ]; then
        options="$options -t scsi"
    fi
    case $from in
    diskette)
        set -- --boot kernel-$name unix.$name ;;
    disk*)
        echo "f /boot 644 boot" >> proto.$name
        options="$options -b $stand/hdboot.bin"
        case $options in
        *-p*) options="$options -m $stand/mbr.bin" ;;
        esac
        if [ $from = disk ]; then
            echo "f /unix 644 unix.$name" >> proto.$name
            set -- --boot kernel-$name
        else
            (echo "d /sys 755"; echo "f /sys/unix 644 unix.$name") >> proto.$name
            set -- --type "Boot from" "unix" \
                --type "no such file" "${from#disk:}" --boot kernel-$name
        fi ;;
    esac
    python3 -I "$top/tools/mkfs.py" -s 2000 proto.$name root.$name
    geometry=$(python3 -I "$top/tools/mkdisk.py" $options $name.img \
        a=root.$name b=swap:6000 d=2000 | tail -1)
    python3 -I "$top/tests/harness/runtest.py" \
        --hd $name.img,$geometry,$bus --until halting "$@"
    if [ $from != diskette ]; then
        grep -q "^Boot from $driver(0,0)" "$top/obj/tests/kernel-$name/serial.txt"
    fi
    python3 -I "$top/tools/fsck.py" $name.img
    python3 -I "$top/tools/fsck.py" -x /persist $name.img |
        grep -q "written by init, to be found on the disk"
    python3 -I "$here/swapused.py" $name.img
}

# An IDE drive the size of a PC/AT's, shared by a partition table, that
# the machine boots from: the BIOS starts the disk's first sector, that
# the system's boot block, that /boot, and /boot the kernel, which is
# told its root is wd0a.  The drive names sectors by cylinder, head and
# sector, and moves 16 to an interrupt.
diskboot wd wd xtide disk "-p"

# A small one that is all the system's and names sectors by number,
# moving one to an interrupt.  The BIOS starts the boot block itself, and
# /boot is told what to load: first a file that is not there, then the
# kernel by its device and a path.
diskboot wdlba wd xtide "disk:wd(0,0)sys/unix" "-g 1024,2,17" wdmulti=1
grep -q "no such file" "$top/obj/tests/kernel-wdlba/serial.txt"

# One of 2.6 GB, of which the system has the end, past the sectors whose
# numbers fit in 16 bits and the bytes whose numbers fit in 31.  That is
# past what a BIOS reads too, so the kernel comes from a diskette.
diskboot wdbig wd ide diskette "-g 5400,16,63 -p -o 5000000"

# A SCSI disk on an Adaptec adapter set as it comes, shared by a
# partition table, booted from through the adapter's BIOS.  The label
# says it is a SCSI disk, from which /boot tells the kernel its root is
# sd0a.
diskboot sd sd scsi:330:11:5:0:DC000 disk "-p -g 20,64,32"

# Another on an adapter set otherwise, with its BIOS off, and at another
# SCSI id, of 2.6 GB with the system at the end of it.  The kernel comes
# from a diskette, which does not say where the root is: diskroot does,
# the device number of sd0a.  Here an interrupt is ignored and a command
# is never given to the adapter, and the driver must notice both and
# carry on.
diskboot sdlost sd scsi:334:10:6:5:0 diskette "-g 2600,64,32 -p -o 5000000" \
    diskroot=512 ahatest=3 ahatimeout=5
grep -q "aha: an interrupt did not arrive" "$top/obj/tests/kernel-sdlost/serial.txt"
grep -q "aha: no answer in 5 seconds" "$top/obj/tests/kernel-sdlost/serial.txt"

echo "kernel: all six boots passed"
