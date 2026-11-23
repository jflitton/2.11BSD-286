#!/usr/bin/env python3
"""Format the manual for the system's /usr/man, as upstream installs it.

usage: mkman.py NROFF SRC LIST OUT

Upstream keeps the manual formatted: man(1) shows /usr/man/catN/NAME.0
and nothing else, and each page is made of its source by manroff,
`nroff -man -h`, as the Makefiles of usr/src/man and of the programs
install it, or by `nroff -man` where a program's Makefile has it so.  This makes the same pages here, with NROFF, the tree's
nroff built for the machine the system is built on, which is run in its
own directory: it is built to look for its macros in tmac/ there, and
the copies of them there say tmac/ where the system's say
/usr/share/tmac/ (usr/src/i286/Makefile).  tests/programs checks that
the system's nroff makes the same pages.

LIST says which pages go on the system: a line names a directory of
/usr/man (or one beside it, ../new/man/cat1, whose pages are not in
whatis) and the sources, relative to SRC, usr/src, of the pages that go
in it; a Makefile there stands for the pages it formats, those its SRCS,
SRCS1 and so on list, as usr/src/man/man2/Makefile does all of section
2.  A page's name is its source's less the source's suffix, `.1`, `.3f`,
`.man` or the like: bin/ar/ar.5.5 would be ar.0.  A source followed by a
colon and a name is installed under that name, as its Makefile installs
it: games/phantasia/phant.nr:phantasia is phantasia.0.  A page that is
named takes the place of one of its name that a Makefile stood for: the
system's a.out(5) is the 286's and not upstream's.  Blank lines and
what follows a # are ignored.

A page's `.so FILE` is put in its place before nroff reads the page, as
nroff would put it, from the directory the page is in, or from the tree's
usr/include when FILE is in /usr/include.  The other names upstream gives
a page are links to it, as the Makefile in the page's directory makes
them with `ln`; those of a page that is not on the system are left out.

/usr/man/whatis, which apropos(1) and whatis(1) read, is made as
usr/src/man/Makefile makes it: makewhatis.sed is run on every page and
what it writes is sorted, without repeats, by sort -u.  sed is the
machine's own, run in the C locale; the lines are sorted by their bytes,
as the system's sort sorts them.

In OUT: the directories with the pages, whatis, and man.proto, which
lists them for tools/mkfs.py as root.proto includes it.
"""
import glob
import os
import re
import shutil
import subprocess
import sys

SUFFIX = re.compile(r"\.(man|[1-8][a-z]?)(\.5)?$")
SO = re.compile(rb"\.so[ \t]+(\S+)")
VAR = re.compile(r"\$[({](\w+)[)}]")


def fail(msg):
    sys.exit("mkman.py: " + msg)


def pages(src, listing):
    """What LISTING asks for: (directory, page name, source) for each."""
    out = []
    named = set()
    for n, line in enumerate(open(listing), 1):
        w = line.split("#")[0].split()
        if not w:
            continue
        if len(w) < 2:
            fail("%s:%d: a directory and its pages" % (listing, n))
        for name in w[1:]:
            name, _, called = name.partition(":")
            path = os.path.join(src, name)
            if os.path.basename(name) == "Makefile":
                found = sources(path)
            else:
                found = [path]
            for path in found:
                if not os.path.exists(path):
                    fail("%s:%d: no %s" % (listing, n, path))
                base = os.path.basename(path)
                page = (called or SUFFIX.sub("", base)) + ".0"
                stood = os.path.basename(name) == "Makefile"
                if not stood:
                    named.add((w[0], page))
                out.append((w[0], page, path, stood))
    return [(d, page, path) for d, page, path, stood in out
            if not (stood and (d, page) in named)]


def sources(makefile):
    """The pages MAKEFILE formats: what its SRCS, SRCS1 and so on list."""
    v = makevars(open(makefile, encoding="latin-1").read()
                 .replace("\\\n", " ").split("\n"))
    d = os.path.dirname(makefile)
    return [os.path.join(d, s) for k in sorted(v) if re.match(r"SRCS\d*$", k)
            for s in v[k].split()]


def source(path, top):
    """PATH with each `.so FILE` in it replaced by FILE."""
    out = []
    for line in open(path, "rb").read().splitlines(True):
        m = SO.match(line)
        if not m:
            out.append(line)
            continue
        name = m.group(1).decode()
        if name.startswith("/usr/include/"):
            name = os.path.join(top, "usr", "include",
                                name[len("/usr/include/"):])
        elif name.startswith("/"):
            fail("%s: .so %s, which is not in the tree" % (path, name))
        else:
            name = os.path.join(os.path.dirname(path), name)
        out.append(source(name, top))
    return b"".join(out)


def makevars(lines):
    """A Makefile's variables, NAME= VALUE, as given."""
    v = {}
    for line in lines:
        m = re.match(r"(\w+)\s*=\s*(.*)$", line)
        if m and not line.startswith("\t"):
            v[m.group(1)] = m.group(2).strip()
    return v


