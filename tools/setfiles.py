#!/usr/bin/env python3
"""Make phantasia's files, as its setfiles does when it is installed.

usage: setfiles.py DIR

setfiles (usr/src/games/phantasia/setfiles.c) makes, in phantasia's
directory, /usr/games/lib/phantasia: gold, motd, mess and enemy, empty;
lastdead, a blank; characs, the file of characters, with one, "<null>";
and void, the energy void that holds the grail.  This writes them in
DIR.  The two that are not text are structures of <phant.h> as the 286
lays them out: each member at an even offset but a char, little-endian
numbers, and IEEE doubles, as the 287 has them.

setfiles never seeds rand(), so what it rolls is the library's sequence
from its start, the same on the 286 as on the PDP-11: the character's
place and gold, and then where the grail is.  The members it does not
set are zero, there as here.
"""
import math
import os
import struct
import sys

# struct stats, in order: (name, kind), a kind being a struct format
# letter, or "sN" for a char array.
STATS = [
    ("name", "s21"), ("pswd", "s9"), ("login", "s10"),
    ("x", "d"), ("y", "d"), ("exp", "d"), ("lvl", "h"), ("quk", "h"),
    ("str", "d"), ("sin", "d"), ("man", "d"), ("gld", "d"), ("nrg", "d"),
    ("mxn", "d"), ("mag", "d"), ("brn", "d"), ("crn", "h"),
    ("rng.type", "h"), ("rng.duration", "h"), ("pal", "b"), ("psn", "d"),
    ("hw", "h"), ("amu", "h"), ("bls", "b"), ("chm", "h"), ("gem", "d"),
    ("quks", "h"), ("swd", "d"), ("shd", "d"), ("typ", "h"), ("vrg", "b"),
    ("lastused", "h"), ("status", "h"), ("tampered", "h"),
    ("scratch1", "d"), ("scratch2", "d"), ("blind", "b"),
    ("wormhole", "h"), ("age", "l"), ("degen", "h"),
]
NRGVOID = [("active", "b"), ("x", "d"), ("y", "d")]
TRUE, NONE = 1, 0


def pack(layout, values):
    """The structure as the 286's compiler lays it out: a member of more
    than one byte at an even offset, and the whole an even length."""
    out = bytearray()
    for name, kind in layout:
        if kind.startswith("s"):
            out += values.get(name, "").encode().ljust(int(kind[1:]), b"\0")
            continue
        v = values.get(name, 0)
        if kind != "b" and len(out) % 2:
            out.append(0)
        out += struct.pack("<" + kind, v)
    if len(out) % 2:
        out.append(0)
    return bytes(out)


class Rand:
    """The library's rand() (lib/libc/compat-4.1/rand.c) for a 16-bit
    int: the generator's upper bits, 0 to 0x7fff."""
    def __init__(self):
        self.x = 1

    def __call__(self):
        self.x = (self.x * 1103515245 + 12345) & 0xffffffff
        return (self.x >> 16) & 0x7fff


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    rand = Rand()
    RAND = 32768.0          # the Makefile's -DRAND

    def roll(base, interval):
        return math.floor(base + interval * (rand() / RAND))

    # buildchar(), and its gold, which adds two rolls, the left first.
    c = {"name": "<null>", "rng.type": NONE, "gem": 0.1}
    c["x"] = roll(-125, 251)
    c["y"] = roll(-125, 251)
    gld = roll(25, 50)
    c["gld"] = gld + roll(0, 25) + 0.1
    grail = {"active": TRUE}
    grail["x"] = roll(-1.0e6, 2.0e6)
    grail["y"] = roll(-1.0e6, 2.0e6)

    os.makedirs(out, exist_ok=True)
    for name in ("gold", "motd", "mess", "enemy"):
        open(os.path.join(out, name), "wb").close()
    open(os.path.join(out, "lastdead"), "wb").write(b" ")
    open(os.path.join(out, "characs"), "wb").write(pack(STATS, c))
    open(os.path.join(out, "void"), "wb").write(pack(NRGVOID, grail))


if __name__ == "__main__":
    main()
