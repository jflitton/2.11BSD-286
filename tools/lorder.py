#!/usr/bin/env python3
"""What the system's lorder prints of its objects, for where the system
is built.

usage: lorder.py FILE.o ...

lorder (usr/src/usr.bin/lorder.sh) says of a set of objects which needs
which, for tsort to put them in an order a library can have: its
Makefile makes libc.a of `lorder *.o | tsort` (usr/src/lib/libc).  The
system's lorder is a script of nm, sed, sort and join, and the nm is
the system's, of its own objects (a.out.h), which this machine's is
not: this prints what the script prints there, to the byte, for the
tsort of the tree compiled for this machine to read.

The script takes from `nm -g` (usr/src/bin/nm) each file's name, and
prints it twice; and of each of its external symbols, the last word of
the line nm prints of it, which is its name or, in a named segment,
the segment's number.  A line with T or D in it is a symbol the file
defines; any other, one it wants, common storage among them.  Each list
of "SYMBOL FILE" is sorted, and join prints for each symbol wanted
and defined "WANTER DEFINER", for each line of the first list with
it and each of the second, in their order.  A file named once is
named twice, as the script has it.  Each file is named as it is given.
An object of the cross compiler's (OMF), of which the system's Makefiles
make a library where the system is built with it (tools/build), is
named twice and nothing more: its linker reads a library in any order.
"""
import re
import struct
import sys

A_OBJECT = 0x0186
N_EXT, N_TYPE = 0x20, 0x1F
N_FN = 0x1F
LETTER = {0x00: "u", 0x01: "a", 0x02: "t", 0x03: "d", 0x04: "b",
          0x14: "r", 0x1E: "s", 0x1D: "g"}


def fail(msg):
    sys.stderr.write("lorder.py: %s\n" % msg)
    sys.exit(1)


def nm(path):
    """The lines `nm -g` prints of an object after its name, or None
    where it says it has no name list."""
    data = open(path, "rb").read()
    if data[:1] == b"\x80":        # OMF's THEADR: the cross compiler's
        return []
    if len(data) < 24:
        fail("%s: not an object file" % path)
    (magic, _m, nseg, _es, _eo, _d, _s, _u,
     nsyms, nstrs) = struct.unpack_from("<HHHHHHHHLL", data, 0)
    if magic != A_OBJECT:
        fail("%s: not an object file" % path)
    if nsyms // 8 == 0:
        return None
    off = 24 + 12 * nseg
    for i in range(nseg):
        _t, nrel, filesz, _mem = struct.unpack_from("<HHLL", data,
                                                    24 + 12 * i)
        off += filesz + (filesz & 1) + 6 * nrel
    strs = data[off + nsyms:off + nsyms + nstrs]
    syms = []
    for k in range(nsyms // 8):
        strx, ntype, ovly, val = struct.unpack_from("<LBBH", data, off + 8 * k)
        if not strx or not ntype & N_EXT:
            continue
        syms.append((strs[strx:strs.index(b"\0", strx)], ntype, ovly, val))
    syms.sort(key=lambda s: s[0])
    out = []
    for name, ntype, ovly, val in syms:
        if ntype == N_FN:
            c = "f"
        else:
            c = LETTER.get(ntype & N_TYPE, "?")
            if c == "u" and val:
                c = "c"
        if ntype & N_EXT:
            c = c.upper()
        line = ("      " if c in "uU" else "%06o" % val) + " %s " % c
        line = line.encode() + name
        if ovly:
            line += b" %d" % ovly
        out.append(line)
    return out


def main():
    files = sys.argv[1:]
    if not files:
        sys.exit(__doc__)
    if len(files) == 1 and files[0].endswith(".o"):
        files = files * 2
    out = sys.stdout.buffer
    symdef, symref = [], []
    for f in files:
        lines = nm(f)
        if lines is None:
            sys.stderr.write("nm: %s: no name list\n" % f)
            continue
        # nm's line of the file's name, which sed prints twice and keeps
        name = f.encode()
        if b".o:" not in name + b":":
            continue
        held = name
        out.write(held + b" " + held + b"\n")
        for line in lines:
            last = line.rsplit(b" ", 1)[1]
            if re.search(rb"[TD] ", line):
                symdef.append(last + b" " + held)
            else:
                symref.append(last + b" " + held)
    symdef.sort()
    symref.sort()
    # join symref symdef, and sed 's/[^ ]* *//'
    i = j = 0
    while i < len(symref) and j < len(symdef):
        a = symref[i].split(b" ", 1)[0]
        b = symdef[j].split(b" ", 1)[0]
        if a < b:
            i += 1
        elif a > b:
            j += 1
        else:
            k = j
            while k < len(symdef) and symdef[k].split(b" ", 1)[0] == a:
                k += 1
            while i < len(symref) and symref[i].split(b" ", 1)[0] == a:
                for d in symdef[j:k]:
                    out.write(symref[i].split(b" ", 1)[1] + b" " +
                              d.split(b" ", 1)[1] + b"\n")
                i += 1
            j = k


if __name__ == "__main__":
    main()
