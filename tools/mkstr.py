#!/usr/bin/env python3
"""Take a program's error messages out of its C, as mkstr(1) does.

usage: mkstr.py [-e NAME[=NEW]]... [-n N] STRINGS OUTDIR FILE ...

Each FILE is copied to OUTDIR, under its own name, with every string
that is the first argument of a call to error() (or of a call to NAME(),
for each -e) replaced by its offset in STRINGS, which gets the message.
The program reads a message from the file when it has one to print, and
has only the number in its code and none of the text in its data: ex
keeps its messages in /usr/share/misc/exstrings this way
(usr/src/ucb/mkstr).  As mkstr does, a name matches at its end, so that
"error(" is also serror( and cerror(, and a string must follow the
parenthesis directly.

tcsh's mkprstr (usr/src/bin/tcsh) is a mkstr of its own for its
xprintf(), and the other two options do what it does differently: with
NAME=NEW the call becomes one to NEW(), which reads the string from the
file and prints with it; with -n a string written in fewer than N
characters is left where it is, as costing less than the number.

In STRINGS each message is followed by a NUL and a newline, and a
message that is there already is used again.  It begins with an empty
one, so that no message has the offset 0, which a program takes to mean
none: ex's error(0).
"""
import os
import sys

ESCAPES = {"a": "\a", "b": "\b", "t": "\t", "r": "\r", "n": "\n",
           "f": "\f", "v": "\v", "\\": "\\"}


def string(text, i):
    """The string whose text begins at TEXT[i], and where it ends, as
    mkstr reads one: a backslash and newline join lines, and a newline
    inside it is kept."""
    out = []
    while i < len(text):
        c = text[i]
        i += 1
        if c == '"':
            return "".join(out), i
        if c == "\\":
            c = text[i]
            i += 1
            if c == "\n":
                continue
            if c in "01234567":
                n = int(c)
                for _ in range(2):
                    if text[i] not in "01234567":
                        break
                    n = n * 8 + int(text[i])
                    i += 1
                c = chr(n)
            else:
                c = ESCAPES.get(c, c)
        out.append(c)
    sys.exit("mkstr: a string does not end")


def main():
    args = sys.argv[1:]
    names = {}                  # NAME(: what it becomes
    shortest = 0
    while len(args) > 1 and args[0] in ("-e", "-n"):
        if args[0] == "-n":
            shortest = int(args[1])
        else:
            name, _, new = args[1].partition("=")
            names[name + "("] = (new or name) + "("
        del args[:2]
    if len(args) < 3:
        sys.exit(__doc__)
    strings, outdir, files = args[0], args[1], args[2:]
    names = names or {"error(": "error("}

    out = bytearray(b"\0\n")
    where = {}                  # message: its offset

    def offset(msg):
        if msg not in where:
            where[msg] = len(out)
            out.extend(msg.encode("latin-1") + b"\0\n")
        return where[msg]

    for path in files:
        text = open(path, encoding="latin-1", newline="").read()
        result = []
        last = 0
        i = text.find('"')
        while i >= 0:
            name = [n for n in names if text.endswith(n, 0, i)]
            if name:
                msg, end = string(text, i + 1)
            if name and end - 1 - (i + 1) >= shortest:
                n = name[0]
                result.append(text[last:i - len(n)] + names[n] +
                              str(offset(msg)))
                last = end
                i = text.find('"', end)
            else:
                i = text.find('"', i + 1)
        result.append(text[last:])
        name = os.path.join(outdir, os.path.basename(path))
        with open(name, "w", encoding="latin-1", newline="") as f:
            f.write("".join(result))
    with open(strings, "wb") as f:
        f.write(out)


if __name__ == "__main__":
    main()
