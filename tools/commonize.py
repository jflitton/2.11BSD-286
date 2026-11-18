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

What is left in _BSS is then the module's static variables, and _BSS is
made only as large as they need, each moved down in turn and kept on an
odd or even address as it was.  Without that a program has the storage
of every variable a header defines again in every file that includes it:
28K of the C shell's 64K.  The module refers to a static variable by
name (a local external), whose offset moves, or, for one declared in a
function, by its offset in _BSS, held in the code or data the fixup
applies to, which is changed as the variable moves.  If anything refers
to _BSS in a way this does not know, _BSS is left as it was.
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


def bss_fixups(recs, bss):
    """Each fixup whose target is _BSS itself, by its offset in it, as
    [value, record, byte, wide]: the offset, and the record and the byte
    in it where it is kept, in a displacement if the fixup has one (WIDE
    if that is 32 bits) and if not in the data being fixed up.  None if
    there is one of a kind this does not know how to move."""
    threads = [None] * 4        # target threads: (method, index)
    data = None                 # the last LEDATA: (record, where its data starts)
    out = []
    for r, (t, b) in enumerate(recs):
        if t in (0xA0, 0xA1):
            _, i = index(b, 0)
            data = (r, i + (4 if t & 1 else 2))
            continue
        if t in (0xA2, 0xA3):
            data = None
            continue
        if t not in (0x9C, 0x9D):
            continue
        wide = t & 1
        i = 0
        while i < len(b):
            if not b[i] & 0x80:
                # A thread: D, the method and the thread's number.
                d, method, num = b[i] & 0x40, (b[i] >> 2) & 7, b[i] & 3
                i += 1
                if d == 0:
                    idx, i = index(b, i)
                    threads[num] = (method & 3, idx)
                elif method < 3:
                    _, i = index(b, i)
                continue
            locat = (b[i] << 8) | b[i + 1]
            i += 2
            fixdat = b[i]
            i += 1
            if not fixdat & 0x80:   # the frame given here
                if (fixdat >> 4) & 7 < 3:
                    _, i = index(b, i)
            if fixdat & 0x08:       # the target from a thread
                target = threads[fixdat & 3]
            else:
                idx, i = index(b, i)
                target = (fixdat & 3, idx)
            disp = None
            if not fixdat & 0x04:   # a displacement
                disp = i
                i += 4 if wide else 2
            if target != (0, bss):
                continue
            loc = (locat >> 10) & 0xF
            if loc == 2:            # the segment alone
                continue
            # Only a 16-bit offset, alone or in a far pointer, relative
            # to a segment, in an LEDATA.
            if not locat & 0x4000 or loc not in (1, 3, 5) or data is None:
                return None
            pos = data[1] + (locat & 0x3FF)
            value = struct.unpack_from("<H", recs[data[0]][1], pos)[0]
            if disp is None:
                out.append([value, data[0], pos, False])
            else:
                d = struct.unpack_from("<I" if wide else "<H", b, disp)[0]
                out.append([(value + d) & 0xFFFF, r, disp, wide])
    return out


def segdef_length(t, b, n):
    """The SEGDEF B with its length made N."""
    acbp = b[0]
    i = 1
    if (acbp >> 5) == 0:
        i += 3
    if t == 0x98:
        if n == 0x10000:
            return bytes([acbp | 2]) + b[1:i] + b"\0\0" + b[i + 2:]
        return bytes([acbp & ~2]) + b[1:i] + struct.pack("<H", n) + b[i + 2:]
    return bytes([acbp & ~2]) + b[1:i] + struct.pack("<I", n) + b[i + 4:]


def commonize(path):
    recs = [[t, bytearray(b)] for t, b in records(open(path, "rb").read())]
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

    # A static variable declared in a function has no name, and the
    # module refers to it by its offset in _BSS.
    end = segs[bss][1]
    fixups = bss_fixups(recs, bss)
    public = {}                 # name: offset, of public variables in _BSS
    static = set()              # offsets of static variables in _BSS
    if fixups is not None:
        static = {v for v, _, _, _ in fixups if v < end}
    starts = {end} | static     # where any variable in _BSS starts
    for t, b in recs:
        if t in (0x90, 0x91, 0xB6, 0xB7):
            seg, head, entries = pubdefs(t, b)
            if seg != bss:
                continue
            for nm, off, typ in entries:
                starts.add(off)
                if t in (0x90, 0x91):
                    public[nm] = off
                else:
                    static.add(off)
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

    # Where each static variable goes, and how large _BSS is then.  Each
    # keeps its offset odd or even, so that one that the module refers to
    # in its middle, which looks like two, is still in one piece.  Nothing
    # moves if the module refers by offset to where a public variable
    # starts, which would be the end of the one before it.
    moved = {}
    size = end
    if fixups is not None and not static & set(public.values()) and \
            not any(t in (0xA0, 0xA1, 0xA2, 0xA3) and index(b, 0)[0] == bss
                    for t, b in recs):
        size = 0
        for off in sorted(static):
            nxt = min(x for x in starts if x > off)
            size += (off - size) & 1
            moved[off] = size
            size += nxt - off
        # How far each reference moves: as far as the variable it is in,
        # or, just past the last one, as far as that.
        shift = []
        for value, r, pos, wide in fixups:
            base = [x for x in static if x <= value]
            if not base:
                moved, size, shift = {}, end, []
                break
            base = max(base)
            shift.append((moved[base] - base, r, pos, wide))
        for delta, r, pos, wide in shift:
            fmt = "<I" if wide else "<H"
            v = struct.unpack_from(fmt, recs[r][1], pos)[0]
            struct.pack_into(fmt, recs[r][1], pos,
                             (v + delta) & (0xFFFFFFFF if wide else 0xFFFF))

    out = []
    nseg = 0
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
        elif t in (0x90, 0x91, 0xB6, 0xB7):
            seg, head, entries = pubdefs(t, b)
            if seg == bss:
                keep = [(nm, moved.get(off, off), typ)
                        for nm, off, typ in entries if nm not in convert]
                if not keep:
                    continue
                fmt = "<I" if t & 1 else "<H"
                b = head + b"".join(counted(nm) + struct.pack(fmt, off) + typ
                                    for nm, off, typ in keep)
            out.append(emit(t, b))
        elif t in (0x98, 0x99):
            nseg += 1
            if nseg == bss:
                b = segdef_length(t, b, size)
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
