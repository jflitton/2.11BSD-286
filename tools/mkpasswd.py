#!/usr/bin/env python3
"""Write /etc/passwd and its database from master.passwd, as mkpasswd(8) does.

usage: mkpasswd.py MASTER.PASSWD DIRECTORY

2.11BSD's getpwnam(), getpwuid() and getpwent() look users up in an ndbm
database, /etc/passwd.dir and /etc/passwd.pag, and read a password from
/etc/master.passwd only for root (lib/libc/gen/getpwent.c).  Without the
database they read master.passwd itself, which only root may, so that
nobody else can find out whose a file is.  Upstream's etc/Makefile runs
`mkpasswd -p master.passwd` to make the database and /etc/passwd; this
makes in DIRECTORY the same three files, passwd, passwd.dir and
passwd.pag, byte for byte as sbin/mkpasswd would on the 286, which
tests/system checks.

Each line of MASTER.PASSWD is a user, stored twice: keyed by name and by
uid, a 16-bit int.  The record is the name, the offset in master.passwd
of the password, in decimal, the uid and gid, the time the password
changes, the class, gecos, home directory and shell, the time the
account expires, and a byte saying which key the record is under.
Integers are as the 286 has them, little-endian, int 16 bits and long 32.

The database is lib/libc/gen/ndbm.c's: the page file is blocks of 1024
bytes, each a table of shorts, the count of items and then the offset
each ends at, with the items packed down from the end of the block, a
key followed by its record; the directory file is a bitmap, with a bit
set for each block that has been split, in blocks of 4096 bytes, the
size of ndbm's buffer where pdp11 is not defined.  A key's hash says
which block it is in.  The blocks are worked on as ndbm does, in a copy
of its buffer, so that what a split or a delete leaves in the unused
part of a block is there too.
"""
import struct
import sys

PBLKSIZ = 1024
DBLKSIZ = 4096
KEYBYNAME, KEYBYUID = b"0", b"1"

HITAB = [61, 57, 53, 49, 45, 41, 37, 33, 29, 25, 21, 17, 13, 9, 5, 1]
HLTAB = [
    0o6100151277, 0o6106161736, 0o6452611562, 0o5001724107,
    0o2614772546, 0o4120731531, 0o4665262210, 0o7347467531,
    0o6735253126, 0o6042345173, 0o3072226605, 0o1464164730,
    0o3247435524, 0o7652510057, 0o1546775256, 0o5714532133,
    0o6173260402, 0o7517101630, 0o2431460343, 0o1743245566,
    0o0261675137, 0o2433103631, 0o3421772437, 0o4447707466,
    0o4435620103, 0o3757017115, 0o3641531772, 0o6767633246,
    0o2673230344, 0o0260612216, 0o4133454451, 0o0615531516,
    0o6137717526, 0o2574116560, 0o2304023373, 0o7061702261,
    0o5153031405, 0o5322056705, 0o7401116734, 0o6552375715,
    0o6165233473, 0o5311063631, 0o1212221723, 0o1052267235,
    0o6000615237, 0o1075222665, 0o6330216006, 0o4402355630,
    0o1451177262, 0o2000133436, 0o6025467062, 0o7121076461,
    0o3123433522, 0o1010635225, 0o1716177066, 0o5161746527,
    0o1736635071, 0o6243505026, 0o3637211610, 0o1756474365,
    0o4723077174, 0o3642763134, 0o5750130273, 0o3655541561,
]


def calchash(item):
    """dcalchash: a char is signed (tools/cc286 -j), and shifts as one."""
    hashl = hashi = 0
    for b in item:
        c = b - 256 if b > 127 else b
        for _ in range(2):
            hashi += HITAB[c & 0o17]
            hashl = (hashl + HLTAB[hashi & 63]) & 0xFFFFFFFF
            c >>= 4
    return hashl


def short(buf, i):
    return struct.unpack_from("<h", buf, 2 * i)[0]


def setshort(buf, i, v):
    struct.pack_into("<h", buf, 2 * i, v)


def makdatum(buf, n):
    if n >= short(buf, 0):
        return None
    t = short(buf, n) if n > 0 else PBLKSIZ
    return bytes(buf[short(buf, n + 1):t])


def finddatum(buf, item):
    n = PBLKSIZ
    i = 0
    while i < short(buf, 0):
        n -= short(buf, i + 1)
        if n == len(item) and buf[short(buf, i + 1):short(buf, i + 1) + n] == item:
            return i
        i += 2
        n = short(buf, i)
    return -1


def delitem(buf, n):
    i2 = short(buf, 0)
    if n >= i2 or n & 1:
        return False
    if n == i2 - 2:
        setshort(buf, 0, i2 - 2)
        return True
    i1 = short(buf, n) if n > 0 else PBLKSIZ
    i1 -= short(buf, n + 2)
    if i1 > 0:
        low = short(buf, i2)
        buf[low + i1:short(buf, n + 2) + i1] = buf[low:short(buf, n + 2)]
    setshort(buf, 0, i2 - 2)
    for k in range(n + 1, short(buf, 0) + 1):
        setshort(buf, k, short(buf, k + 2) + i1)
    return True


