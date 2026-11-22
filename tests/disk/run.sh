#!/bin/sh
# Test that the system makes a disk it starts from, as 2.11BSD has a
# disk made (usr/doc/2.10/setup.2.10): boot it from a hard disk with its
# sources on /usr/src and a second disk with nothing on it, not a
# label, and have it, with its own programs,
#
#	label the second disk (disklabel): a the root, b the swap area,
#	    c all of it, d the sources;
#	make a file system for the root (newfs), restore a dump of its
#	    own root onto it (dump, restor) and check it (fsck);
#	give the disk the boot block (disklabel -B);
#	make a file system for the sources (newfs), copy /usr/src to it
#	    (tar) and check it.
#
# The disk it made must have nothing wrong with either file system
# (tools/fsck.py).  Its root must have what the system's own root has,
# name for name: each file's contents, mode and owner, each other name
# of a file, each link and each device, but the files a system that is
# running writes; and its sources what tools/srcmk.py lists.  Then the
# machine is started from that disk alone: it must come up, and make
# /boot and the boot block from the sources it was given, the same as
# the ones it started from.
#
# TOOLS, TOOLS_KERNEL, TOOLS_PROGRAMS and TOOLS_BOOT are as in
# tests/system (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/disk needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
name=disk$case
out=$top/obj/tests/$name
mkdir -p "$out"
buildsys

cd "$out"
cp "$top/obj/root/root.fs" root.fs
rm -rf fs
mkdir fs
python3 -I "$top/tools/srcmk.py" fs/src
python3 -I "$top/tools/mkfs.py" -s 64000 -t $(($(date +%s) - 172800)) \
    fs/src/proto src.fs >/dev/null

# What the system is to do.  The new disk is of 272000 sectors.  A
# disk with no label has one partition, all of it, which disklabel has
# open to write the label through and the label makes smaller: -W has
# the kernel let it, and -r writes the sector, the kernel writing a
# label only over one.  The root is given 6000 inodes: more, on one of
# 48000 blocks, and fsck could not check it as the system starts,
# where it has no scratch file.
cat > fs/disk.sh <<'EOF'
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
disklabel wd1 | sed '/^[0-9]* partitions:/,$d' > /tmp/label
cat >> /tmp/label << 'END'
4 partitions:
  a: 96000 0 2.11BSD 1024 1024
  b: 32768 96000 swap
  c: 272000 0 unused 0 0
  d: 128000 128768 2.11BSD 1024 1024
END
disklabel -W wd1 && disklabel -R -r wd1 /tmp/label && disklabel -N wd1 ||
    exit 1

newfs -i 8192 wd1a || exit 1
sync
dump 0f - /dev/rwd0a | restor rf - /dev/rwd1a || exit 1
fsck -y /dev/rwd1a
disklabel -B wd1 || exit 1

newfs wd1d || exit 1
mkdir /tmp/s
mount /dev/wd1d /tmp/s || exit 1
cd /usr/src && tar cf - . | (cd /tmp/s && tar xpf -) || exit 1
cd /
umount /tmp/s || exit 1
fsck -y -t /tmp/scratch /dev/rwd1d
disklabel wd1
EOF
{
    echo "f /disk.sh 644 fs/disk.sh"
} > want.proto
python3 -I "$top/tools/mkfs.py" want.proto want.fs >/dev/null

# The disk the system is on, and one of the same size with nothing on
# it: 133 MB.
shape=1000,16,17
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g $shape \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" disk.img a=root.fs \
    b=swap:8000 d=src.fs e=want.fs | tail -1)
rm -f new.img
dd if=/dev/zero of=new.img bs=512 count=0 seek=272000 2>/dev/null

TIMEOUT=${TIMEOUT:-7200} python3 -I "$top/tests/harness/runtest.py" \
    --hd disk.img,$geometry,xtide --hd2 new.img,$shape --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /usr/src; mount /dev/wd0e /mnt; sh /mnt/disk.sh > /tmp/out 2>&1; echo "disk: ma""de $?"' \
    --type "disk: made" 'sed "s/^#/:/" /tmp/out; umount /mnt; umount /usr/src; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name

serial=$out/serial.txt
if ! grep -q "^disk: made 0" "$serial"; then
    echo "$name: the system did not make the disk" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" new.img
python3 -I "$top/tools/fsck.py" -p d new.img
if ! dd if=new.img bs=512 count=1 2>/dev/null | cmp -s - "$stand/hdboot.bin"; then
    echo "$name: the disk does not begin with the boot block" >&2
    exit 1
fi
# The root as the system had it when it dumped it, which is the one
# made here but for what a system that is running writes: its logs,
# the daemons' files and syslogd's socket, who is logged in and the
# terminals' modes, cron's directory, acctd's configuration, and /tmp.
# newfs gives a file system a lost+found, which the sources have not.
running='^/dev/log \|^/dev/tty\|^/etc/acctd.cf \|^/tmp/\|^/usr/adm/\|^/var/cron\|^/var/run/'
python3 -I "$top/tools/fsck.py" -l root.fs | grep -v "$running" > root.want
python3 -I "$top/tools/fsck.py" -l new.img | grep -v "$running" > root.got
python3 -I "$top/tools/fsck.py" -l src.fs > src.want
python3 -I "$top/tools/fsck.py" -p d -l new.img | grep -v '^/lost+found d ' \
    > src.got
for f in root src; do
    if ! diff $f.want $f.got > $f.diff; then
        head -20 $f.diff >&2
        echo "$name: the disk's $f file system is not what the system" \
            "has: $(grep -c '^[<>]' $f.diff) lines of $out/$f.diff" >&2
        exit 1
    fi
done

# From the disk it made, and nothing else.
TIMEOUT=${TIMEOUT:-1800} python3 -I "$top/tests/harness/runtest.py" \
    --hd "$out/new.img",$shape,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /usr/src; cd /usr/src/sys/i286stand && make && cmp boot /boot && cmp hdboot.bin /mdec/wduboot && echo "disk: ma""kes"; df' \
    --type "# " 'cd /; umount /usr/src; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name-2
if ! grep -q "^disk: makes" "$top/obj/tests/$name-2/serial.txt"; then
    echo "$name: the system did not start from the disk it made, or did" \
        "not make /boot there as it was given it" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" new.img
python3 -I "$top/tools/fsck.py" -p d new.img
echo "$name: the system made a disk, with $(wc -l < root.want | tr -d ' ')" \
    "names in its root and $(wc -l < src.want | tr -d ' ') in its sources," \
    "each what it has itself, started from it and made /boot there"
