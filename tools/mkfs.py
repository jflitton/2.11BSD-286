#!/usr/bin/env python3
"""Make a file system image for the 286 kernel.

usage: mkfs.py [-s BLOCKS] [-i INODES] PROTO IMAGE

The image is a 2.11BSD file system (usr/src/sys/h/fs.h, inode.h, dir.h)
with every number little-endian, which is how this kernel reads and
writes one: 1K blocks, the super block in block 1, the inodes from block
2, and after them the files.  BLOCKS is its size, by default the smallest
that holds everything with a quarter over; INODES by default is enough for
the files with as many again.

PROTO lists what goes in it, a line for each thing, in any order that
names a directory before what is in it.  Blank lines and lines beginning
with # are ignored.  Modes are octal.

    d PATH MODE                     a directory
    f PATH MODE FILE                a file, with the contents of FILE
    c PATH MODE MAJOR MINOR         a character device
    b PATH MODE MAJOR MINOR         a block device
    l PATH TARGET                   a symbolic link
    h PATH OTHER                    another name for OTHER, a file

FILE is relative to the directory PROTO is in.  The root directory exists
already.  Everything is owned by root, and is in group 0 unless MODE is
followed by a colon and the number of another: 2755:2 is a program that
runs with group kmem.
"""
import os
import struct
import sys
import time

BSIZE = 1024
INOPB = 16
NICFREE = 50
NDADDR = 4
NINDIR = BSIZE // 4
DIRBLKSIZ = 512
ROOTINO = 2
MNT_CLEAN = 0x2000

IFDIR, IFCHR, IFBLK, IFREG, IFLNK = 0o40000, 0o20000, 0o60000, 0o100000, 0o120000


class Inode:
    def __init__(self, mode):
        self.mode = mode
        self.nlink = 0
        self.data = b""         # contents, for a file, link or directory
        self.rdev = 0
        self.gid = 0
        self.entries = None     # a directory's: [(name, inode number)]


def dirdata(entries):
    """A directory's contents: entries packed into 512-byte blocks."""
    out = bytearray()
    block = []                  # (ino, name) in the current block
    used = 0

    def flush():
        nonlocal block, used
        if not block:
            return
        b = bytearray()
        for k, (ino, name) in enumerate(block):
            size = (6 + len(name) + 1 + 3) & ~3
            reclen = size if k < len(block) - 1 else DIRBLKSIZ - len(b)
            b += struct.pack("<HHH", ino, reclen, len(name)) + name
            b += b"\0" * (size - 6 - len(name))
        out.extend(b.ljust(DIRBLKSIZ, b"\0"))
        block, used = [], 0

    for name, ino in entries:
        name = name.encode()
        size = (6 + len(name) + 1 + 3) & ~3
        if used + size > DIRBLKSIZ:
            flush()
        block.append((ino, name))
        used += size
    flush()
    return bytes(out)


