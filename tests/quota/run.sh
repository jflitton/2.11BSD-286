#!/bin/sh
# Test disk quotas: boot the system with /home a file system of its own,
# which /etc/fstab says rq for, so that /etc/rc has quotacheck make its
# quota file and quotaon turn its quotas on, and a user, quser, whose
# home is there.  As root, quota.sh gives quser limits with edquota
# (ed.sh is its editor), and user.sh, as quser, fills them: past the
# soft limits, where the kernel warns, to the hard ones, where it
# refuses.  quser then logs in three times, and login has the kernel
# warn each time, and at the third say that the soft limits have
# become hard, as user.sh finds; removing files gives their bytes back.
# Root turns the quotas off, gives quser a file, has quotacheck count
# it, turns them on again, and starts the system again, after which the
# limits and the counts are still there.  What quota, repquota and the
# kernel say must be as `expected` has it.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/quota needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/quota
. "$top/tests/harness/system.sh"
out=$top/obj/tests/quota$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
dest=$top/obj/build/dest
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
    dest=$top/obj/nbuild/dest
fi
cd "$out"

# quser, with no password, and the password files made again with it.
rm -rf etc && mkdir etc
cp "$dest/etc/master.passwd" etc/master.passwd
echo 'quser::100:31::0:0:Quota User:/home/quser:/bin/sh' >> etc/master.passwd
python3 -I "$top/tools/mkpasswd.py" etc/master.passwd etc
sed 's,^\(/dev/wd0b.*\)$,\1\
/dev/wd0d	/home	ufs	rq		1	2,' "$top/usr/src/etc/fstab" > etc/fstab
sed -e "s,^f /etc/fstab .*,f /etc/fstab 644 $out/etc/fstab," \
    -e "s,^f /etc/master.passwd .*,f /etc/master.passwd 600 $out/etc/master.passwd," \
    -e "s,^f /etc/passwd .*,f /etc/passwd 644 $out/etc/passwd," \
    -e "s,^f /etc/passwd.dir .*,f /etc/passwd.dir 644 $out/etc/passwd.dir," \
    -e "s,^f /etc/passwd.pag .*,f /etc/passwd.pag 644 $out/etc/passwd.pag," \
    "$proto" > root.proto
echo "f /root/quota.sh 644 $here/quota.sh" >> root.proto
echo "f /root/user.sh 644 $here/user.sh" >> root.proto
echo "f /root/ed.sh 755 $here/ed.sh" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

# /home: quser's directory, and in it .hushlogin, so that login says
# nothing but what the kernel has it say of the quotas.
: > hushlogin
printf 'd /quser 755:100.31\nf /quser/.hushlogin 644:100.31 %s\n' \
    "$out/hushlogin" > home.proto
python3 -I "$top/tools/mkfs.py" -s 2000 home.proto home.fs >/dev/null

# A Bourne shell at a terminal takes exit to be for a script, and goes
# on; it ends at the end of its input, ^D.
CTRLD=$(printf '\004')
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" quota.img \
    a=root.fs b=swap:4000 d=home.fs | tail -1)
TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
    --hd quota.img,$geometry,xtide --resets 1 \
    --type "login: " root \
    --type "# " 'sh /root/quota.sh' \
    --type "# " "$CTRLD" \
    --type "login: " quser \
    --type "$ " 'quota' \
    --type "$ " "$CTRLD" \
    --type "login: " quser \
    --type "$ " 'quota' \
    --type "$ " "$CTRLD" \
    --type "login: " quser \
    --type "$ " 'sh /root/user.sh over' \
    --type "$ " "$CTRLD" \
    --type "login: " root \
    --type "# " 'sh /root/quota.sh check; reboot' \
    --type "login: " root \
    --type "# " 'sh /root/quota.sh after; echo "@ex""it 00"' \
    --boot quota$case

# What the system wrote from the first check to the reboot, and after
# it, without the times root last logged in, nor how long dd took.
serial=$top/obj/tests/quota$case/serial.txt
tr -d '\r' < "$serial" |
    sed -n -e '/^== quota$/,/^== rebooting$/p' -e '/^== after$/,/^== end$/p' |
    sed -e '/^Last login:/d' -e 's/ in [0-9.]* secs .*//' -e '/^$/d' > got
if ! diff "$here/expected" got; then
    echo "quota: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "quota: limits set, warned of, enforced, counted again and kept"