def expand(s, v, depth=0):
    if depth > 10:
        return s
    return VAR.sub(lambda m: expand(v.get(m.group(1), ""), v, depth + 1), s)


def links(makefile):
    """The links the install of MAKEFILE makes among formatted pages:
    (directory, name, directory, name of the page), from `ln A.0 B.0`,
    alone or in a `for i in LIST; do ln A.0 DIR/$$i; done`."""
    text = open(makefile, encoding="latin-1").read().replace("\\\n", " ")
    lines = text.split("\n")
    v = makevars(lines)
    out = []

    def ln(cmd):
        w = cmd.split()
        if len(w) == 3 and w[0].lstrip("-") == "ln" and \
                w[1].endswith(".0") and w[2].endswith(".0"):
            a, b = os.path.split(w[1]), os.path.split(w[2])
            if a[0].startswith("/usr/man/") and b[0].startswith("/usr/man/"):
                out.append((b[0][9:], b[1], a[0][9:], a[1]))

    for line in lines:
        if not line.startswith("\t"):
            continue
        line = expand(line, v)
        loop = re.search(r"for (\w+) in ([^;]*);\s*do\s+(.*?);?\s*done", line)
        if loop:
            for word in loop.group(2).split():
                for cmd in loop.group(3).split(";"):
                    ln(cmd.replace("$$" + loop.group(1), word))
            continue
        for cmd in line.split(";"):
            ln(cmd)
    return out


def others(wanted):
    """The other names of the pages WANTED, as the Makefiles beside
    their sources link them: (directory, name) to the page's."""
    made = {}
    for d, name, path in wanted:
        if (d, name) in made:
            fail("%s/%s from both %s and %s" % (d, name, made[d, name], path))
        made[d, name] = path
    out = {}
    for mk in sorted({os.path.join(os.path.dirname(p), "Makefile")
                      for _, _, p in wanted}):
        if not os.path.exists(mk):
            continue
        for d, name, pd, page in links(mk):
            if (pd, page) not in made:
                continue
            if (d, name) in made:
                fail("%s links %s/%s, which is a page" % (mk, d, name))
            out[d, name] = (pd, page)
    return out


def tabs(path):
    """What manroff gives nroff besides -man: -h, tabs where it can
    space with them; and nothing where the Makefile beside PATH formats
    it with `nroff -man` and not manroff, as bin/ar's does."""
    makefile = os.path.join(os.path.dirname(path), "Makefile")
    if os.path.exists(makefile) and re.search(
            r"nroff -man\s+" + re.escape(os.path.basename(path)) + r"\s",
            open(makefile, encoding="latin-1").read()):
        return []
    return ["-h"]


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    nroff, src, listing, out = sys.argv[1:]
    nroff = os.path.abspath(nroff)
    top = os.path.dirname(os.path.dirname(os.path.abspath(src)))
    wanted = pages(src, listing)
    linked = others(wanted)

    for d in glob.glob(os.path.join(out, "cat*")):
        shutil.rmtree(d)
    made = {}
    for d, name, path in wanted:
        made[d, name] = path
        r = subprocess.run([nroff, "-man"] + tabs(path),
                           input=source(path, top),
                           capture_output=True, cwd=os.path.dirname(nroff))
        if r.returncode or r.stderr:
            fail("%s: %s" % (path, r.stderr.decode("latin-1").strip() or
                             "nroff exits %d" % r.returncode))
        os.makedirs(os.path.join(out, d), exist_ok=True)
        open(os.path.join(out, d, name), "wb").write(r.stdout)

    lines = []
    for d, name in sorted(made):
        if d.startswith("../"):
            continue
        r = subprocess.run(["sed", "-n", "-f",
                            os.path.join(src, "man", "makewhatis.sed"),
                            os.path.join(out, d, name)],
                           capture_output=True, env=dict(os.environ,
                                                         LC_ALL="C"))
        if r.returncode:
            fail("sed: " + r.stderr.decode("latin-1").strip())
        lines += r.stdout.splitlines(True)
    open(os.path.join(out, "whatis"), "wb").write(b"".join(sorted(set(lines))))

    with open(os.path.join(out, "man.proto"), "w") as f:
        f.write("# The manual's pages, made by tools/mkman.py from %s.\n"
                % os.path.relpath(listing, top))
        for d in sorted({d for d, _ in made} | {d for d, _ in linked}):
            f.write("d %s 755\n" % os.path.normpath("/usr/man/" + d))
        for d, name in sorted(made):
            f.write("f %s/%s 444 %s/%s\n" %
                    (os.path.normpath("/usr/man/" + d), name, d, name))
        for (d, name), (pd, page) in sorted(linked.items()):
            f.write("h /usr/man/%s/%s /usr/man/%s/%s\n" % (d, name, pd, page))
        f.write("f /usr/man/whatis 444 whatis\n")
    print("mkman.py: %d pages and %d links to them" % (len(made), len(linked)))


if __name__ == "__main__":
    main()
