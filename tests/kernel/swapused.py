"""Say how much of the swap partition of the test's disk was written.

usage: swapused.py IMAGE

IMAGE is the disk tests/kernel/run.sh made, after the kernel has run from
it.  tools/mkdisk.py left partition b all zeros, so a sector of it that
is not was written by the kernel.  Exits 1 if there is none.
"""
import struct
import sys

with open(sys.argv[1], "rb") as f:
    def sectors(first, count):
        f.seek(first * 512)
        return f.read(count * 512)

    start = 0                   # of the system's part of the disk
    boot = sectors(0, 1)
    if boot[510:512] == b"\x55\xaa" and boot[446 + 4] == 0xB9:
        start = struct.unpack_from("<L", boot, 446 + 8)[0]
    size, offset = struct.unpack_from("<LL", sectors(start + 1, 1), 120 + 12)
    swap = sectors(start + offset, size)
used = sum(any(swap[k:k + 512]) for k in range(0, len(swap), 512))
if not used:
    sys.exit("nothing was written to the swap area")
print("%s: %d of the swap area's %d sectors were written, from sector %d of the disk"
      % (sys.argv[1].rsplit("/", 1)[-1], used, size, start + offset))
