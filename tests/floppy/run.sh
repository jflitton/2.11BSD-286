#!/bin/sh
# Test that the system makes diskettes, as 2.11BSD has an RX02 made
# (rx(4), rxformat(8)), and ones that start it: boot it from a hard
# disk with a diskette of 1.44 MB that has never been formatted in the
# second drive, a 3.5 inch one, and have it, with its own programs,
#
#	format it (flformat);
#	make a file system (newfs, which the driver's label tells what
#	    the diskette is), copy /boot and /unix to it, and check it
#	    (fsck);
#	give it the boot block (dd).
#
# The diskette must have nothing wrong with its file system
# (tools/fsck.py), begin with the boot block, hold /boot and /unix as
# the system's root has them, and have no sector left as it was.  Then
# the machine is started from it, with a hard disk the BIOS finds
# nothing to start on: it must come up with its root on that disk, and
# there make the same of a diskette of 1.2 MB in a 5.25 inch drive.
# Started from that one, it formats a double density diskette of 720K
# in a 3.5 inch drive, writes the kernel and the boot programs to it
# with tar and reads them back the same, the driver having found the
# diskette is not one of 1.44 MB, then makes a file system on it and
# puts a file there.
#
# TOOLS, TOOLS_KERNEL, TOOLS_PROGRAMS and TOOLS_BOOT are as in
# tests/system (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/floppy needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
name=floppy$case
out=$top/obj/tests/$name
mkdir -p "$out"
buildsys

cd "$out"
cp "$top/obj/root/root.fs" root.fs
rm -rf fs
mkdir fs

# What the system is to do with a diskette it is to start from, in the
# second drive: make.sh, which is told the kilobytes newfs is to find
# it has, and when the system was itself started from one, compares
# that one's kernel with its own.  more.sh is for the diskette of 720K.
# flformat asks before it formats when it is run from a terminal.
cat > fs/make.sh <<'EOF'
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
mkdir /tmp/f
if test $# = 2
then
	mount /dev/$2 /tmp/f || exit 1
	cmp /unix /tmp/f/unix || exit 1
	umount /tmp/f || exit 1
fi
flformat /dev/rfl1 < /dev/null || exit 1
newfs /dev/rfl1 > /tmp/newfs || exit 1
cat /tmp/newfs
grep " -s $1 /dev/rfl1" /tmp/newfs > /dev/null || exit 1
mount /dev/fl1 /tmp/f || exit 1
cp /boot /unix /tmp/f || exit 1
umount /tmp/f || exit 1
dd if=/mdec/fluboot of=/dev/rfl1 count=1 || exit 1
fsck -y /dev/rfl1
EOF
cat > fs/more.sh <<'EOF'
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
mkdir /tmp/f /tmp/x
mount /dev/fl0 /tmp/f || exit 1
cmp /unix /tmp/f/unix || exit 1
umount /tmp/f || exit 1
flformat -f 720 /dev/rfl1 < /dev/null || exit 1
(cd / && tar cf /dev/rfl1 unix boot mdec) || exit 1
(cd /tmp/x && tar xpf /dev/rfl1) || exit 1
for f in unix boot mdec/wduboot mdec/fluboot
do
	cmp /$f /tmp/x/$f || exit 1
done
newfs /dev/rfl1 > /tmp/newfs || exit 1
cat /tmp/newfs
grep " -s 720 /dev/rfl1" /tmp/newfs > /dev/null || exit 1
mount /dev/fl1 /tmp/f || exit 1
cp /etc/services /tmp/f || exit 1
umount /tmp/f || exit 1
fsck -y /dev/rfl1
EOF
{
    echo "f /make.sh 644 fs/make.sh"
    echo "f /more.sh 644 fs/more.sh"
} > want.proto
python3 -I "$top/tools/mkfs.py" want.proto want.fs >/dev/null

# The disk the system is on, all of it the system's; and the diskettes,
# each with every byte 0xA5, which is neither what formatting writes
# nor anything a file system is made of.
shape=977,5,17
geometry=$(python3 -I "$top/tools/mkdisk.py" -g $shape \
    -b "$stand/hdboot.bin" disk.img a=root.fs b=swap:8000 e=want.fs | tail -1)
blank() {
    python3 -I -c 'import sys
open(sys.argv[1], "wb").write(b"\xa5" * int(sys.argv[2]))' "$1" "$2"
}
blank high.img 1474560
blank five.img 1228800
blank double.img 737280

