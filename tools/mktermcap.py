#!/usr/bin/env python3
"""Make the terminal capability file, as usr/src/share/termcap does.

usage: mktermcap.py DIR OUTPUT

DIR is usr/src/share/termcap.  Its Makefile makes the file with the
editor, `ex - termcap.src < reorder`: reorder adds termcap.local and
moves the commonest terminals to the front, where tgetent(3), which
reads the file from the beginning, finds them soonest.  This does what
that does, with the few editor commands reorder uses: `$r FILE`;
`/RE/;.,/RE/-mN`, which moves an entry, from the line RE first matches
after the current one to the line before the next that matches the
second, to after line N; `w! FILE`, which here writes OUTPUT; and `q`.
"""
import os
import re
import sys


def read(path):
    """PATH's lines.  Only a newline ends one: the file has control
    characters in it, which str.splitlines() would take for ends."""
    text = open(path, encoding="latin-1").read()
    return text[:-1].split("\n") if text.endswith("\n") else text.split("\n")


def search(lines, dot, pattern):
    """The first line after DOT that PATTERN, a basic regular expression
    of the kinds reorder has, matches, going on from the beginning."""
    rx = re.compile(re.sub(r"[|]", r"\\|", pattern))
    for k in range(1, len(lines) + 1):
        n = (dot + k - 1) % len(lines) + 1
        if rx.search(lines[n - 1]):
            return n
    sys.exit("mktermcap: nothing matches /%s/" % pattern)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    here, output = sys.argv[1:]
    lines = read(os.path.join(here, "termcap.src"))
    dot = len(lines)
    move = re.compile(r"/([^/]*)/;\.,/([^/]*)/-m(\d+)$")
    for cmd in read(os.path.join(here, "reorder")):
        m = move.match(cmd)
        if cmd.startswith("$r "):
            lines += read(os.path.join(here, cmd[3:]))
            dot = len(lines)
        elif m:
            first = search(lines, dot, m.group(1))
            last = search(lines, first, m.group(2)) - 1
            after = int(m.group(3))
            if first <= after < last:
                sys.exit("mktermcap: %s moves lines into themselves" % cmd)
            block = lines[first - 1:last]
            del lines[first - 1:last]
            if after >= first:
                after -= len(block)
            lines[after:after] = block
            dot = after + len(block)
        elif cmd.startswith("w! "):
            with open(output, "w", encoding="latin-1") as f:
                f.write("".join(s + "\n" for s in lines))
        elif cmd == "q":
            break
        else:
            sys.exit("mktermcap: reorder has a command this does not know: "
                     + cmd)


if __name__ == "__main__":
    main()
