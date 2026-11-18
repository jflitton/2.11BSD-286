#!/usr/bin/env python3
"""Change a variable in an executable file.

usage: kpatch.py EXECUTABLE NAME=VALUE ...

EXECUTABLE is in the system's format (docs/executable-format.md) and has
its symbol table; NAME is a C variable in it, 16 bits, with a value in the
file, which is to say one that the program gives an initial value other
than zero.  The kernel marks the variables meant for this "(patchable)".
"""
import struct
import sys

HDR = "<HHHHHHHHLL"
SEG = "<HHLL"


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    path = sys.argv[1]
    b = bytearray(open(path, "rb").read())
    (magic, _m, nseg, _es, _eo, _ds, _st, _r, symsize, strsize) = \
        struct.unpack_from(HDR, b, 0)
    if magic != 0x0286:
        sys.exit("%s: not an executable" % path)
    off = struct.calcsize(HDR) + nseg * struct.calcsize(SEG)
    where = []                  # (file offset, bytes in file) of each segment
    for i in range(nseg):
        _k, _r, filesz, _memsz = struct.unpack_from(
            SEG, b, struct.calcsize(HDR) + i * struct.calcsize(SEG))
        where.append((off, filesz))
        off += filesz + (filesz & 1)
    symoff, stroff = off, off + symsize
    syms = {}
    for o in range(symoff, symoff + symsize, 8):
        strx, _type, seg, value = struct.unpack_from("<LBbH", b, o)
        end = b.index(b"\0", stroff + strx)
        syms[b[stroff + strx:end].decode()] = (seg, value)
    for arg in sys.argv[2:]:
        name, _, value = arg.partition("=")
        if "_" + name not in syms:
            sys.exit("%s: no symbol %s" % (path, name))
        seg, at = syms["_" + name]
        base, filesz = where[seg - 1]
        if at + 2 > filesz:
            sys.exit("%s: %s has no value in the file" % (path, name))
        struct.pack_into("<H", b, base + at, int(value, 0))
    open(path, "wb").write(b)


if __name__ == "__main__":
    main()
