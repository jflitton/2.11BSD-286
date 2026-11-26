#!/bin/sh
# Test sendmail.MX, the sendmail that upstream has and does not make,
# which finds where to send a letter by the domain's MX records: the
# build makes it and does not install it (tools/build/build.sh), and
# this installs it in a root file system as its Makefile would, as
# /usr/sbin/sendmail, setgid kmem, with its messages and its help, and
# boots that.  By mx.sh root has it deliver a letter here, through an
# alias, made anew by newaliases, and with the configuration frozen;
# and has named serve a domain whose relay has two MX records, and
# sendmail send a letter by SMTP by the one that works, to its own
# daemon, which delivers it here; and a letter for a relay that is
# no host come back.  What the system says must be as `expected` has it.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/mx needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/mx
. "$top/tests/harness/system.sh"
out=$top/obj/tests/mx$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
mx=$made/usr.sbin/sendmail.MX
cd "$out"

python3 -I "$top/tools/strip.py" "$mx/src/sendmail" sendmail
sed -e "s,^f /usr/sbin/sendmail .*,f /usr/sbin/sendmail 6755:0.2 $out/sendmail," \
    -e "s,^\(f /usr/share/misc/sendmail.sr [^ ]*\) .*,\1 $mx/src/sendmail.sr," \
    -e "s,^\(f /usr/share/misc/sendmail.hf [^ ]*\) .*,\1 $mx/lib/sendmail.hf," \
    "$proto" > root.proto
echo "f /root/mx.sh 644 $here/mx.sh" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" mx.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-600} python3 -I "$top/tests/harness/runtest.py" \
    --hd mx.img,$geometry,xtide \
    --type "login: " root \
    --type "# " 'sh /root/mx.sh; echo "@ex""it 00"' \
    --boot mx$case

# What the system wrote from the first part to the last.
serial=$top/obj/tests/mx$case/serial.txt
tr -d '\r' < "$serial" | sed -n '/^== local$/,/^== end$/p' |
    sed -e '/^$/d' > got
if ! diff "$here/expected" got; then
    echo "mx: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "mx: letters delivered here and by an MX record, and one returned"
