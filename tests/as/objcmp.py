#!/usr/bin/env python3
"""Say whether two assemblers made the same of one file.

usage: objcmp.py NASM.o AS.o

NASM.o is what NASM made (-f obj, the format of the cross tools), and
AS.o what the system's own assembler made of the same source
(usr/src/bin/as/i286, a.out.h).  The two are the same if the text, the
data and the bss are, byte for byte; if the same words in them are to be
relocated, in the same way and against the same thing; and if they
define the same global symbols, with the same values, and ask for the
same common storage.

The object formats put what is added to an address in different
places, and NASM lists every name it was told is external where the
system's assembler lists those that are used; both are allowed for.
Each of NASM's segments is part of the text, the data or the bss, by
its class, and those of one class follow each other in the order the
file names them, as the system's assembler has them.  The data and the
bss are one segment when a program runs, DGROUP to NASM.  Where a program
starts (..start) is a symbol to the one and not to the other, and is
not compared.

Exit status 0 if they are the same; else what differs is printed.
"""
import struct
import sys

SECT = {"CODE": 1, "DATA": 2, "BEGDATA": 2, "BSS": 3}
R_OFF, R_SEG, R_PCREL, R_EXT = 1, 2, 3, 0x10
N_EXT = 0x20


class Obj:
    def __init__(self):
        self.data = {1: bytearray(), 2: bytearray(), 3: bytearray()}
        self.size = {1: 0, 2: 0, 3: 0}
        self.relocs = set()     # (sect, addr, kind, target, addend)
        self.globals = set()    # (name, sect, value)
        self.commons = set()    # (name, size)


def read_omf(path):
    b = open(path, "rb").read()
    o = Obj()
    lnames = [None]
    segs = [None]               # (sect, length)
    exts = [None]
    last = None                 # (sect, offset) of the last LEDATA
    i = 0

    def index(body, p):
        if body[p] & 0x80:
            return ((body[p] & 0x7F) << 8) | body[p + 1], p + 2
        return body[p], p + 1

    def name(body, p):
        n = body[p]
        return body[p + 1:p + 1 + n].decode("latin-1"), p + 1 + n

    while i + 3 <= len(b):
        t = b[i]
        n = struct.unpack_from("<H", b, i + 1)[0]
        body = b[i + 3:i + 3 + n - 1]
        i += 3 + n
        if t in (0x80, 0x88, 0x9A):             # THEADR COMENT GRPDEF
            pass
        elif t == 0x96:                         # LNAMES
            p = 0
            while p < len(body):
                s, p = name(body, p)
                lnames.append(s)
        elif t == 0x98:                         # SEGDEF
            acbp = body[0]
            p = 1
            if acbp >> 5 == 0:
                p += 3
            length = struct.unpack_from("<H", body, p)[0]
            p += 2
            _, p = index(body, p)
            cls, p = index(body, p)
            sect = SECT.get(lnames[cls])
            if sect is None:
                sys.exit("%s: a segment of class %s" % (path, lnames[cls]))
            if acbp & 2:
                length = 0x10000
            base = o.size[sect]
            segs.append((sect, base))
            o.size[sect] = base + length
            if sect != 3:
                o.data[sect] += bytearray(length)
        elif t == 0x8C:                         # EXTDEF
            p = 0
            while p < len(body):
                s, p = name(body, p)
                _, p = index(body, p)
                exts.append(s)
        elif t == 0xB0:                         # COMDEF
            p = 0
            while p < len(body):
                s, p = name(body, p)
                _, p = index(body, p)
                p += 1
                v = body[p]
                p += 1
                if v == 0x81:
                    v = struct.unpack_from("<H", body, p)[0]
                    p += 2
                elif v == 0x84:
                    v = int.from_bytes(body[p:p + 3], "little")
                    p += 3
                elif v == 0x88:
                    v = struct.unpack_from("<L", body, p)[0]
                    p += 4
                exts.append(s)
                o.commons.add((s, v))
        elif t == 0x90:                         # PUBDEF
            _, p = index(body, 0)
            seg, p = index(body, p)
            if seg == 0:
                p += 2
            while p < len(body):
                s, p = name(body, p)
                off = struct.unpack_from("<H", body, p)[0]
                p += 2
                _, p = index(body, p)
                if seg:
                    o.globals.add((s, segs[seg][0], segs[seg][1] + off))
                else:
                    o.globals.add((s, 0, off))
        elif t == 0xA0:                         # LEDATA
            seg, p = index(body, 0)
            off = struct.unpack_from("<H", body, p)[0]
            p += 2
            sect, base = segs[seg]
            off += base
            o.data[sect][off:off + len(body) - p] = body[p:]
            last = (sect, off)
        elif t == 0x9C:                         # FIXUPP
            p = 0
            while p < len(body):
                if not body[p] & 0x80:
                    sys.exit("%s: a thread in a FIXUPP" % path)
                locat = (body[p] << 8) | body[p + 1]
                fix = body[p + 2]
                p += 3
                segrel = locat & 0x4000
                loctype = (locat >> 10) & 0xF
                where = last[1] + (locat & 0x3FF)
                frame = (fix >> 4) & 7
                if not fix & 0x80 and frame < 3:
                    _, p = index(body, p)
                method = fix & 3
                datum, p = index(body, p)
                if not fix & 4:
                    sys.exit("%s: a FIXUPP with a displacement" % path)
                sect = last[0]
                d = o.data[sect]
                field = struct.unpack_from("<H", d, where)[0]
                if method == 0:
                    target = segs[datum][0]
                    field = (field + segs[datum][1]) & 0xFFFF
                elif method == 2:
                    target = exts[datum]
                elif loctype == 2:
                    target = 2          # the group: the data's segment
                else:
                    sys.exit("%s: a FIXUPP against a group" % path)
                if loctype == 2:
                    kind = R_SEG
                    field = 0
                    if target == 3:
                        target = 2
                elif loctype in (1, 5):
                    kind = R_OFF if segrel else R_PCREL
                else:
                    sys.exit("%s: a FIXUPP of type %d" % (path, loctype))
                struct.pack_into("<H", d, where, 0)
                o.relocs.add((sect, where, kind, target, field))
        elif t in (0x8A, 0x8B):                 # MODEND
            break
        else:
            sys.exit("%s: record %#x" % (path, t))
    return o


