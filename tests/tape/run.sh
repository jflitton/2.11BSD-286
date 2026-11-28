#!/bin/sh
# Test the SCSI tape driver, st(4), and the programs that use a tape:
# boot the system from a SCSI disk on the Adaptec, with two tape drives
# of 86Box's on the same adapter (tests/harness/runtest.py --tape): st0
# an Archive Viper 150, a QIC drive whose records are blocks of 512
# bytes, with a tape that has nothing on it; st1 an Archive Python, a
# DAT drive whose records are of any length, with a tape written here, a tar
# archive and four records of their own sizes.  By tape.sh the system
# reads the host's tape with tcopy, tar and dd; writes two archives on
# the QIC tape with tar and finds each again with mt; moves over blocks
# and reads each file to its mark; dumps a file system to the QIC tape with
# dump and restores it onto another with restor, while the disk and
# the tape both have commands out on the adapter; writes the DAT tape
# with ansitape and reads it back, and writes it again with tar; and
# takes the QIC tape out.  What the system said must be
# as `expected` has it, and the host must read back what it wrote on
# the DAT tape.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/tape needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/tape
. "$top/tests/harness/system.sh"
out=$top/obj/tests/tape$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
fi
cd "$out"

# The root the build made, on the SCSI disk, with the script.
sed "s,^/dev/wd0,/dev/sd0," "$top/usr/src/etc/fstab" > fstab
sed "s,^f /etc/fstab .*,f /etc/fstab 644 $out/fstab," "$proto" > root.proto
echo "f /root/tape.sh 644 $here/tape.sh" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

# A file system to dump: the kernel's headers and the PC's drivers.
{
    echo "d /h 755"
    for f in "$top"/usr/src/sys/h/*.h; do
        echo "f /h/$(basename "$f") 644 $f"
    done
    echo "d /i286at 755"
    for f in "$top"/usr/src/sys/i286at/*; do
        echo "f /i286at/$(basename "$f") 644 $f"
    done
} > d.proto
python3 -I "$top/tools/mkfs.py" d.proto d.fs >/dev/null

# The tapes: the QIC drive's with nothing on it, and the DAT drive's
# with a tar archive in records of 10K, as tar writes it, and records
# of 100, 3000, 512 and 7 bytes, each file ending at a tape mark.
: > st0.tap
python3 -I - st1.tap <<'EOF'
import io, struct, sys, tarfile
def rec(f, b):
    f.write(struct.pack("<I", len(b)) + b + b"\0" * (len(b) & 1)
            + struct.pack("<I", len(b)))
a = io.BytesIO()
with tarfile.open(fileobj=a, mode="w", format=tarfile.USTAR_FORMAT) as t:
    for name, text in (("hello", b"hello from the host\n"),
                       ("bye", b"and goodbye\n")):
        i = tarfile.TarInfo(name)
        i.size, i.mode, i.mtime = len(text), 0o644, 0x60000000
        t.addfile(i, io.BytesIO(text))
a = a.getvalue()
with open(sys.argv[1], "wb") as f:
    for i in range(0, len(a), 10240):
        rec(f, a[i:i + 10240])
    f.write(b"\0\0\0\0")
    for n in (100, 3000, 512, 7):
        rec(f, bytes(48 + (i + n) % 10 for i in range(n)))
    f.write(b"\0\0\0\0")
EOF

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -t scsi -g 60,64,32 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" tape.img \
    a=root.fs b=swap:4000 d=d.fs e=fs:8000 | tail -1)
TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
    --hd tape.img,$geometry,scsi:330:11:5:0:DC000 \
    --tape st0.tap,viper,2 --tape st1.tap,python,3 \
    --type "login: " root \
    --type "# " 'sh /root/tape.sh; echo "@ex""it 00"' \
    --boot tape$case

for w in "st0: ARCHIVE VIPER 150 21247 2.10, id 2" "st1: ARCHIVE Python 04687-XXX 4.CM, id 3"; do
    if ! grep -q "^$w" serial.txt; then
        echo "tape: the kernel did not say \"$w\"" >&2
        exit 1
    fi
done
tr -d '\r' < serial.txt | sed -n '/^== status$/,/^== end$/p' > got
if ! diff "$here/expected" got; then
    echo "tape: what the system said is not as $here/expected has it" >&2
    exit 1
fi

# The DAT tape as the system left it: one tar archive, in records of
# 10K, of lines and more as tape.sh made them.
rm -rf untap
python3 -I "$top/tools/untap.py" st1.tap untap > untap.txt
if [ "$(cat untap.txt)" != "file0: 4 records, 40960 bytes, record sizes [10240]" ]; then
    cat untap.txt >&2
    echo "tape: the DAT tape is not one archive in four records of 10K" >&2
    exit 1
fi
rm -rf x
mkdir x
tar xf untap/file0 -C x
awk 'BEGIN { for (i = 1; i <= 3000; i++) print "line", i }' > lines
awk 'BEGIN { for (i = 1; i <= 500; i++) print "more", i }' > more
if ! cmp -s lines x/lines || ! cmp -s more x/more; then
    echo "tape: the archive on the DAT tape is not of what was written" >&2
    exit 1
fi
echo "tape: read, written, moved over and taken out, on QIC and DAT"
