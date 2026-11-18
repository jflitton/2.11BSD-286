#!/usr/bin/env python3
"""Make a hard disk image with a 2.11BSD disk label.

usage: mkdisk.py [-g CYLINDERS,HEADS,SECTORS] [-p] [-o SECTOR] [-t TYPE]
                 [-b BOOTBLOCK] [-m FIRSTSECTOR] IMAGE PARTITION=WHAT ...

The image is a whole disk of the geometry given, by default 615,4,17: the
20 MB drive of many a PC/AT.  With -p its first sector is a PC partition
table with one entry, which gives the system everything from the second
track on, or with -o from that sector on, as if another system had what
comes before; without -p the whole disk is the system's.  Either way the
label is the second sector of the system's part, and its partitions are
counted from the start of that part (usr/src/sys/h/disklabel.h,
usr/src/sys/i286at/pcdisk.h).

TYPE is what the label says the disk is, "ide", the default, or "scsi".
/boot goes by it to tell the kernel which driver has its root.

To boot from the disk, -b puts BOOTBLOCK in the first sector of the
system's part (usr/src/sys/i286stand/hdboot.s, assembled), which loads the
file /boot of partition a; and on a disk with a partition table, -m gives
the disk's first sector the program in FIRSTSECTOR (mbr.s), which loads
that.  Both must be within the first 1024 cylinders, with /boot and the
kernel: the BIOS reads no further.

Only what is put on the disk is written to the image, so a big disk that
is mostly empty takes little room where the host's file system allows.

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
DTYPE = {"ide": 5, "scsi": 4}       # DTYPE_ESDI, DTYPE_SCSI
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
    first = None
    dtype = "ide"
    sector = {"-b": None, "-m": None}
    while args and args[0] in ("-g", "-p", "-o", "-t", "-b", "-m"):
        if args[0] == "-p":
            pc = True
            del args[:1]
        elif args[0] == "-t":
            if len(args) < 2 or args[1] not in DTYPE:
                sys.exit(__doc__)
            dtype = args[1]
            del args[:2]
        elif args[0] in sector:
            if len(args) < 2:
                sys.exit(__doc__)
            sector[args[0]] = open(args[1], "rb").read()
            if len(sector[args[0]]) != 512 or sector[args[0]][510:] != b"\x55\xaa" \
                    or any(sector[args[0]][446:510]):
                sys.exit("%s: not a boot sector with the table's place empty"
                         % args[1])
            del args[:2]
        elif args[0] == "-o":
            if len(args) < 2 or not args[1].isdigit():
                sys.exit(__doc__)
            first = int(args[1])
            del args[:2]
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
    if first is not None and not pc:
        sys.exit("-o needs -p")
    if sector["-m"] and not pc:
        sys.exit("-m needs -p: a disk that is all the system's starts with "
                 "the boot block")
    start = (sectors if first is None else first) if pc else 0
    size = total - start
    if size < 16:
        sys.exit("no room for the system on a disk of %d sectors" % total)
    pieces = {}                 # sector on the disk: what is written there

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
            pieces[start + where] = data
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
    label = struct.pack("<LBB16s16s5HL3HH4BHH5L5LL", DISKMAGIC, DTYPE[dtype], 0,
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
    a = bytearray(pieces.get(start, bytes(1024)))
    if any(a[512:1024]):
        sys.exit("partition a has something in its second sector, where the label goes")
    a[512:512 + len(label)] = label
    if sector["-b"]:
        if any(a[:512]):
            sys.exit("partition a has something in its first sector, where "
                     "the boot block goes")
        a[:512] = sector["-b"]
    pieces[start] = bytes(a)

    if pc:
        entry = (b"\x80" + chs(start, heads, sectors) + bytes([PCP_2BSD])
                 + chs(total - 1, heads, sectors) + struct.pack("<LL", start, size))
        pieces[0] = ((sector["-m"] or bytes(512))[:446] + entry + bytes(48)
                     + b"\x55\xaa")
    with open(image, "wb") as f:
        f.truncate(total * 512)
        for sector, data in pieces.items():
            f.seek(sector * 512)
            f.write(data)
    for letter in sorted(parts):
        psize, offset, fstype = parts[letter]
        print("%s: %s: %7d sectors from %7d  %s" % (
            os.path.basename(image), letter, psize, offset,
            {FS_SWAP: "swap", FS_V71K: "file system"}.get(fstype, "")))
    print("%d,%d,%d" % (cyls, heads, sectors))


if __name__ == "__main__":
    main()
