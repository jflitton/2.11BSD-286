#!/usr/bin/env python3
"""ex and ed, where the system is built, for the Makefiles that have
them edit: tools/build/build.sh's.

usage: ex.py TOP [ex's arguments]
       ex.py TOP -ed [ed's arguments]

`ex - termcap.src`, share/termcap's, has it run reorder on termcap.src:
tools/mktermcap.py does what reorder does.  `ex - <tokenscript`,
bin/awk's, has it put in token.c the table of the tokens' names and
numbers, a line of y.tab.h each, done here as the script does it:
of each line, `# define NAME N` is `"NAME", N,`, and the lines between
the table's `{` and `};` are those.

Any other `ex - FILE` or `ed - FILE` has the commands of the script it
reads done to FILE, as the system's ex and ed do them, of those that
pascal's Makefiles have them do (gram.ex, which makes eyacc's y.tab.c
pi's and pxp's, and px's Emake.ed): addresses by number, `.`, `$`, a
mark, a search forward or back, which goes round the end, and with `+`
and `-`, joined by `,` or by `;`, which sets the current line; and the
commands a, d, e, g, k, m, q, s, t and w.  The current line after each
is as ex and ed leave it.  A search that finds nothing, a substitution
that changes nothing, or a command not among those is an error, which
ends it before it writes, as the system's does when what it reads is
not a terminal.
"""
import os
import re
import subprocess
import sys

top = sys.argv[1]
args = sys.argv[2:]
ed = args[:1] == ["-ed"]
if ed:
    args = args[1:]
if not ed and args == ["-", "termcap.src"]:
    sys.exit(subprocess.call(["python3", "-I", top + "/tools/mktermcap.py",
                              ".", "termcap"]))
script = sys.stdin.read()
name = "ed" if ed else "ex"


class Error(Exception):
    pass


def bre(pat):
    """The regular expression PAT, as ed and ex have it, as Python's."""
    out = []
    i = 0
    while i < len(pat):
        c = pat[i]
        if c == "\\" and i + 1 < len(pat):
            d = pat[i + 1]
            i += 2
            if d in "()":
                out.append(d)
            elif d in "<>":
                out.append(r"\b")
            else:
                out.append(re.escape(d))
            continue
        if c == "[":
            j = i + 1
            if j < len(pat) and pat[j] == "^":
                j += 1
            if j < len(pat) and pat[j] == "]":
                j += 1
            j = pat.index("]", j)
            body = pat[i + 1:j].replace("\\", "\\\\").replace("[", "\\[")
            if body.startswith("^]"):
                body = "^\\]" + body[2:]
            elif body.startswith("]"):
                body = "\\]" + body[1:]
            out.append("[" + body + "]")
            i = j + 1
            continue
        if c == "^" and i == 0:
            out.append("^")
        elif c == "$" and i == len(pat) - 1:
            out.append("$")
        elif c == "*" and (i == 0 or out[-1:] == ["("]):
            out.append(r"\*")
        elif c in "*.":
            out.append(c)
        else:
            out.append(re.escape(c))
        i += 1
    return re.compile("".join(out))


class Line:
    """A line, which is itself wherever it is moved, as a mark is."""

    def __init__(self, text):
        self.text = text