TIMEOUT=${TIMEOUT:-3600} python3 -I "$top/tests/harness/runtest.py" \
    --hd disk.img,$geometry,xtide --fdb high.img --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0e /mnt; sh /mnt/make.sh 1440 > /tmp/out 2>&1; echo "floppy: ma""de $?"' \
    --type "floppy: made" 'sed "s/^#/:/" /tmp/out; umount /mnt; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name

if ! grep -q "^floppy: made 0" "$out/serial.txt"; then
    echo "$name: the system did not make the diskette of 1.44 MB" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" -l root.fs | grep '^/boot \|^/unix ' |
    awk '{ print $1, $5, $6 }' > boot.want
# How many of the diskette's sectors are as they were before the system
# had it, and how many as formatting leaves one.
sectors() {
    python3 -I -c 'import sys
d = open(sys.argv[1], "rb").read()
s = [d[i:i + 512] for i in range(0, len(d), 512)]
print(s.count(b"\xa5" * 512), s.count(b"\xf6" * 512))' "$1"
}
check() {
    python3 -I "$top/tools/fsck.py" $1.img
    if ! dd if=$1.img bs=512 count=1 2>/dev/null |
        cmp -s - "$stand/flboot.bin"; then
        echo "$name: $1.img does not begin with the boot block" >&2
        exit 1
    fi
    python3 -I "$top/tools/fsck.py" -l $1.img | grep '^/boot \|^/unix ' |
        awk '{ print $1, $5, $6 }' > $1.got
    if ! cmp -s boot.want $1.got; then
        echo "$name: $1.img has not /boot and /unix as the root has them" >&2
        exit 1
    fi
    set -- $1 $(sectors $1.img)
    if [ "$2" != 0 ] || [ "$3" -lt 1000 ]; then
        echo "$name: $1.img was not formatted: $2 sectors are as they" \
            "were, and $3 as formatting leaves them" >&2
        exit 1
    fi
}
check high

# From that diskette, the disk having nothing in its first sector for
# the BIOS to start; and then from the one made there.
dd if=/dev/zero of=disk.img bs=512 count=1 conv=notrunc 2>/dev/null
TIMEOUT=${TIMEOUT:-3600} python3 -I "$top/tests/harness/runtest.py" \
    --hd "$out/disk.img",$geometry,xtide --fda "$out/high.img" \
    --fdb "$out/five.img" --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0e /mnt; sh /mnt/make.sh 1200 fl0 > /tmp/out 2>&1; echo "floppy: fi""ve $?"' \
    --type "floppy: five" 'sed "s/^#/:/" /tmp/out; umount /mnt; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name-2
if ! grep -q "^floppy: five 0" "$top/obj/tests/$name-2/serial.txt"; then
    echo "$name: the system did not start from the diskette of 1.44 MB," \
        "or did not make the one of 1.2 MB" >&2
    exit 1
fi
check five

TIMEOUT=${TIMEOUT:-1800} python3 -I "$top/tests/harness/runtest.py" \
    --hd "$out/disk.img",$geometry,xtide --fda "$out/five.img" \
    --fdb "$out/double.img",1474560 --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0e /mnt; sh /mnt/more.sh > /tmp/out 2>&1; echo "floppy: mo""re $?"' \
    --type "floppy: more" 'sed "s/^#/:/" /tmp/out; umount /mnt; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name-3
if ! grep -q "^floppy: more 0" "$top/obj/tests/$name-3/serial.txt"; then
    echo "$name: the system did not start from the diskette of 1.2 MB," \
        "or did not make the one of 720K" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" double.img
if ! python3 -I "$top/tools/fsck.py" -l double.img | grep -q '^/services f '; then
    echo "$name: the diskette of 720K has not the file put on it" >&2
    exit 1
fi
set -- $(sectors double.img)
if [ "$1" != 0 ]; then
    echo "$name: double.img was not formatted: $1 sectors are as they were" >&2
    exit 1
fi

python3 -I "$top/tools/fsck.py" high.img
python3 -I "$top/tools/fsck.py" five.img
echo "$name: the system formatted and made diskettes of 1.44 MB and 1.2 MB" \
    "and started from each, and formatted one of 720K for tar and for a" \
    "file system"
