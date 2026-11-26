#!/bin/sh
# Test crash dumps: boot the system from a hard disk to multi-user and
# have its kernel panic.  System call 51, which no program makes, is
# given a function in no segment (tools/kpatch.py), and a program that
# makes it (crash.s) faults the kernel, which brings the disks up to
# date, writes its memory to the swap area and starts the machine
# again; /etc/crash being a link to /usr/crash, /etc/rc has savecore
# save the dump there, with the kernel.  Then reboot -d takes a dump
# without a panic, which savecore saves beside the first.  For each,
# check.sh shows what savecore saved and wrote in the shutdown log,
# the stack that adb finds in the dump, from where the dump was taken
# back to the system call, the panic's message, the processes that ps
# finds in the dump and the tables pstat does, which must be as
# `expected` says.  Once on an IDE disk and once on a SCSI disk on the
# Adaptec, each driver writing the dump itself.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/crash needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/crash
. "$top/tests/harness/system.sh"
out=$top/obj/tests/crash$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
cd "$out"
# The cross compiler's kernel has a stack of other frames: its static
# functions have no symbols, so that ktrap's frame is named for the
# function before it, and reboot needs no frame and makes none.
want=$here/expected
if [ "$TOOLS_KERNEL" = cross ]; then
    want=$here/expected-cross
fi
cp "$kernel" unix
python3 -I "$top/tools/kpatch.py" unix sysent+310=0

# The root the build made, with that kernel, the scripts, and room for
# two dumps of the machine's 4 MB; and a swap area with room for one
# past where the kernel begins it (dumplo).
crash() {
    name=$1 disk=$2 bus=$3 options=$4
    sed "s,^/dev/wd0,/dev/$disk," "$top/usr/src/etc/fstab" > fstab.$name
    sed -e "s,^f /unix .*,f /unix 744:0.2 $out/unix," \
        -e "s,^f /etc/fstab .*,f /etc/fstab 644 $out/fstab.$name," \
        "$proto" > proto.$name
    for f in crash.s panic.sh check.sh; do
        echo "f /root/$f 644 $here/$f" >> proto.$name
    done
    python3 -I "$top/tools/mkfs.py" -s 40000 proto.$name root.$name \
        >/dev/null
    geometry=$(python3 -I "$top/tools/mkdisk.py" -p $options \
        -b "$stand/hdboot.bin" -m "$stand/mbr.bin" $name.img \
        a=root.$name b=swap:10000 | tail -1)
    TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
        --hd $name.img,$geometry,$bus --resets 4 \
        --type "login: " root \
        --type "# " 'sh /root/panic.sh' \
        --type "login: " root \
        --type "# " 'sh /root/check.sh 0 panic; reboot -d' \
        --type "login: " root \
        --type "# " 'sh /root/check.sh 1; echo "@ex""it 00"' \
        --boot crash-$name$case
    serial=$top/obj/tests/crash-$name$case/serial.txt
    for w in "panic: trap" "dumping to dev .* off 512" "dump succeeded"; do
        if ! grep -q "$w" "$serial"; then
            echo "crash: no \"$w\" in what the $name system wrote" >&2
            exit 1
        fi
    done
    tr -d '\r' < "$serial" | sed -n '/^== dump/,/^== end/p' > got.$name
    if ! diff "$want" got.$name; then
        echo "crash: the $name system's dumps are not as $want has them" >&2
        exit 1
    fi
}

# An IDE disk shared by a partition table, as tests/system's is, and a
# SCSI disk on the adapter as it comes, booted through its BIOS, as
# tests/kernel's first is: each with room for the root and two dumps.
crash wd wd0 xtide "-g 977,10,17"
crash sd sd0 scsi:330:11:5:0:DC000 "-t scsi -g 60,64,32"
echo "crash: a panic's dump and reboot -d's saved and read, on IDE and SCSI"
