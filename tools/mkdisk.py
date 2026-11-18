#!/usr/bin/env python3
"""Make a hard disk image with a 2.11BSD disk label.

usage: mkdisk.py [-g CYLINDERS,HEADS,SECTORS] [-p] IMAGE PARTITION=WHAT ...

The image is a whole disk of the geometry given, by default 615,4,17: the
20 MB drive of many a PC/AT.  With -p its first sector is a PC partition
table with one entry, which gives the system everything from the second
track on; without, the whole disk is the system's.  Either way the label
is the second sector of the system's part, and its partitions are counted
from the start of that part (usr/src/sys/h/disklabel.h,
usr/src/sys/i286/pcpart.h).

Each PARTITION is a letter from a to h, and WHAT is one of

    FILE            a file system image, as tools/mkfs.py makes
    swap:SECTORS    a swap area
    SECTORS         that much room and nothing in it

They are laid out in the order given.  The first must be a: the label is
kept in the file system's unused second sector.  Partition c, if not
given, is made the whole of the system's part, as is the custom.

The last line printed is the geometry, as tests/harness/runtest.py --hd
wants it.
"""
import os
import struct
import sys

DISKMAGIC = 0x82564557
DTYPE_ESDI = 5
FS_UNUSED, FS_SWAP, FS_V71K = 0, 1, 5
PCP_2BSD = 0xB9
MAXPARTITIONS = 8


def chs(lba, heads, sectors):
    """A sector as a partition table names it: head, sector with the
    cylinder's top two bits, cylinder's low eight."""
    cyl, rest = divmod(lba, heads * sectors)
    cyl = min(cyl, 1023)
    return bytes([rest // sectors, rest % sectors + 1 | cyl >> 8 << 6, cyl & 255])


def main():
    args = sys.argv[1:]
    cyls, heads, sectors = 615, 4, 17
    pc = False
    while args and args[0] in ("-g", "-p"):
        if args[0] == "-p":
            pc = True
            del args[:1]
        else:
            try:
                cyls, heads, sectors = (int(n) for n in args[1].split(","))
            except (IndexError, ValueError):
                sys.exit(__doc__)
            del args[:2]
    if len(args) < 2:
        sys.exit(__doc__)
    image = args[0]
    total = cyls * heads * sectors
    start = sectors if pc else 0
    size = total - start
    img = bytearray(total * 512)

    parts = {}                  # letter: (size, offset, fstype)
    where = 0
    for n, arg in enumerate(args[1:]):
        letter, _, what = arg.partition("=")
        if len(letter) != 1 or letter not in "abcdefgh" or letter in parts or not what:
            sys.exit("%s: cannot make sense of this" % arg)
        if n == 0 and letter != "a":
            sys.exit("the first partition must be a")
        if what.startswith("swap:") and what[5:].isdigit():
            psize, fstype = int(what[5:]), FS_SWAP
        elif what.isdigit():
            psize, fstype = int(what), FS_UNUSED
        else:
            data = open(what, "rb").read()
            data += b"\0" * (-len(data) % 512)
            psize, fstype = len(data) // 512, FS_V71K
            at = (start + where) * 512
            img[at:at + len(data)] = data
        if where + psize > size:
            sys.exit("%s: no room: the disk has %d sectors for the system"
                     % (arg, size))
        parts[letter] = (psize, where, fstype)
        where += psize
    parts.setdefault("c", (size, 0, FS_UNUSED))

    npart = ord(max(parts)) - ord("a") + 1
    table = b""
    for k in range(MAXPARTITIONS):
        psize, offset, fstype = parts.get(chr(ord("a") + k), (0, 0, FS_UNUSED))
        fs = fstype == FS_V71K
        table += struct.pack("<LLHBB", psize, offset, 1024 if fs else 0,
                             fstype, 1 if fs else 0)
    label = struct.pack("<LBB16s16s5HL3HH4BHH5L5LL", DISKMAGIC, DTYPE_ESDI, 0,
                        b"image", b"",
                        512, sectors, heads, cyls, heads * sectors, size,
                        0, 0, 0,                # spares, alternate cylinders
                        3600, 1, 0, 0, 0,       # rpm, interleave, skews
                        0, 0,                   # seek time, flags
                        *([0] * 10), DISKMAGIC)
    rest = struct.pack("<HHH", npart, 512, 1024)        # then the partitions
    summed = label + b"\0\0" + rest + table[:npart * 12]
    check = 0
    for (w,) in struct.iter_unpack("<H", summed):
        check ^= w
    label += struct.pack("<H", check) + rest + table
    assert len(label) == 216
    at = (start + 1) * 512
    if any(img[at:at + 512]):
        sys.exit("partition a has something in its second sector, where the label goes")
    img[at:at + len(label)] = label

    if pc:
        entry = (b"\x80" + chs(start, heads, sectors) + bytes([PCP_2BSD])
                 + chs(total - 1, heads, sectors) + struct.pack("<LL", start, size))
        img[446:446 + 16] = entry
        img[510:512] = b"\x55\xaa"
    with open(image, "wb") as f:
        f.write(img)
    for letter in sorted(parts):
        psize, offset, fstype = parts[letter]
        print("%s: %s: %7d sectors from %7d  %s" % (
            os.path.basename(image), letter, psize, offset,
            {FS_SWAP: "swap", FS_V71K: "file system"}.get(fstype, "")))
    print("%d,%d,%d" % (cyls, heads, sectors))


if __name__ == "__main__":
    main()
