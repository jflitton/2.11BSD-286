#!/usr/bin/env python3
"""Make monop's deck of cards, /usr/games/lib/cards.pck, as initdeck does.

usage: initdeck.py CARDS.INP CARDS.PCK

CARDS.INP is usr/src/games/monop/cards.inp: the Community Chest cards
and then the Chance cards, a line "%-" between the two decks and "%%"
between cards, each card a line of two characters and a number that say
what it does, and then its text (usr/src/games/monop/initdeck.c).
CARDS.PCK is what monop reads (cards.c): a <deck.h> DECK for each deck,
as the 286 lays it out, its number of cards and of the last one drawn
(ints), whether its get-out-of-jail-free card is out (a char, then a
byte to align what follows) and a pointer, which monop sets itself and
initdeck writes as 0: 8 bytes; then the offset in the file of each card of
the first deck and then of the second, a long each; then each card, its
two characters, its number as an int, and its text with a NUL after it.
Numbers are little-endian.  This reads the text as initdeck reads it,
one character at a time, so that the file is the same.
"""
import struct
import sys

EOF = -1


class Reader:
    def __init__(self, data):
        self.data, self.i = data, 0

    def getc(self):
        if self.i >= len(self.data):
            return EOF
        self.i += 1
        return self.data[self.i - 1]


def count(data):
    """initdeck's count(): how many cards each deck has."""
    inf = Reader(data)
    num = [0, 0]
    deck, newline = 0, True
    while True:
        c = inf.getc()
        if c == EOF:
            break
        if newline and c == ord("%"):
            newline = False
            num[deck] += 1
            if inf.getc() == ord("-"):
                deck = 1
        else:
            newline = c == ord("\n")
    num[deck] += 1
    return num


def putem(data, start):
    """initdeck's putem(): the cards, from START in the file, and where
    each deck's begin."""
    inf = Reader(data)
    out = bytearray()
    offsets = [[start], []]
    deck = 0

    def ftell():
        return start + len(out)

    def number(stop_at_eof):
        num = 0
        while True:
            c = inf.getc()
            if c == ord("\n") or (stop_at_eof and c == EOF):
                break
            num = num * 10 + (c - ord("0"))
        out.extend(struct.pack("<h", num))

    out.append(inf.getc() & 0xff)
    out.append(inf.getc() & 0xff)
    number(False)
    newline = False
    while True:
        c = inf.getc()
        if c == EOF:
            break
        if newline and c == ord("%"):
            out.append(0)
            newline = False
            if inf.getc() == ord("-"):
                deck = 1
            while inf.getc() != ord("\n"):
                pass
            offsets[deck].append(ftell())
            c = inf.getc()
            if c == EOF:
                break
            out.append(c)
            out.append(inf.getc() & 0xff)
            number(True)
        else:
            out.append(c)
            newline = c == ord("\n")
    out.append(0)
    return offsets, bytes(out)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    data = open(sys.argv[1], "rb").read()
    num = count(data)
    start = 2 * 8 + 4 * (num[0] + num[1])
    offsets, cards = putem(data, start)
    if [len(o) for o in offsets] != num:
        sys.exit("initdeck.py: %s: the decks counted differently" %
                 sys.argv[1])
    out = b"".join(struct.pack("<hhbxH", len(o), 0, 0, 0) for o in offsets)
    for o in offsets:
        out += struct.pack("<%dl" % len(o), *o)
    open(sys.argv[2], "wb").write(out + cards)


if __name__ == "__main__":
    main()
