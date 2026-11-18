#!/usr/bin/env python3
"""Check a 2.11BSD file system in an image, or print a file from it.

usage: fsck.py [-p LETTER] IMAGE
       fsck.py [-p LETTER] -x PATH IMAGE

IMAGE is a file system as this kernel keeps one (tools/mkfs.py), or a
whole disk (tools/mkdisk.py), in which case the file system is the one in
partition LETTER of its label, by default a.

The first form reads everything and changes nothing.  It checks that
every block belongs to exactly one file or to the free list, that every
file in use is in as many directories as its link count says, that the
directories are well formed, and that the super block's counts are right.
It prints what is wrong and exits 1, or prints a line of totals and exits
0.  It is for testing the kernel: it wants a file system that was
synchronized and then left alone, and repairs nothing.

The second form writes the contents of the file PATH to standard output.
"""
import struct
import sys

BSIZE = 1024
INOPB = 16
NICFREE = 50
NICINOD = 100
NDADDR = 4
NINDIR = BSIZE // 4
DIRBLKSIZ = 512
ROOTINO = 2
DISKMAGIC = 0x82564557
PCP_2BSD = 0xB9
IFMT, IFDIR, IFCHR, IFBLK, IFREG, IFLNK = (
    0o170000, 0o40000, 0o20000, 0o60000, 0o100000, 0o120000)
SB = "<Hlh%dlh%dH4bllHhh12sHHH" % (NICFREE, NICINOD)


def partition(path, letter):
    """The file system's bytes: all of the image, or a partition of its
    label.  No more of the image is read than that."""
    with open(path, "rb") as f:
        def sectors(first, count):
            f.seek(first * 512)
            return f.read(count * 512)

        start = 0
        boot = sectors(0, 1)
        if boot[510:512] == b"\x55\xaa":
            for k in range(4):
                e = boot[446 + 16 * k:462 + 16 * k]
                if e[4] == PCP_2BSD:
                    start = struct.unpack_from("<L", e, 8)[0]
                    break
        label = sectors(start + 1, 1)
        if len(label) < 512 or struct.unpack_from("<L", label)[0] != DISKMAGIC:
            f.seek(0)
            return f.read()
        npart = struct.unpack_from("<H", label, 114)[0]
        k = ord(letter) - ord("a")
        if k >= npart:
            sys.exit("the label has no partition %s" % letter)
        size, offset = struct.unpack_from("<LL", label, 120 + 12 * k)
        return sectors(start + offset, size)


