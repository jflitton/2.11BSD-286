#!/usr/bin/env python3
"""Write what the system needs to make itself: its /usr/src.

usage: srcmk.py OUT

OUT/proto, for mkfs.py, is the file system that is to be /usr/src
there: the sources, in the directories they have here.  They are made
there from the Makefiles of their own directories, as they are given:
the libraries, the programs, what is not a program and the manual from
upstream's, those of the directories usr/src's own Makefile names, which
`make build` there makes (LIBDIR and SRCDIR), and of those their
Makefiles name in turn; the kernel from the Makefile config writes in
sys/PC (sys/conf/config); and /boot and the boot blocks from
sys/i286stand's.  The directories a Makefile puts each model's objects
in, m and l, as upstream's put the profiled ones in profiled, are there
and empty.

What make makes there is left where it is run, as upstream's Makefiles
leave it.  Each is to be, to the byte, what is made of the same sources
here (tests/self).
"""
import os
import re
import sys

TOP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(TOP, "usr", "src")

# What the system is given of usr/src besides what usr/src's Makefile
# makes: each directory with the files in it, and not what is in
# directories below.
DIRS = ["sys/h", "sys/i286", "sys/i286at", "sys/sys", "sys/net",
        "sys/netinet", "sys/i286if", "sys/conf", "sys/PC", "sys/i286stand"]
# Directories that it is given the headers of and nothing else: the
# kernel's sources name some of the PDP-11's.
HEADERS = ["sys/pdp", "sys/pdpif", "sys/pdpuba"]
# Names in those for other directories.
LINKS = ["sys/machine"]


# The directories usr/src's Makefile makes are walked from those it
# names, but for the C library's, which is given whole: its Makefiles
# name their directories by macros, and the 286's name the PDP-11's C
# (lib/libc/i286/gen).
WHOLE = ["lib/libc"]
# A Makefile that puts the medium model's objects in m and the large's
# in l, as upstream's put the profiled ones in profiled, which its
# directory has.
MODELS = re.compile(r"^\t.*(?:\bcd [ml];|[\s(][ml]/)", re.M)


def models(d):
    """The directories D's Makefile puts each model's objects in."""
    p = makefile(d)
    if p and MODELS.search(open(p, encoding="latin-1").read()):
        return [d + "/m", d + "/l"]
    return []


def makefile(d):
    """D's Makefile, makefile as make reads it first, or None."""
    for m in ("makefile", "Makefile"):
        p = os.path.join(SRC, d, m)
        if os.path.isfile(p):
            return p
    return None


def text(d):
    """D's Makefile, its lines continued joined."""
    return open(makefile(d), encoding="latin-1").read().replace("\\\n", " ")


def entered(t):
    """The names of the directories that the Makefile T, as text() has
    it, goes into from a rule of its own to run make, as identd's does
    in @(cd src ; make ...)."""
    return set(re.findall(r"^\t@?\(?\s*cd ([\w.+-]+)\s*[;&].*\bmake\b",
                          t, re.M))


def named(t):
    """The names of the directories that the Makefile T, as text() has
    it, goes into by a rule's target, cd $@, where the targets are not
    SUBDIR's but macros of their own, as ucb/pascal's ${ALL} is."""
    macros = {m.group(1): m.group(2).split()
              for m in re.finditer(r"^(\w+)\s*=(.*)$", t, re.M)}

    def expand(words, depth=0):
        out = []
        for w in words:
            m = re.fullmatch(r"\$[({](\w+)[)}]", w)
            if m and depth < 8:
                out += expand(macros.get(m.group(1), []), depth + 1)
            else:
                out.append(w)
        return out

    names = set()
    for m in re.finditer(r"^([^\t#:=\n]+):[^\n]*\n\t@?\(?\s*cd \$@\s*[;&]",
                         t, re.M):
        names.update(expand(m.group(1).split()))
    return names


def subdirs(d):
    """The names of the directories D's Makefile has make run in: those
    it names in SUBDIR, or in DIRS as man's does, or in LIBDIR and SRCDIR
    as usr/src's does, or those it enters, by name or by a rule's target,
    but for a directory given whole, whose own Makefile is what makes
    what is below it; and the text of the Makefile."""
    t = text(d)
    names = set()
    for m in re.finditer(r"^(?:SUBDIRS?|DIRS|LIBDIR|SRCDIR)\s*=(.*)$", t,
                         re.M):
        names.update(m.group(1).split())
    if d not in WHOLE:
        names |= named(t)
    return names | entered(t), t


def walked(d, entered=None):
    """The directories make is in when it is run in D, D among them,
    and so on from theirs, each added to ENTERED; and what is under any
    of them with no Makefile of its own, which is its files, and a
    directory whose files the Makefile names, which has a Makefile of
    its own for where the system is built (bin/as/i286)."""
    out = [d]
    if entered is not None:
        entered.append(d)
    names, text = subdirs(d)
    for name in sorted(os.listdir(os.path.join(SRC, d))):
        sub = d + "/" + name
        if not os.path.isdir(os.path.join(SRC, sub)):
            continue
        if makefile(sub):
            if name in names:
                out += walked(sub, entered)
            elif re.search(r"(?<![\w.+/-])%s/" % re.escape(name), text):
                out.append(sub)
        else:
            out += [os.path.relpath(dp, SRC)
                    for dp, _, _ in os.walk(os.path.join(SRC, sub))]
    return out


def top(entered=None):
    """The directories make is in when usr/src's Makefile has it make
    each that it names, or that are given whole, and those whose files
    their Makefiles name (walked)."""
    out = []
    names, text = subdirs(".")
    for d in sorted(names):
        out += walked(d, entered)
    for d in WHOLE:
        out += [os.path.relpath(dp, SRC)
                for dp, _, _ in os.walk(os.path.join(SRC, d))]
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    dirs = set(DIRS) | set(top())
    empty = []
    for d in sorted(dirs):
        empty += models(d)
    proto = []
    seen = set()

    def directory(d):
        if d and d not in seen:
            directory(os.path.dirname(d))
            seen.add(d)
            proto.append("d /%s 755" % d)

    for d in sorted(dirs):
        directory(d)
        for name in sorted(os.listdir(os.path.join(SRC, d))):
            rel = d + "/" + name
            p = os.path.join(SRC, rel)
            # A name for another file is given what that file has in
            # it, which may be where the system has nothing.  What the
            # cross compiler says of a file is left beside it.
            # One that is run, a script a Makefile has, is to be run
            # there.
            if os.path.isfile(p) and not name.endswith(".err"):
                proto.append("f /%s %s %s" % (
                    rel, "755" if os.access(p, os.X_OK) else "644", p))
    for d in HEADERS:
        directory(d)
        for name in sorted(os.listdir(os.path.join(SRC, d))):
            if name.endswith(".h"):
                proto.append("f /%s/%s 644 %s" % (
                    d, name, os.path.join(SRC, d, name)))
    for d in empty:
        directory(d)
    for l in LINKS:
        directory(os.path.dirname(l))
        proto.append("l /%s %s" % (l, os.readlink(os.path.join(SRC, l))))
    # usr/src's own Makefile, which make build is run by.
    proto.append("f /Makefile 644 %s" % os.path.join(SRC, "Makefile"))
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "proto"), "w") as f:
        f.write("\n".join(proto) + "\n")


if __name__ == "__main__":
    main()