def main():
    args = sys.argv[1:]
    nblocks = ninodes = None
    while args and args[0] in ("-s", "-i"):
        if args[0] == "-s":
            nblocks = int(args[1])
        else:
            ninodes = int(args[1])
        del args[:2]
    if len(args) != 2:
        sys.exit(__doc__)
    proto, image = args
    here = os.path.dirname(os.path.abspath(proto))

    inodes = {ROOTINO: Inode(IFDIR | 0o755)}
    inodes[ROOTINO].entries = [(".", ROOTINO), ("..", ROOTINO)]
    inodes[ROOTINO].nlink = 2
    paths = {"/": ROOTINO}

    def add(path, ip, number=None):
        parent, name = os.path.split(path.rstrip("/"))
        if parent not in paths or inodes[paths[parent]].entries is None:
            sys.exit("%s: %s: no directory %s" % (proto, path, parent))
        if not name or len(name) > 63 or path in paths:
            sys.exit("%s: %s: bad or repeated name" % (proto, path))
        if number is None:
            number = max(inodes) + 1
            inodes[number] = ip
        paths[path] = number
        ip.nlink += 1
        dp = inodes[paths[parent]]
        dp.entries.append((name, number))
        if ip.entries is not None:
            ip.entries = [(".", number), ("..", paths[parent])]
            ip.nlink = 2
            dp.nlink += 1

    def made(kind, mode):
        """An inode of KIND with MODE, which may give a group."""
        mode, _, gid = mode.partition(":")
        ip = Inode(kind | int(mode, 8))
        ip.gid = int(gid or 0)
        return ip

    for n, line in enumerate(open(proto), 1):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        try:
            kind, path = w[0], w[1]
            if kind == "d":
                ip = made(IFDIR, w[2])
                ip.entries = []
            elif kind == "f":
                ip = made(IFREG, w[2])
                ip.data = open(os.path.join(here, w[3]), "rb").read()
            elif kind in "cb":
                ip = made(IFCHR if kind == "c" else IFBLK, w[2])
                ip.rdev = int(w[3]) << 8 | int(w[4])
            elif kind == "l":
                ip = Inode(IFLNK | 0o777)
                ip.data = w[2].encode()
            elif kind == "h":
                number = paths.get(w[2])
                if number is None or inodes[number].entries is not None:
                    sys.exit("%s:%d: no file %s" % (proto, n, w[2]))
                add(path, inodes[number], number)
                continue
            else:
                raise ValueError
        except (IndexError, ValueError):
            sys.exit("%s:%d: cannot make sense of this line" % (proto, n))
        add(path, ip)
    for ip in inodes.values():
        if ip.entries is not None:
            ip.data = dirdata(ip.entries)

    def blocks_of(size):
        n = (size + BSIZE - 1) // BSIZE
        if n > NDADDR:
            n += 1 + (max(0, n - NDADDR - NINDIR) + NINDIR - 1) // NINDIR
            if n - 1 > NDADDR + NINDIR:
                n += 1
        return n

    if ninodes is None:
        ninodes = 2 * len(inodes) + 16
    ninodes = (ninodes + INOPB - 1) // INOPB * INOPB
    isize = 2 + ninodes // INOPB                # first block after the inodes
    need = isize + sum(blocks_of(len(ip.data)) for ip in inodes.values())
    if nblocks is None:
        nblocks = need + need // 4 + 8
    if nblocks < need or max(inodes) > ninodes:
        sys.exit("%s: needs %d blocks and %d inodes" % (image, need, max(inodes)))

    img = bytearray(nblocks * BSIZE)
    nextblk = isize

    def alloc(data=b""):
        nonlocal nextblk
        b = nextblk
        nextblk += 1
        img[b * BSIZE:b * BSIZE + len(data)] = data
        return b

    now = int(time.time())
    for number, ip in inodes.items():
        addr = [0] * 7
        if ip.mode & 0o170000 in (IFCHR, IFBLK):
            addr[0] = ip.rdev << 16
        else:
            data = ip.data
            nb = (len(data) + BSIZE - 1) // BSIZE
            blks = [alloc(data[k * BSIZE:(k + 1) * BSIZE]) for k in range(nb)]
            addr[:NDADDR] = (blks[:NDADDR] + [0] * NDADDR)[:NDADDR]
            rest = blks[NDADDR:]
            if rest:
                addr[4] = alloc(struct.pack("<%dL" % len(rest[:NINDIR]),
                                            *rest[:NINDIR]))
                rest = rest[NINDIR:]
            if rest:
                if len(rest) > NINDIR * NINDIR:
                    sys.exit("a file is too big for this tool")
                singles = []
                for k in range(0, len(rest), NINDIR):
                    part = rest[k:k + NINDIR]
                    singles.append(alloc(struct.pack("<%dL" % len(part), *part)))
                addr[5] = alloc(struct.pack("<%dL" % len(singles), *singles))
        d = struct.pack("<HHHHl7L5HH3l", ip.mode, ip.nlink, 0, ip.gid,
                        len(ip.data),
                        *addr, 0, 0, 0, 0, 0, 0, now, now, now)
        assert len(d) == 64
        off = (number + 2 * INOPB - 1) * 64     # itod and itoo
        img[off:off + 64] = d

    # The free list: the super block holds up to NICFREE block numbers,
    # and the first of them is a block holding the next NICFREE.
    nfree, free = 0, [0] * NICFREE

    def bfree(b):
        nonlocal nfree, free
        if nfree >= NICFREE:
            img[b * BSIZE:b * BSIZE + 2 + 4 * NICFREE] = struct.pack(
                "<h%dl" % NICFREE, nfree, *free)
            nfree = 0
        free[nfree] = b
        nfree += 1

    bfree(0)                    # the end of the list
    for b in range(nblocks - 1, nextblk - 1, -1):
        bfree(b)
    tfree = nblocks - nextblk
    sb = struct.pack("<Hlh%dlh100H4bllHhh12sHHH" % NICFREE,
                     isize, nblocks, nfree, *free,
                     0, *([0] * 100),           # no inodes cached
                     0, 0, 0, 0,                # locks and flags
                     now, tfree, ninodes - len(inodes) - 1,
                     1, 1,                      # step and cyl
                     b"/", 1, 0, MNT_CLEAN)
    img[BSIZE:BSIZE + len(sb)] = sb
    with open(image, "wb") as f:
        f.write(img)
    print("%s: %d blocks, %d of them free; %d inodes, %d used"
          % (os.path.basename(image), nblocks, tfree, ninodes, len(inodes)))


if __name__ == "__main__":
    main()
