#!/bin/sh
# Test Pascal on the system: pi, px, pix and pxp.  By pascal.sh root
# translates the programs here with pi and runs them: control (t1.p),
# arithmetic and how numbers are written (t2.p), with px; sets, arrays,
# records, strings and pointers (t3.p), with pix; files and the input
# (t4.p), as an object that runs itself; Berkeley Pascal's own built in
# routines (t5.p); each error px finds as a program runs, with the calls
# it was made in (errors.p); and pxp on prof.p, printing it again, and
# its profile after a run.  What they print must be as `expected` has
# it, which was worked out from the programs and from how px prints.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/pascal needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/pascal
. "$top/tests/harness/system.sh"
out=$top/obj/tests/pascal$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
cd "$out"

cp "$proto" root.proto
for f in pascal.sh t1.p t2.p t3.p t4.p t5.p errors.p prof.p; do
    echo "f /root/$f 644 $here/$f"
done >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" pascal.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-1800} python3 -I "$top/tests/harness/runtest.py" \
    --hd pascal.img,$geometry,xtide \
    --type "login: " root \
    --type "# " 'sh /root/pascal.sh; echo "@ex""it 00"' \
    --boot pascal$case

# What the system wrote from the first part to the last.
tr -d '\r' < serial.txt | sed -n '/^== control$/,/^== end$/p' > got
if ! diff "$here/expected" got; then
    echo "pascal: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "pascal: pi, px, pix and pxp, as written"