class Editor:
    def __init__(self, path):
        self.path = path
        self.marks = {}
        self.last = None
        self.read()

    def read(self):
        with open(self.path, errors="surrogateescape") as f:
            self.buf = [Line(t) for t in f.read().splitlines()]
        self.dot = len(self.buf)

    def regex(self, pat):
        if pat:
            self.last = bre(pat)
        if self.last is None:
            raise Error("no previous regular expression")
        return self.last

    def search(self, pat, back, start):
        r = self.regex(pat)
        n = len(self.buf)
        for k in range(1, n + 1):
            i = (start - k - 1) % n + 1 if back else (start + k - 1) % n + 1
            if r.search(self.buf[i - 1].text):
                return i
        raise Error("pattern not found: " + pat)

    def delimited(self, text, i, delim):
        """What is between I and the next DELIM not escaped, and where
        after it the text goes on."""
        j = i
        s = []
        while j < len(text) and text[j] != delim:
            if text[j] == "\\" and j + 1 < len(text):
                if text[j + 1] == delim:
                    s.append(delim)
                else:
                    s.append(text[j:j + 2])
                j += 2
                continue
            s.append(text[j])
            j += 1
        return "".join(s), min(j + 1, len(text)) if j < len(text) else j

    def address(self, text, i):
        """One address at I, or None, and where after it the text goes
        on."""
        a = None
        m = re.match(r"\s*", text[i:])
        i += m.end()
        if i < len(text):
            c = text[i]
            if c == ".":
                a, i = self.dot, i + 1
            elif c == "$":
                a, i = len(self.buf), i + 1
            elif c.isdigit():
                m = re.match(r"\d+", text[i:])
                a, i = int(m.group()), i + m.end()
            elif c == "'":
                k = text[i + 1]
                if k not in self.marks or \
                        self.marks[k] not in self.buf:
                    raise Error("undefined mark " + k)
                a = self.buf.index(self.marks[k]) + 1
                i += 2
            elif c in "/?":
                pat, i = self.delimited(text, i + 1, c)
                a = self.search(pat, c == "?", self.dot)
        while i < len(text) and text[i] in "+-":
            sign = 1 if text[i] == "+" else -1
            m = re.match(r"\d+", text[i + 1:])
            n = int(m.group()) if m else 1
            a = (self.dot if a is None else a) + sign * n
            i += 1 + (m.end() if m else 0)
        return a, i

    def addresses(self, text, i):
        """The addresses at I, the last two of them, and where the
        command is."""
        got = []
        while True:
            a, i = self.address(text, i)
            if i < len(text) and text[i] in ",;":
                if a is None:
                    a = 1 if text[i] == "," and not got else self.dot
                got.append(a)
                if text[i] == ";":
                    self.dot = a
                i += 1
                continue
            if a is not None:
                got.append(a)
            break
        for a in got:
            if a < 0 or a > len(self.buf):
                raise Error("address out of range")
        return got[-2:], i

    def write(self, lo, hi, path):
        with open(path, "w", errors="surrogateescape") as f:
            for ln in self.buf[lo - 1:hi]:
                f.write(ln.text + "\n")

    def substitute(self, lo, hi, rest):
        delim = rest[0]
        pat, j = self.delimited(rest, 1, delim)
        repl, j = self.delimited(rest, j, delim)
        flags = rest[j:].strip()
        r = self.regex(pat)
        parts = []
        k = 0
        while k < len(repl):
            c = repl[k]
            if c == "&":
                parts.append(0)
            elif c == "\\" and k + 1 < len(repl):
                d = repl[k + 1]
                parts.append(int(d) if d.isdigit() else d)
                k += 1
            else:
                parts.append(c)
            k += 1

        def to(m):
            return "".join(m.group(p) if isinstance(p, int) else p
                           for p in parts)
        done = None
        for n in range(lo, hi + 1):
            ln = self.buf[n - 1]
            new, count = r.subn(to, ln.text, 0 if "g" in flags else 1)
            if count:
                ln.text = new
                done = n
        if done is None:
            raise Error("substitute pattern match failed")
        self.dot = done

    def command(self, text, more):
        """Do the command TEXT, whose text to add, for a, is from MORE,
        a function that gives the next line or None."""
        got, i = self.addresses(text, 0)
        m = re.match(r"\s*", text[i:])
        i += m.end()
        c = text[i] if i < len(text) else ""
        rest = text[i + 1:]
        whole = c in "gw"
        if not got:
            got = [1, len(self.buf)] if whole else [self.dot]
        lo, hi = got[0], got[-1]
        if c in "dkmst" and (lo < 1 or lo > hi):
            raise Error("bad range")
        if c == "a":
            new = []
            while True:
                t = more()
                if t is None or t == ".":
                    break
                new.append(Line(t))
            self.buf[hi:hi] = new
            self.dot = hi + len(new)
        elif c == "d":
            del self.buf[lo - 1:hi]
            self.dot = min(lo, len(self.buf))
        elif c == "e":
            self.read()
        elif c == "g":
            delim = rest[0]
            pat, j = self.delimited(rest, 1, delim)
            r = self.regex(pat)
            lines = rest[j:].split("\n")
            marked = [ln for ln in self.buf[lo - 1:hi] if r.search(ln.text)]
            for ln in marked:
                if ln not in self.buf:
                    continue
                self.dot = self.buf.index(ln) + 1
                todo = iter(lines)
                for t in todo:
                    self.command(t, lambda: next(todo, None))
        elif c == "k":
            self.marks[rest.strip()] = self.buf[hi - 1]
        elif c in "mt":
            a, _ = self.address(rest, 0)
            a = self.dot if a is None else a
            block = self.buf[lo - 1:hi]
            if c == "m":
                if lo <= a < hi:
                    raise Error("move into itself")
                del self.buf[lo - 1:hi]
                if a >= hi:
                    a -= hi - lo + 1
            else:
                block = [Line(ln.text) for ln in block]
            self.buf[a:a] = block
            self.dot = a + len(block)
        elif c == "q":
            sys.exit(0)
        elif c == "s":
            self.substitute(lo, hi, rest)
        elif c == "w":
            path = rest.lstrip("!").strip() or self.path
            self.write(lo, hi, path)
        elif c == "":
            if got:
                self.dot = hi
        else:
            raise Error("not done here: " + text)


def main():
    files = [a for a in args if a != "-"]
    if "-" not in args or len(files) != 1:
        sys.exit("%s: only %s - FILE is done here" % (name, name))
    e = Editor(files[0])
    lines = iter(script.split("\n"))
    for text in lines:
        # A g's commands go on to the next line after a \.
        while text.startswith("g") or re.match(r"[^a-z]*g/", text):
            if not text.endswith("\\"):
                break
            nxt = next(lines, None)
            if nxt is None:
                break
            text = text[:-1] + "\n" + nxt
        if text.startswith('"'):
            continue
        try:
            e.command(text, lambda: next(lines, None))
        except Error as x:
            sys.exit("%s: %s: %s" % (name, files[0], x))


if not ed and args == ["-"] and os.path.isfile("tokenscript") and \
        script == open("tokenscript").read():
    table = []
    for line in open("y.tab.h").read().splitlines():
        line = re.sub(r"# *define *", "", line, count=1)
        line = '"' + line
        line = line.replace(" ", '", ', 1)
        table.append(line + ",")
    lines = open("token.c").read().splitlines()
    start = next(i for i, l in enumerate(lines) if re.search(r"= *{$", l))
    end = next(i for i in range(start + 1, len(lines))
               if re.match(r"^};$", lines[i]))
    lines[start + 1:end] = table
    with open("token.c", "w") as f:
        f.write("\n".join(lines) + "\n")
else:
    main()
