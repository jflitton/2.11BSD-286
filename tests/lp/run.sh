#!/bin/sh
# Test the parallel port's driver, lp(4), with a printer on the port:
# 86Box's generic text printer (tests/harness/runtest.py --lpt), which
# keeps what it prints as text.  By lp.sh root prints more than a page
# straight to /dev/lp0, and a file of tabs, a carriage return, spaces at
# the end of a line and a form feed; then has lpr and lpd print, through
# lpf, as /etc/printcap has the printer.  What the printer printed must
# be as `printed` has it, and what the system said as `expected`.  The
# printer acknowledges each character with an interrupt, which sends the
# next; without it the clock would send one every few seconds, and the
# printing would not end in time.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/lp needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/lp
. "$top/tests/harness/system.sh"
out=$top/obj/tests/lp$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
cd "$out"

cp "$proto" root.proto
echo "f /root/lp.sh 644 $here/lp.sh" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" lp.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-600} python3 -I "$top/tests/harness/runtest.py" \
    --hd lp.img,$geometry,xtide --lpt \
    --type "login: " root \
    --type "# " 'sh /root/lp.sh; echo "@ex""it 00"' \
    --boot lp$case

# What the system wrote from the first part to the last, and what the
# printer printed.
tr -d '\r' < serial.txt | sed -n '/^== probe$/,/^== end$/p' > got
if ! diff "$here/expected" got; then
    echo "lp: what the system said is not as $here/expected has it" >&2
    exit 1
fi
if ! cmp -s "$here/printed" printed.txt; then
    diff "$here/printed" printed.txt >&2 || :
    echo "lp: what the printer printed is not as $here/printed has it" >&2
    exit 1
fi
echo "lp: printed straight to the port and by lpd, as written"