def read_obj(path):
    b = open(path, "rb").read()
    (magic, model, nseg, _e, _o, _d, _s, _u,
     nsyms, nstrs) = struct.unpack_from("<HHHHHHHHLL", b, 0)
    if magic != 0x0186 or nseg != 3:
        sys.exit("%s: not an object" % path)
    o = Obj()
    off = 24 + 36
    nrel = {}
    for s in (1, 2, 3):
        _t, n, filesz, memsz = struct.unpack_from("<HHLL", b, 24 + 12 * (s - 1))
        nrel[s] = n
        o.size[s] = memsz
        o.data[s] = bytearray(b[off:off + filesz])
        off += filesz + (filesz & 1)
    rels = {}
    for s in (1, 2, 3):
        rels[s] = [struct.unpack_from("<HHH", b, off + 6 * k)
                   for k in range(nrel[s])]
        off += 6 * nrel[s]
    strs = b[off + nsyms:off + nsyms + nstrs]
    syms = []
    for k in range(nsyms // 8):
        strx, ntype, seg, val = struct.unpack_from("<LBbH", b, off + 8 * k)
        nm = strs[strx:strs.index(b"\0", strx)].decode("latin-1")
        syms.append((nm, ntype, seg, val))
        if ntype == N_EXT and val:
            o.commons.add((nm, val))
        elif ntype & N_EXT and ntype != N_EXT and nm[:2] != "..":
            # its type is its segment: N_TEXT is 2, and the text 1
            t = ntype & 0x1F
            o.globals.add((nm, t - 1 if 2 <= t <= 4 else 0, val))
    for s in (1, 2, 3):
        d = o.data[s]
        prev = -1
        for addr, sym, rtype in rels[s]:
            if addr <= prev:
                sys.exit("%s: relocation out of order" % path)
            prev = addr
            kind = rtype & 0xF
            field = struct.unpack_from("<H", d, addr)[0]
            if rtype & R_EXT:
                target = syms[sym][0]
            else:
                target = sym
            if kind == R_PCREL:
                field = (field + addr + 2) & 0xFFFF
            if kind == R_SEG and target == 3:
                target = 2              # the bss is in the data's segment
            struct.pack_into("<H", d, addr, 0)
            o.relocs.add((s, addr, kind, target, field))
    return o


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    a = read_omf(sys.argv[1])
    b = read_obj(sys.argv[2])
    bad = 0
    names = {1: "text", 2: "data", 3: "bss"}
    for s in (1, 2, 3):
        if a.size[s] != b.size[s]:
            print("%s: %d bytes and %d" % (names[s], a.size[s], b.size[s]))
            bad = 1
        elif a.data[s] != b.data[s]:
            for k in range(len(a.data[s])):
                if a.data[s][k] != b.data[s][k]:
                    break
            print("%s: differ at %#x: %s and %s" %
                  (names[s], k, a.data[s][k:k + 8].hex(),
                   b.data[s][k:k + 8].hex()))
            bad = 1
    for what, x, y in (("relocation", a.relocs, b.relocs),
                       ("global", a.globals, b.globals),
                       ("common", a.commons, b.commons)):
        if x != y:
            for r in sorted(x - y, key=str)[:5]:
                print("%s only NASM has: %s" % (what, (r,)))
            for r in sorted(y - x, key=str)[:5]:
                print("%s only as has: %s" % (what, (r,)))
            bad = 1
    sys.exit(bad)


if __name__ == "__main__":
    main()
