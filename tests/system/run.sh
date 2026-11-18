#!/bin/sh
# Test the system: boot it from a hard disk to multi-user, log in on the
# console as root, use the shell and some programs, and halt it.  The
# kernel, /boot, the C library, the programs and the root file system are
# all built as they would be for a real machine (usr/src/sys/PC,
# usr/src/sys/i286stand, usr/src/lib/libc/i286, usr/src/i286).
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/system needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
out=$top/obj/tests/system
mkdir -p "$out"

quiet() { grep -v '^cc \|^as \|^wlib \|Warning!\|Note!\|^unix:' || true; }
make -s -C "$top/usr/src/sys/PC" | quiet
make -s -C "$top/usr/src/sys/i286stand" | quiet
make -s -C "$top/usr/src/lib/libc/i286" | quiet
make -s -C "$top/usr/src/i286" all | quiet
make -s -C "$top/usr/src/i286" root | quiet

# An IDE disk shared by a partition table, as tests/kernel's first, with
# the root file system in partition a and a swap area in b.
cd "$out"
stand=$top/obj/stand
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -b "$stand/hdboot.bin" \
    -m "$stand/mbr.bin" system.img a="$top/obj/root/root.fs" b=swap:4000 |
    tail -1)

# Each text to type waits for something only the system writes: a typed
# line is echoed, so what it waits for is split by quotes when typed.
TIMEOUT=${TIMEOUT:-400} python3 -I "$top/tests/harness/runtest.py" \
    --hd system.img,$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'echo "she""ll-ok" $HOME' \
    --type "shell-ok /root" 'cd /tmp; mkdir dir; cd dir; pwd' \
    --type "/tmp/dir" 'cp /etc/motd m; cmp /etc/motd m && echo "sa""me"' \
    --type "same" 'for i in a b c; do echo "x$i"; done' \
    --type "xc" 'sh -c "exit 3"; echo "status $?"' \
    --type "status 3" 'echo /dev/*' \
    --type "/dev/wd1h" 'ls -l /etc | grep pass""wd' \
    --type "master.passwd" 'who; echo "@ex""it 00"; sync; /sbin/reboot -h' \
    --boot system

serial=$top/obj/tests/system/serial.txt
for want in "shell-ok /root" "/tmp/dir" "same" "xa" "xb" "xc" "status 3" \
    "/dev/console /dev/kmem" "master.passwd" "root  *console" "halting"; do
    if ! grep -q "$want" "$serial"; then
        echo "system: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" system.img
python3 -I "$top/tools/fsck.py" -x /tmp/dir/m system.img | grep -q "for the 286"
echo "system: logged in, used the shell and halted"
