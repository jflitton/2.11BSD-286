#!/usr/bin/env python3
"""Make the index refer(1) searches /usr/dict/papers with, as its runinv
does when refer's Makefile installs it, for the 286.

usage: runinv.py EIGN DIR PAPER...

runinv is `mkey R* | inv -v -h997 -n Xind`, and the files renamed
Ind.ia, Ind.ib and Ind.ic, which this writes in DIR
(usr/src/usr.bin/refer: mkey?.c, inv?.c, deliv2.c).  mkey reads each
PAPER, records ended by an empty line, and for each record writes its
place, PAPER:OFFSET,LENGTH, and its keys: of each word, letters and
digits, its first six, in lower case, of three or more and not among
the first 100 of EIGN (/usr/share/misc/eign), nor a number but a year,
19xx; and not those of lines of %X, %Y, %Z or %#, nor any after the
record's first 500 bytes.  inv writes the places in Ind.ic, a line
each, and hashes each key into 997: Ind.ib has for each hash the
offsets in Ind.ic of the records with a key of it, ints, each list
ended by -1; Ind.ia the number of hashes and whether the offsets are
longs (an int each), where each list begins in Ind.ib (longs) and how
long each is (ints, as inv counts them).  The hash is the 286's, of
16-bit ints, as are the ints, little-endian.
"""
import os
import struct
import sys

NHASH = 997
COMCOUNT = 100
KEYCOUNT = 100
MINLEN = 3
MAXLINE = 500
IGLIST = b"XYZ#"


def short(n):
    """N as a 16-bit int has it."""
    n &= 0xffff
    return n - 0x10000 if n & 0x8000 else n


def hash16(s):
    """deliv2.c's hash(), in 16-bit ints.  A shift by a negative count
    (n % 4 of a negative n) is by its five low bits on the 286, 29 to
    31, which leaves nothing of a 16-bit int."""
    n = 0
    for c in s:
        k = int(n / 4) * 4
        count = (n - k) & 0x1f
        t = short(c * n + c)
        n = short(n + (short(t << count) if count < 16 else 0))
    return n if n > 0 else short(-n)


def cmod(a, b):
    """C's %, which takes the sign of A."""
    r = abs(a) % b
    return -r if a < 0 else r


def fgets(data, pos, size):
    """fgets(s, SIZE, f) from DATA at POS: the line and where it ends."""
    end = data.find(b"\n", pos, pos + size - 1)
    end = pos + size - 1 if end < 0 else end + 1
    end = min(end, len(data))
    return data[pos:end], end


def records(data):
    """mkey2.c's grec(): each record's text, offset and length."""
    pos = lp = 0
    while True:
        s = b""
        curtype = 0
        tlen = 0
        n = 0
        while pos < len(data):
            tm, pos = fgets(data, pos, 200)
            tlen += len(tm)
            if tm[:1] in (b"%", b"."):
                curtype = tm[1] if len(tm) > 1 else 0
            if tlen < MAXLINE and (curtype == 0 or curtype not in IGLIST):
                s += tm
            n = tlen
            if tm[:1] == b"\n":
                break
        else:
            if not s:
                return
            n = tlen
        yield s, lp, n
        lp += n


def mkey(eign, papers):
    """mkey's lines, without their newlines: PLACE TAB KEYS."""
    common = set()
    with open(eign, "rb") as f:
        data = f.read()
    pos = 0
    for _ in range(COMCOUNT):
        if pos >= len(data):
            break
        w, pos = fgets(data, pos, 15)
        common.add(w[:-1] if w.endswith(b"\n") else w)
    out = []
    for name in papers:
        with open(name, "rb") as f:
            data = f.read()
        label = os.path.basename(name).encode()
        for text, lp, lim in records(data):
            keys = []
            key = b""
            alph = 0
            for c in text:
                if len(keys) >= KEYCOUNT:
                    break
                if chr(c).isalnum() and c < 0x80:
                    if alph < 6:
                        key += bytes([c])
                    alph += 1
                    continue
                key = bytes(x | 0x20 for x in key)
                if (len(key) >= MINLEN and
                        (not key[:1].isdigit() or
                         (key[:2] == b"19" and len(key) == 4)) and
                        key not in common):
                    keys.append(key)
                key = b""
                alph = 0
            if keys:
                out.append(b"%s:%d,%d\t%s" % (label, lp, lim,
                                              b" ".join(keys)))
    return out


def inv(lines, outdir):
    """inv -h997 -n: Ind.ic, then the sorted hashes into Ind.ia and
    Ind.ib (inv2.c's newkeys(), inv6.c's whash())."""
    ic = b""
    items = []
    for line in lines:
        place, _, keys = line.partition(b"\t")
        lp = len(ic)
        ic += place + b"\n"
        for k in keys.split():
            items.append(b"%04d %06d\n" % (cmod(hash16(k), NHASH), lp))
    iflong = len(ic) >= 32768
    items.sort()
    size = 4 if iflong else 2
    word = "<l" if iflong else "<h"
    ib = []
    hpt = [0] * (NHASH + 1)
    hfreq = [0] * NHASH
    hash_ = ct = 0
    lp = 0
    opoint = -1
    for item in items:
        k, point = (int(x) for x in item.split())
        if hash_ < k:
            ib.append(struct.pack(word, -1))
            hfreq[hash_] = ct
            while hash_ < k:
                hash_ += 1
                hpt[hash_] = lp
                hfreq[hash_] = 0
            lp += size
            hpt[hash_] = lp
            opoint = -1
            ct = 0
        if point != opoint:
            opoint = point
            ib.append(struct.pack(word, point))
            lp += size
            ct += 1
    ib.append(struct.pack(word, -1))
    while hash_ < NHASH:
        hash_ += 1
        hpt[hash_] = lp
    ia = struct.pack("<hh", NHASH, int(iflong))
    ia += struct.pack("<%dl" % NHASH, *hpt[:NHASH])
    ia += struct.pack("<%dh" % NHASH, *(short(x) for x in hfreq))
    for ext, data in (("ia", ia), ("ib", b"".join(ib)), ("ic", ic)):
        with open(os.path.join(outdir, "Ind." + ext), "wb") as f:
            f.write(data)


def main():
    if len(sys.argv) < 4:
        sys.exit("usage: runinv.py EIGN DIR PAPER...")
    eign, outdir = sys.argv[1:3]
    os.makedirs(outdir, exist_ok=True)
    inv(mkey(eign, sorted(sys.argv[3:])), outdir)


main()
