#!/bin/sh
# Test SLIP: boot the system with /etc/rc.conf giving sl0 the second
# serial port, COM2, at rc.conf's 9600, as 10.0.3.1, with the far end
# 10.0.3.2, sloop in /etc/hosts.  The far end is the harness
# (runtest.py --slip), which sends back each packet with its two
# addresses swapped, so that what the system sends to sloop comes back
# as from sloop, and the system talks to itself across the line.  By
# slip.sh root pings sloop,
# with packets that fit the line's 1006 bytes and with packets that IP
# divides in two, and copies a program to it and back with rcp, by
# rshd, which .rhosts lets root use from sloop.  What the system says,
# with the times taken out, must be as `expected` has it, and neither
# the interface nor IP may count an error.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/slip needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/slip
. "$top/tests/harness/system.sh"
out=$top/obj/tests/slip$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
dest=$top/obj/build/dest
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
    dest=$top/obj/nbuild/dest
fi
cd "$out"

rm -rf etc && mkdir etc
sed -e 's/^# sladdress=.*/sladdress=10.0.3.1/' \
    -e 's/^# sldest=.*/sldest=10.0.3.2/' "$dest/etc/rc.conf" > etc/rc.conf
cp "$dest/etc/hosts" etc/hosts
echo '10.0.3.2	sloop' >> etc/hosts
echo 'sloop root' > etc/rhosts
sed -e "s,^\(f /etc/rc.conf [^ ]*\) .*,\1 $out/etc/rc.conf," \
    -e "s,^\(f /etc/hosts [^ ]*\) .*,\1 $out/etc/hosts," \
    "$proto" > root.proto
echo "f /root/.rhosts 600 $out/etc/rhosts" >> root.proto
echo "f /root/slip.sh 644 $here/slip.sh" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" slip.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-600} python3 -I "$top/tests/harness/runtest.py" \
    --hd slip.img,$geometry,xtide --slip \
    --type "login: " root \
    --type "# " 'sh /root/slip.sh; echo "@ex""it 00"' \
    --boot slip$case

# What the system wrote from the first check to the last.
serial=$top/obj/tests/slip$case/serial.txt
tr -d '\r' < "$serial" | sed -n '/^== attach$/,/^== end$/p' |
    sed -e '/^$/d' > got
if ! diff "$here/expected" got; then
    echo "slip: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "slip: pings and a program sent and fetched by rcp, over COM2"
