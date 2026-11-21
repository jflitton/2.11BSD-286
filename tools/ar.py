#!/usr/bin/env python3
"""Make a library of the system's object files, as ar and ranlib would.

usage: ar.py LIB.a FILE.o ...
       ar.py -t LIB.a

LIB.a is made anew of the files, in the order given, with the table of
contents first that ranlib makes and ld wants (__.SYMDEF): the archive
of ar(5), and the table of ranlib(5), as usr/include/ar.h and ranlib.h
have them.  The objects are the system's own (a.out.h), which its
assembler writes (usr/src/bin/as/i286).  This is for where the system
is built, which has the system's assembler and linker compiled for it
and not ar and ranlib; on the system those two make the same.

Every member's date is 0, and its owner, and its mode is 644: what is
made of the same files is the same.

-t lists a library's members, and the symbols its table has for each.
"""
import os
import struct
import sys

ARMAG = b"!<arch>\n"
ARFMAG = b"`\n"
SYMDEF = "__.SYMDEF"
A_OBJECT = 0x0186
N_EXT, N_TYPE = 0x20, 0x1F


def fail(msg):
    sys.stderr.write("ar.py: %s\n" % msg)
    sys.exit(1)


def header(name, size):
    """The 60 bytes before a member; a name that is too long for it, or
    has a blank, goes before the member's contents instead."""
    raw = name.encode()
    if len(raw) > 16 or b" " in raw:
        first = b"#1/%-13d" % len(raw)
        size += len(raw)
        extra = raw
    else:
        first = b"%-16s" % raw
        extra = b""
    h = first + b"%-12d%-6d%-6d%-8o%-10d" % (0, 0, 0, 0o644, size) + ARFMAG
    assert len(h) == 60
    return h, extra


def symbols(path, data):
    """The names an object defines for other files, its common storage
    among them, in the order of its symbol table."""
    if len(data) < 60:
        fail("%s: not an object file" % path)
    (magic, _m, nseg, _es, _eo, _d, _s, _u,
     nsyms, nstrs) = struct.unpack_from("<HHHHHHHHLL", data, 0)
    if magic != A_OBJECT or nseg != 3:
        fail("%s: not an object file" % path)
    off = 24 + 12 * nseg
    for i in range(nseg):
        _t, nrel, filesz, _mem = struct.unpack_from("<HHLL", data, 24 + 12 * i)
        off += filesz + (filesz & 1) + 6 * nrel
    strs = data[off + nsyms:off + nsyms + nstrs]
    names = []
    for k in range(nsyms // 8):
        strx, ntype, _seg, val = struct.unpack_from("<LBbH", data, off + 8 * k)
        if not strx or not ntype & N_EXT:
            continue
        if ntype & N_TYPE == 0 and not val:
            continue
        names.append(strs[strx:strs.index(b"\0", strx)])
    return names


def make(lib, files):
    members = []            # (header, contents, names)
    seen = set()
    for f in files:
        name = os.path.basename(f)
        if name in seen:
            fail("%s: two members of that name" % name)
        seen.add(name)
        data = open(f, "rb").read()
        h, extra = header(name, len(data))
        members.append((h, extra + data, symbols(f, data)))
    nsym = sum(len(m[2]) for m in members)
    strlen = sum(len(n) + 1 for m in members for n in m[2])
    ransize = 4 + 8 * nsym + 4 + strlen
    pad = ransize & 1
    ransize += pad
    first = len(ARMAG) + 60 + ransize
    out = bytearray(ARMAG)
    out += header(SYMDEF, ransize)[0]
    out += struct.pack("<L", 8 * nsym)
    pos = first
    strx = 0
    for h, data, names in members:
        for n in names:
            out += struct.pack("<LL", strx, pos)
            strx += len(n) + 1
        pos += len(h) + len(data) + (len(data) & 1)
    out += struct.pack("<L", strlen)
    for _h, _data, names in members:
        for n in names:
            out += n + b"\0"
    if pad:
        out += b"\n"
    assert len(out) == first
    for h, data, _names in members:
        out += h + data
        if len(data) & 1:
            out += b"\n"
    with open(lib, "wb") as f:
        f.write(out)


def table(lib):
    b = open(lib, "rb").read()
    if b[:8] != ARMAG:
        fail("%s: not a library" % lib)
    pos = 8
    syms = {}
    while pos + 60 <= len(b):
        h = b[pos:pos + 60]
        if h[58:60] != ARFMAG:
            fail("%s: bad header at %d" % (lib, pos))
        size = int(h[48:58])
        name = h[:16].rstrip().decode()
        data = pos + 60
        if name.startswith("#1/"):
            n = int(name[3:])
            name = b[data:data + n].decode()
            data += n
        if name == SYMDEF:
            n = struct.unpack_from("<L", b, data)[0] // 8
            strs = data + 4 + 8 * n + 4
            for k in range(n):
                strx, off = struct.unpack_from("<LL", b, data + 4 + 8 * k)
                s = b[strs + strx:b.index(b"\0", strs + strx)].decode()
                syms.setdefault(off, []).append(s)
        else:
            print("%s: %s" % (name, " ".join(syms.get(pos, []))))
        pos += 60 + size + (size & 1)


def main():
    a = sys.argv[1:]
    if len(a) == 2 and a[0] == "-t":
        table(a[1])
    elif len(a) >= 2 and not a[0].startswith("-"):
        make(a[0], a[1:])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
