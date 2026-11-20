#!/usr/bin/env python3
"""Make fortune's data file, as strfile(6) does, for the 286.

usage: strfile.py [-r] SOURCE DATA

SOURCE is fortune's text: strings each ended by a line that begins with
two %, or with "%-", which also marks where a section begins
(usr/src/games/fortune/strfile.c).  DATA is what fortune(6) reads: a
table, its <strfile.h> as the 286 lays it out, of the number of
strings, the longest and the shortest one's lengths (unsigned ints),
where each of three sections begins (longs) and flags (an int); then the
offset in the file of each string, and of where the last one ends, a
long each; then the strings, each followed by a NUL.  Numbers are
little-endian and the structure has no padding: 20 bytes.

-r puts each section's strings in a random order, as strfile -r does,
and sets the flag that says so.  fortune does not choose a string by
chance: it takes the next of the section, and keeps its place in the
table.  strfile shuffles with random(), seeded with the time; this
shuffles the same way with the C library's rand() from its first number
(lib/libc/compat-4.1/rand.c, as a 16-bit int has it), so that each build
makes the same file.
"""
import struct
import sys

MAXDELIMS = 3
STR_RANDOM = 0x1


class Rand:
    """rand(): the generator's upper bits, 0 to 0x7fff."""
    def __init__(self):
        self.x = 1

    def __call__(self):
        self.x = (self.x * 1103515245 + 12345) & 0xffffffff
        return (self.x >> 16) & 0x7fff


def main():
    args = sys.argv[1:]
    rflag = False
    while args and args[0].startswith("-"):
        if args[0] == "-r":
            rflag = True
        else:
            sys.exit(__doc__)
        args = args[1:]
    if len(args) != 2:
        sys.exit(__doc__)
    lines = open(args[0], "rb").read().splitlines(True)

    strings, delims, cur = [], [], b""
    for line in lines:
        if line.startswith(b"%%") or line.startswith(b"%-"):
            strings.append(cur)
            cur = b""
            if line.startswith(b"%-") and len(delims) < MAXDELIMS:
                delims.append(len(strings))
        else:
            cur += line
    strings.append(cur)

    flags = 0
    order = list(range(len(strings)))
    if rflag:
        # strfile's randomize(): within each section, each place in turn
        # takes one of those from it to the section's end.
        flags |= STR_RANDOM
        rand = Rand()
        bounds = [0] + delims + [len(strings)]
        for start, cnt in zip(bounds, bounds[1:]):
            for k in range(start, cnt):
                i = k + rand() % (cnt - k)
                order[k], order[i] = order[i], order[k]

    head = 20 + 4 * (len(strings) + 1)
    offsets, off = [], head
    for s in strings:
        offsets.append(off)
        off += len(s) + 1
    end = off
    lengths = [len(s) for s in strings]
    table = struct.pack("<HHH3lh", len(strings), max(lengths), min(lengths),
                        *(delims + [0] * (MAXDELIMS - len(delims))), flags)
    seekpts = [offsets[i] for i in order] + [end]
    out = table + struct.pack("<%dl" % len(seekpts), *seekpts)
    out += b"".join(s + b"\0" for s in strings)
    open(args[1], "wb").write(out)


if __name__ == "__main__":
    main()
