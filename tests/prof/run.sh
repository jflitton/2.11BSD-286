#!/bin/sh
# Test profiling on the system: cc -p, mcrt0.o, the C library for it
# and prof.  By prof.sh root compiles t.c with cc -p and runs it, which
# writes mon.out, and prof says how many times each function was
# called: the program's own, one static, which prof -a has, the C
# library's abs, in C, and getpid, a system call's stub.  The same
# objects linked without -p write no mon.out; cc -p of the medium model
# is refused; and f77 -p profiles a Fortran program the same way.  What
# they print must be as `expected` has it, which was worked out from
# the programs: the counts and not the times, but that fib had some.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/prof needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/prof
. "$top/tests/harness/system.sh"
out=$top/obj/tests/prof$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
cd "$out"

cp "$proto" root.proto
for f in prof.sh t.c f.f; do
    echo "f /root/$f 644 $here/$f"
done >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" prof.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-1200} python3 -I "$top/tests/harness/runtest.py" \
    --hd prof.img,$geometry,xtide \
    --type "login: " root \
    --type "# " 'sh /root/prof.sh; echo "@ex""it 00"' \
    --boot prof$case

# What the system wrote from the first part to the last.
tr -d '\r' < serial.txt | sed -n '/^== cc -p$/,/^== end$/p' > got
if ! diff "$here/expected" got; then
    echo "prof: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "prof: cc -p, f77 -p and prof, as written"
