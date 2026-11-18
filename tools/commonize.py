#!/usr/bin/env python3
"""Give an object file's uninitialized variables common storage.

usage: commonize.py OBJECT ...

Old C declares variables in headers without "extern", so that every file
that includes one defines them, and relies on the linker to make each
name one variable: the "common" model, in which an initialized
definition in one file and any number of uninitialized ones are the same
object.  Open Watcom has no such model.  It makes each uninitialized
variable an ordinary definition in _BSS, and the linker keeps the first
it meets and ignores the rest (warning W1027), even when a later one is
the definition with the initial value.  A program then finds its tables
empty.

This rewrites each object, in place, the way a compiler with common
storage would have written it: each public variable in _BSS becomes a
communal (an OMF COMDEF) of the same size, which the linker merges with
the others of that name and with a definition that has a value, if there
is one.  The compiler refers to a module's own public variables by name,
through its list of external names, so moving the name moves every
reference; the storage left behind in _BSS is unused.  A variable the module never
refers to is not on that list, and its communal goes on the end of it.

The size of each is the distance to the next variable in _BSS, public or
static, or to the end of the segment: never too small, and too large only
by the padding between variables.
"""
import struct
import sys


def index(b, i):
    """An OMF index at b[i]: the value and where it ends."""
    if b[i] & 0x80:
        return ((b[i] & 0x7F) << 8) | b[i + 1], i + 2
    return b[i], i + 1


def name(b, i):
    n = b[i]
    return b[i + 1:i + 1 + n].decode("latin-1"), i + 1 + n


def counted(s):
    e = s.encode("latin-1")
    return bytes([len(e)]) + e


def length(n):
    """A COMDEF communal length."""
    if n < 0x80:
        return bytes([n])
    if n < 0x10000:
        return b"\x81" + struct.pack("<H", n)
    if n < 0x1000000:
        return b"\x84" + struct.pack("<I", n)[:3]
    return b"\x88" + struct.pack("<I", n)


def records(data):
    out = []
    i = 0
    while i < len(data):
        t = data[i]
        n = struct.unpack_from("<H", data, i + 1)[0]
        out.append([t, data[i + 3:i + 3 + n - 1]])  # without the checksum
        i += 3 + n
    return out


def emit(t, body):
    r = bytes([t]) + struct.pack("<H", len(body) + 1) + body
    return r + bytes([(-sum(r)) & 0xFF])


def pubdefs(t, b):
    """A PUBDEF's group and segment, and its (name, offset, type) entries."""
    group, i = index(b, 0)
    seg, i = index(b, i)
    head = b[:i]
    if seg == 0:
        head = b[:i + 2]
        i += 2
    entries = []
    wide = t & 1
    while i < len(b):
        nm, i = name(b, i)
        if wide:
            off = struct.unpack_from("<I", b, i)[0]
            i += 4
        else:
            off = struct.unpack_from("<H", b, i)[0]
            i += 2
        typ, j = index(b, i)
        entries.append((nm, off, b[i:j]))
        i = j
    return seg, head, entries


def commonize(path):
    recs = records(open(path, "rb").read())
    lnames = [None]
    segs = [None]               # (name, length)
    for t, b in recs:
        if t == 0x96:
            i = 0
            while i < len(b):
                nm, i = name(b, i)
                lnames.append(nm)
        elif t in (0x98, 0x99):
            acbp = b[0]
            i = 1
            if (acbp >> 5) == 0:
                i += 3
            if t == 0x98:
                ln = struct.unpack_from("<H", b, i)[0]
                i += 2
            else:
                ln = struct.unpack_from("<I", b, i)[0]
                i += 4
            if acbp & 2:            # the big bit: a whole 64K
                ln = 0x10000
            segnm, i = index(b, i)
            segs.append((lnames[segnm], ln))
    bss = [k for k in range(1, len(segs)) if segs[k][0] == "_BSS"]
    if not bss:
        return 0
    bss = bss[0]

    public = {}                 # name: offset, of public variables in _BSS
    starts = {segs[bss][1]}     # where any variable in _BSS starts
    for t, b in recs:
        if t in (0x90, 0x91, 0xB6, 0xB7):
            seg, head, entries = pubdefs(t, b)
            if seg != bss:
                continue
            for nm, off, typ in entries:
                starts.add(off)
                if t in (0x90, 0x91):
                    public[nm] = off
    starts = sorted(starts)

    externs = set()
    for t, b in recs:
        if t == 0x8C:
            i = 0
            while i < len(b):
                nm, i = name(b, i)
                _, i = index(b, i)
                externs.add(nm)
    convert = {}
    for nm, off in public.items():
        nxt = min(s for s in starts if s > off) if off < starts[-1] else off
        convert[nm] = max(nxt - off, 1)
    if not convert:
        return 0
    # Those the module never refers to are not on its list of externals.
    # They go on the end of it, where their numbers are new ones.
    extra = [nm for nm in sorted(convert, key=lambda n: public[n])
             if nm not in externs]

    out = []
    for t, b in recs:
        if t == 0x8C:
            # Split the list into runs, keeping every name where it was:
            # an external's number is its place in the list.
            runs = []
            i = 0
            while i < len(b):
                start = i
                nm, i = name(b, i)
                _, i = index(b, i)
                if nm in convert:
                    entry = counted(nm) + b"\x00\x62" + length(convert[nm])
                    kind = 0xB0
                else:
                    entry = b[start:i]
                    kind = 0x8C
                if runs and runs[-1][0] == kind:
                    runs[-1][1] += entry
                else:
                    runs.append([kind, entry])
            for kind, body in runs:
                out.append(emit(kind, body))
        elif t in (0x90, 0x91):
            seg, head, entries = pubdefs(t, b)
            if seg == bss:
                keep = [e for e in entries if e[0] not in convert]
                if not keep:
                    continue
                fmt = "<I" if t & 1 else "<H"
                b = head + b"".join(counted(nm) + struct.pack(fmt, off) + typ
                                    for nm, off, typ in keep)
            out.append(emit(t, b))
        elif t == 0x8A:
            if extra:
                out.append(emit(0xB0, b"".join(
                    counted(nm) + b"\x00\x62" + length(convert[nm])
                    for nm in extra)))
            out.append(emit(t, b))
        else:
            out.append(emit(t, b))
    open(path, "wb").write(b"".join(out))
    return len(convert)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for p in sys.argv[1:]:
        commonize(p)


if __name__ == "__main__":
    main()