class Fs:
    def __init__(self, img):
        self.img = img
        f = struct.unpack_from(SB, img, BSIZE)
        self.isize, self.fsize, self.nfree = f[0:3]
        self.free = f[3:3 + NICFREE]
        self.ninode = f[3 + NICFREE]
        self.inodes = f[4 + NICFREE:4 + NICFREE + NICINOD]
        self.tfree, self.tinode = f[-8], f[-7]
        self.flags = f[-1]
        self.problems = []
        self.owner = {}         # block: the inode that has it, or "free"
        if not (2 < self.isize < self.fsize <= len(img) // BSIZE):
            sys.exit("not a file system: %d blocks of inodes, %d blocks, in %d"
                     % (self.isize - 2, self.fsize, len(img) // BSIZE))
        self.maxino = (self.isize - 2) * INOPB

    def bad(self, what):
        self.problems.append(what)

    def block(self, b):
        return self.img[b * BSIZE:(b + 1) * BSIZE]

    def inode(self, n):
        off = (n + 2 * INOPB - 1) * 64
        f = struct.unpack_from("<HHHHl7L5HH3l", self.img, off)
        return {"mode": f[0], "nlink": f[1], "size": f[4], "addr": f[5:12]}

    def claim(self, b, who):
        """Block B belongs to WHO; False if it cannot."""
        if not self.isize <= b < self.fsize:
            self.bad("%s has block %d, which is outside the file system" % (who, b))
            return False
        if b in self.owner:
            self.bad("block %d belongs to %s and to %s" % (b, self.owner[b], who))
            return False
        self.owner[b] = who
        return True

    def blocks(self, ino, ip, claim=True):
        """The data blocks of a file in order, 0 for a hole."""
        who = "inode %d" % ino
        nb = (ip["size"] + BSIZE - 1) // BSIZE
        out = []

        def indirect(b, depth):
            if b == 0:
                out.extend([0] * NINDIR ** depth)
                return
            if claim and not self.claim(b, who):
                return
            for a in struct.unpack("<%dL" % NINDIR, self.block(b)):
                if depth > 1:
                    indirect(a, depth - 1)
                else:
                    out.append(a)

        out.extend(ip["addr"][:NDADDR])
        for depth in (1, 2, 3):
            if len(out) >= nb and not ip["addr"][NDADDR + depth - 1]:
                continue
            indirect(ip["addr"][NDADDR + depth - 1], depth)
        for k, b in enumerate(out):
            if b and k >= nb and claim:
                self.bad("%s has block %d past its end" % (who, b))
            if b and claim:
                self.claim(b, who)
        return out[:nb]

    def read(self, ino, ip, claim=False):
        data = b"".join(self.block(b) if b else bytes(BSIZE)
                        for b in self.blocks(ino, ip, claim))
        return data[:ip["size"]]

    def entries(self, ino, data):
        """A directory's (name, inode number) pairs."""
        out = []
        if len(data) % DIRBLKSIZ:
            self.bad("directory %d: its size is not a multiple of %d" % (ino, DIRBLKSIZ))
        for base in range(0, len(data) - DIRBLKSIZ + 1, DIRBLKSIZ):
            off = base
            while off < base + DIRBLKSIZ:
                n, reclen, namlen = struct.unpack_from("<HHH", data, off)
                if reclen < 8 or reclen % 4 or off + reclen > base + DIRBLKSIZ or \
                        (n and 6 + namlen + 1 > reclen):
                    self.bad("directory %d: bad entry at byte %d" % (ino, off))
                    break
                if n:
                    out.append((data[off + 6:off + 6 + namlen].decode("latin-1"), n))
                off += reclen
        return out

    def check(self):
        inodes = {}
        for n in range(1, self.maxino + 1):
            ip = self.inode(n)
            if ip["mode"]:
                inodes[n] = ip
        links = dict.fromkeys(inodes, 0)
        ndirs = 0
        for n, ip in inodes.items():
            kind = ip["mode"] & IFMT
            if kind in (IFCHR, IFBLK):
                continue
            if kind not in (IFDIR, IFREG, IFLNK):
                self.bad("inode %d: mode %o is no kind of file" % (n, ip["mode"]))
                continue
            data = self.read(n, ip, claim=True)
            if kind != IFDIR:
                continue
            ndirs += 1
            names = self.entries(n, data)
            if len(names) < 2 or names[0] != (".", n) or names[1][0] != "..":
                self.bad("directory %d does not start with . and .." % n)
            for name, m in names:
                if m not in inodes:
                    self.bad("directory %d: %s is inode %d, which is not in use"
                             % (n, name, m))
                else:
                    links[m] += 1
        if ROOTINO not in inodes or inodes[ROOTINO]["mode"] & IFMT != IFDIR:
            self.bad("there is no root directory")
        for n, ip in inodes.items():
            if n == 1:
                continue        # never used, by old custom
            if links[n] != ip["nlink"]:
                self.bad("inode %d: link count %d, but it is in directories %d times"
                         % (n, ip["nlink"], links[n]))

        # The free list.
        nfree = 0
        count, lst = self.nfree, self.free
        seen = set()
        while True:
            if not 0 <= count <= NICFREE:
                self.bad("a free list block has a count of %d" % count)
                break
            if count == 0:
                break
            for b in lst[1:count]:
                nfree += self.claim(b, "the free list")
            if lst[0] == 0:
                break
            if lst[0] in seen or not self.claim(lst[0], "the free list"):
                break
            seen.add(lst[0])
            nfree += 1
            f = struct.unpack("<h%dl" % NICFREE, self.block(lst[0])[:2 + 4 * NICFREE])
            count, lst = f[0], f[1:]
        missing = [b for b in range(self.isize, self.fsize) if b not in self.owner]
        if missing:
            self.bad("%d blocks are in no file and not free: %s%s"
                     % (len(missing), ", ".join(map(str, missing[:8])),
                        " ..." if len(missing) > 8 else ""))
        if nfree != self.tfree:
            self.bad("the super block counts %d free blocks; there are %d"
                     % (self.tfree, nfree))
        ifree = self.maxino - len(inodes) - (1 not in inodes)
        if ifree != self.tinode:
            self.bad("the super block counts %d free inodes; there are %d"
                     % (self.tinode, ifree))
        for n in self.inodes[:max(0, min(self.ninode, NICINOD))]:
            if n in inodes:
                self.bad("the super block lists inode %d as free; it is in use" % n)
        return len(inodes), ndirs, nfree

    def lookup(self, path):
        n = ROOTINO
        for name in [p for p in path.split("/") if p]:
            ip = self.inode(n)
            if ip["mode"] & IFMT != IFDIR:
                return None
            n = dict(self.entries(n, self.read(n, ip))).get(name)
            if n is None:
                return None
        return n


def main():
    args = sys.argv[1:]
    letter, path = "a", None
    while len(args) > 1 and args[0] in ("-p", "-x"):
        if args[0] == "-p":
            letter = args[1]
        else:
            path = args[1]
        del args[:2]
    if len(args) != 1 or len(letter) != 1 or letter not in "abcdefgh":
        sys.exit(__doc__)
    fs = Fs(partition(args[0], letter))
    if path is not None:
        n = fs.lookup(path)
        if n is None:
            sys.exit("%s: not in the file system" % path)
        sys.stdout.buffer.write(fs.read(n, fs.inode(n)))
        return
    files, dirs, free = fs.check()
    for p in fs.problems[:40]:
        print(p)
    if fs.problems:
        sys.exit("%s: %d things wrong" % (args[0], len(fs.problems)))
    print("%s: %d files, %d of them directories; %d blocks, %d free; nothing wrong"
          % (args[0].rsplit("/", 1)[-1], files, dirs, fs.fsize, free))


if __name__ == "__main__":
    main()