def additem(buf, item, item1):
    i2 = short(buf, 0)
    i1 = short(buf, i2) if i2 > 0 else PBLKSIZ
    i1 -= len(item) + len(item1)
    if i1 <= (i2 + 3) * 2:
        return False
    setshort(buf, 0, i2 + 2)
    setshort(buf, i2 + 1, i1 + len(item1))
    buf[i1 + len(item1):i1 + len(item1) + len(item)] = item
    setshort(buf, i2 + 2, i1)
    buf[i1:i1 + len(item1)] = item1
    return True


class Dbm:
    """An ndbm database being made, its two files kept as bytearrays."""

    def __init__(self):
        self.pag = bytearray()
        self.dir = bytearray()
        self.maxbno = -1
        self.pagbno = -1
        self.pagbuf = bytearray(PBLKSIZ)

    def getbit(self):
        if self.bitno > self.maxbno:
            return 0
        bn = self.bitno // 8
        return self.dir[bn] >> (self.bitno % 8) & 1 if bn < len(self.dir) else 0

    def setbit(self):
        self.maxbno = max(self.maxbno, self.bitno)
        bn = self.bitno // 8
        end = (bn // DBLKSIZ + 1) * DBLKSIZ
        if len(self.dir) < end:
            self.dir.extend(bytes(end - len(self.dir)))
        self.dir[bn] |= 1 << self.bitno % 8

    def readpag(self, bno):
        blk = self.pag[bno * PBLKSIZ:(bno + 1) * PBLKSIZ]
        return bytearray(blk) if len(blk) == PBLKSIZ else bytearray(PBLKSIZ)

    def writepag(self, bno, buf):
        end = (bno + 1) * PBLKSIZ
        if len(self.pag) < end:
            self.pag.extend(bytes(end - len(self.pag)))
        self.pag[bno * PBLKSIZ:end] = buf

    def access(self, h):
        self.hmask = 0
        while True:
            self.blkno = h & self.hmask
            self.bitno = self.blkno + self.hmask
            if not self.getbit():
                break
            self.hmask = self.hmask * 2 + 1
        if self.blkno != self.pagbno:
            self.pagbno = self.blkno
            self.pagbuf = self.readpag(self.blkno)

    def store(self, key, dat):
        """dbm_store(..., DBM_INSERT): False if the key was there."""
        while True:
            self.access(calchash(key))
            if finddatum(self.pagbuf, key) >= 0:
                return False
            if additem(self.pagbuf, key, dat):
                self.pagbno = self.blkno
                self.writepag(self.blkno, self.pagbuf)
                return True
            if len(key) + len(dat) + 6 >= PBLKSIZ:
                sys.exit("mkpasswd: dbm_store failed.")
            ovf = bytearray(PBLKSIZ)
            i = 0
            while True:
                item = makdatum(self.pagbuf, i)
                if item is None:
                    break
                if calchash(item) & self.hmask + 1:
                    if not (additem(ovf, item, makdatum(self.pagbuf, i + 1))
                            and delitem(self.pagbuf, i)):
                        sys.exit("mkpasswd: dbm_store failed.")
                    continue
                i += 2
            self.pagbno = self.blkno
            self.writepag(self.blkno, self.pagbuf)
            self.writepag(self.blkno + self.hmask + 1, ovf)
            self.setbit()


def atol(s, bits):
    """atoi or atol: leading blanks, a sign and digits, in so many bits."""
    s = s.lstrip(b" \t\n")
    sign = -1 if s[:1] == b"-" else 1
    s = s[1:] if s[:1] in (b"-", b"+") else s
    digits = len(s) - len(s.lstrip(b"0123456789"))
    v = sign * int(s[:digits] or b"0") & (1 << bits) - 1
    return v - (1 << bits) if v >> bits - 1 else v


def int16(s):
    return atol(s, 16)


def int32(s):
    return atol(s, 32)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    master = open(sys.argv[1], "rb").read()
    db = Dbm()
    orig = b""
    pos = 0
    for raw in master.splitlines(keepends=True):
        offset, pos = pos, pos + len(raw)
        if not raw.endswith(b"\n") or len(raw) > 255:
            continue
        f = raw[:-1].split(b":")
        if len(f) < 7:
            continue
        name, passwd, uid, gid, cls, change, expire = f[:7]
        gecos, home, shell = (f[7:10] + [b""] * 3)[:3]
        offset += len(name) + 1
        uid, gid = int16(uid), int16(gid)
        rec = (name + b"\0" + str(offset).encode() + b"\0" +
               struct.pack("<hhl", uid, gid, int32(change)) +
               cls + b"\0" + gecos + b"\0" + home + b"\0" + shell + b"\0" +
               struct.pack("<l", int32(expire)))
        db.store(name, rec + KEYBYNAME)
        db.store(struct.pack("<h", uid), rec + KEYBYUID)
        orig += b"%s:*:%d:%d:%s:%s:%s\n" % (name, uid, gid, gecos, home, shell)
    for suffix, data in ("", orig), (".dir", db.dir), (".pag", db.pag):
        with open("%s/passwd%s" % (sys.argv[2], suffix), "wb") as out:
            out.write(data)


if __name__ == "__main__":
    main()
