"""Say how much of the swap partition of the test's disk was written.

usage: swapused.py IMAGE

IMAGE is the disk tests/kernel/run.sh made, after the kernel has run from
it.  tools/mkdisk.py left partition b all zeros, so a sector of it that
is not was written by the kernel.  Exits 1 if there is none.
"""
import struct
import sys

img = open(sys.argv[1], "rb").read()
start = 0                       # of the system's part of the disk
if img[510:512] == b"\x55\xaa" and img[446 + 4] == 0xB9:
    start = struct.unpack_from("<L", img, 446 + 8)[0]
size, offset = struct.unpack_from("<LL", img, (start + 1) * 512 + 120 + 12)
swap = img[(start + offset) * 512:(start + offset + size) * 512]
used = sum(any(swap[k:k + 512]) for k in range(0, len(swap), 512))
if not used:
    sys.exit("nothing was written to the swap area")
print("%s: %d of the swap area's %d sectors were written"
      % (sys.argv[1].rsplit("/", 1)[-1], used, size))
