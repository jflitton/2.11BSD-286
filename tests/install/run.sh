#!/bin/sh
# Test that the system makes what a machine with nothing on its disk is
# given the system from: boot it from a hard disk, with a diskette in
# the second drive and a second disk with nothing on it, and have it
#
#	format the diskette and make of it the one to start from
#	    (flformat, /usr/src/sys/i286stand/buildmini): /boot, the
#	    kernel and the miniroot, a root file system for memory;
#	dump its root to the second disk, as to a tape (dump).
#
# The diskette must have nothing wrong with its file system, nor the
# miniroot in it with its own (tools/fsck.py).  Then the machine is
# started from the diskette, with a first disk that has nothing on it,
# not a label, and the second: it must come up on the miniroot with a
# shell at the console, and there label the disk, make a file system,
# restore the dump to it, check it and give it the boot block
# (disklabel, newfs, restor, fsck, disklabel -B), as upstream's document
# has a root restored (usr/doc/2.10/setup.2.10).  The disk must then
# have nothing wrong with it and hold what the system's own root has,
# name for name, but the files a running system writes; and the machine
# is started from it alone and must come up.
#
# TOOLS, TOOLS_KERNEL, TOOLS_PROGRAMS and TOOLS_BOOT are as in
# tests/system (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/install needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
name=install$case
out=$top/obj/tests/$name
mkdir -p "$out"
buildsys

cd "$out"
cp "$top/obj/root/root.fs" root.fs
rm -rf fs
mkdir fs
# What the system is to do.  buildmini mounts the diskette on /mnt.  The
# second disk is written from its first sector, as a tape is, and the
# kernel keeps the sector a label would be in from being written until
# disklabel -W has it not.
cat > fs/make.sh <<'EOF'
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
flformat /dev/rfl1 < /dev/null || exit 1
sh /tmp/w/buildmini fl1 || exit 1
sync
disklabel -W wd1 || exit 1
dump 0f /dev/rwd1a /dev/rwd0a || exit 1
EOF
{
    echo "f /make.sh 644 fs/make.sh"
    echo "f /buildmini 755 $top/usr/src/sys/i286stand/buildmini"
} > want.proto
python3 -I "$top/tools/mkfs.py" want.proto want.fs >/dev/null

# The disk the system is on; one of the same size for the dump, and one
# to install on, with nothing on either; and a diskette never formatted.
shape=977,5,17
sectors=83045
geometry=$(python3 -I "$top/tools/mkdisk.py" -g $shape \
    -b "$stand/hdboot.bin" disk.img a=root.fs b=swap:8000 e=want.fs | tail -1)
rm -f tape.img new.img
dd if=/dev/zero of=tape.img bs=512 count=0 seek=$sectors 2>/dev/null
dd if=/dev/zero of=new.img bs=512 count=0 seek=$sectors 2>/dev/null
python3 -I -c 'open("mini.img", "wb").write(b"\xa5" * 1474560)'

TIMEOUT=${TIMEOUT:-7200} python3 -I "$top/tests/harness/runtest.py" \
    --hd disk.img,$geometry,xtide --hd2 tape.img,$shape --fdb mini.img \
    --until halting \
    --type "login: " "root" \
    --type "# " 'mkdir /tmp/w; mount /dev/wd0e /tmp/w; sh /tmp/w/make.sh > /tmp/out 2>&1; echo "install: ma""de $?"' \
    --type "install: made" 'sed "s/^#/:/" /tmp/out; umount /tmp/w; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name
if ! grep -q "^install: made 0" "$out/serial.txt"; then
    echo "$name: the system did not make the diskette and the dump" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" mini.img
if ! dd if=mini.img bs=512 count=1 2>/dev/null | cmp -s - "$stand/flboot.bin"; then
    echo "$name: the diskette does not begin with the boot block" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" -x /miniroot mini.img > miniroot.fs
python3 -I "$top/tools/fsck.py" miniroot.fs

# From the diskette, onto the disk with nothing on it.  The miniroot's
# shell is at the console, the screen and keyboard: it is given one
# that reads the serial port, and that one the rest.  The label is the
# kernel's own for a disk without one, with these partitions.  restor
# waits for a line before it writes.
TIMEOUT=${TIMEOUT:-7200} python3 -I "$top/tests/harness/runtest.py" \
    --hd "$out/new.img",$shape,xtide --hd2 "$out/tape.img",$shape \
    --fda "$out/mini.img" --until halting \
    --key "# " 'sh < /dev/tty00 > /dev/tty00 2>&1' \
    --type "# " "disklabel wd0 | sed '/^[0-9]* partitions:/,\$d' > /tmp/l; echo '3 partitions:' >> /tmp/l" \
    --type "# " "echo ' a: 64000 0 2.11BSD 1024 1024' >> /tmp/l; echo ' b: 8000 64000 swap' >> /tmp/l; echo ' c: $sectors 0 unused 0 0' >> /tmp/l" \
    --type "# " 'disklabel -W wd0 && disklabel -R -r wd0 /tmp/l && disklabel -N wd0 && newfs -i 8192 wd0a; echo "install: la""bel $?"' \
    --type "install: label" 'echo | restor rf /dev/rwd1a /dev/rwd0a; echo "install: res""tor $?"' \
    --type "install: restor" 'fsck -y /dev/rwd0a; disklabel -B wd0; echo "install: bo""ot $?"' \
    --type "install: boot" 'echo "@ex""it 00"; sync; reboot -h' \
    --boot $name-2
serial=$top/obj/tests/$name-2/serial.txt
for step in "label 0" "restor 0" "boot 0"; do
    if ! grep -q "^install: $step" "$serial"; then
        echo "$name: on the miniroot, the step that ends \"$step\" did" \
            "not" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" new.img
# As tests/disk compares a root that was dumped and restored.
running='^/dev/log \|^/dev/tty\|^/etc/acctd.cf \|^/tmp/\|^/usr/adm/\|^/var/cron\|^/var/run/'
python3 -I "$top/tools/fsck.py" -l root.fs | grep -v "$running" > root.want
python3 -I "$top/tools/fsck.py" -l new.img | grep -v "$running" > root.got
if ! diff root.want root.got > root.diff; then
    head -20 root.diff >&2
    echo "$name: the disk's root is not what the system has:" \
        "$(grep -c '^[<>]' root.diff) lines of $out/root.diff" >&2
    exit 1
fi

# From the disk it was installed on, and nothing else.
TIMEOUT=${TIMEOUT:-1800} python3 -I "$top/tests/harness/runtest.py" \
    --hd "$out/new.img",$shape,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'df; echo "install: u""p"; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name-3
if ! grep -q "^install: up" "$top/obj/tests/$name-3/serial.txt"; then
    echo "$name: the system did not start from the disk it was" \
        "installed on" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" new.img
echo "$name: the system made a diskette to install from and a dump of" \
    "its root; a machine started from the diskette restored it to a" \
    "disk with nothing on it, $(wc -l < root.want | tr -d ' ') names," \
    "and started from that"
