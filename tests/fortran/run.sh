#!/bin/sh
# Test Fortran on the system: f77, its libraries libF77, libI77 and
# libU77, and ratfor, struct and efl.  By fortran.sh root compiles the
# programs here with f77 and runs them: control (t1.f), with -O1 too,
# which gives a DO's variable a register; arithmetic and the intrinsic
# functions (t2.f); subprograms in a file compiled apart (t3.f, t3s.f);
# characters and files (t4.f); ratfor's (t5.r); and t6.f as it is and
# as struct writes it again.  What they print must be as `expected` has
# it, which was worked out from the programs and from how libI77 prints,
# there being no other Fortran compiler to compare with.  efl must
# translate its own tests as upstream's has them (usr.bin/efl/efltest).
#
# And what f77 -O makes of libU77's ioinit.f must be tools/build/ioinit.s,
# which is what the system is given of that file where it is built
# (tools/build/commands.sh).  When it is not, the system's is put in
# obj/tests/fortran/ioinit.s, for tools/build.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/fortran needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/fortran
. "$top/tests/harness/system.sh"
out=$top/obj/tests/fortran$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
cd "$out"

cp "$proto" root.proto
for f in fortran.sh t1.f t2.f t3.f t3s.f t4.f t5.r t6.f; do
    echo "f /root/$f 644 $here/$f"
done >> root.proto
echo "d /root/efltest 755" >> root.proto
for f in george select matmul; do
    echo "f /root/efltest/$f.e 644 $top/usr/src/usr.bin/efl/efltest/$f.e"
    echo "f /root/efltest/$f.out 644 $top/usr/src/usr.bin/efl/efltest/$f.out"
done >> root.proto
echo "f /root/ioinit.f 644 $top/usr/src/usr.lib/libU77/ioinit.f" >> root.proto
echo "f /root/ioinit.ref 644 $top/tools/build/ioinit.s" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" fortran.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-1800} python3 -I "$top/tests/harness/runtest.py" \
    --hd fortran.img,$geometry,xtide \
    --type "login: " root \
    --type "# " 'sh /root/fortran.sh; echo "@ex""it 00"' \
    --boot fortran$case

# What the system wrote from the first part to the last.
tr -d '\r' < serial.txt | sed -n '/^== control$/,/^== end$/p' > got
rm -f ioinit.s
if grep -q '^== ioinit.s$' got; then
    sed -n '/^== ioinit.s$/,/^== ioinit.s end$/p' got | sed '1d;$d' |
        (cd "$out" && uudecode)
    echo "fortran: f77 makes ioinit.f otherwise than tools/build/ioinit.s;" \
        "the system's is $out/ioinit.s" >&2
    exit 1
fi
if ! diff "$here/expected" got; then
    echo "fortran: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "fortran: f77, ratfor, struct and efl, as written"
