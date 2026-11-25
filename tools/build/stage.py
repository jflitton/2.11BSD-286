#!/usr/bin/env python3
"""Lay out what the system's Makefiles are run in where the system is
built, for tools/build/build.sh.

usage: stage.py TOP OUT

OUT/root/usr/src is a copy of the tree's usr/src: a copy and not
links, since a Makefile writes files the tree has (a version's number,
a header that a program of its own makes).  It is kept from one build
to the next with what was made in it, as the system's /usr/src is: a
file is copied again only if it has changed, with the time it has in
the tree, so that make makes again what is made of it, and one that the
tree no longer has is taken away (OUT/staged, the files copied).  The
directories that a library's Makefile puts each model's objects in, m
and l, are made, as tools/srcmk.py gives them to the system.  A
GNUmakefile, which is how a directory is made where the system is built
and which this machine's make would read first, is not copied.
OUT/root/usr/include is the tree's.

A Makefile is changed as it is copied where make itself is to find a
file the system has: /usr/include, where a file depends on a header
there, is OUT/root/usr/include; a directory of /usr/src in VPATH is the
one of OUT/root, and an object it names ./NAME.o, which the system's
make does not look for in VPATH and this machine's does, having taken
the ./ away, is named by its whole path (usr.bin/ranlib's misc.o,
which is not ar's); and anything else in the system's root that one
depends on, a library, a file or a directory that was installed, is the
one in OUT/dest, but in the Makefiles of the libraries, which install
them and name none they are made with.  A command it names by where
the system has it is seen to as it is run (sh.py), and where `make
install` puts a file is left as it is: the commands that are given such
a name put it in OUT/dest (commands.sh).  The directories whose
Makefiles are not what they were, or which have a file that is not, or
are the nearest above it with a Makefile, are listed in OUT/remade, a
line each, for build.sh to clean them first: a Makefile need not say
what a program is made of (bin's has `${STD}:' and no more, and upstream
made clean before it made all), and usr/src's own is not one to clean
for a file (the kernel's sources, whose Makefile says).

adventure's game is made by running a.out, which reads the game's text
and writes itself out with it, and only the 286 can run that: here its
Makefile makes a.out and no more, and installs what the 286 needs to
make the game, a.out, glorkz and a Makefile that does the rest
(adventure.mk), in /usr/games/lib/adventure, as the root has it
(tests/programs).
"""
import os
import re
import shutil
import sys

top, out = [os.path.abspath(a) for a in sys.argv[1:3]]
sys.path.insert(0, os.path.join(top, "tools"))
import srcmk  # noqa: E402
root = os.path.join(out, "root")
src = os.path.join(top, "usr", "src")
usrsrc = os.path.join(root, "usr", "src")
dest = os.path.join(out, "dest")

INC = re.compile(r"(?<![\w\"'=-])/usr/include(?=[/\s]|$)", re.M)
VPATH = re.compile(r"^(VPATH\s*=.*)$", re.M)
DOT = re.compile(r"(?<=[\s=])\./(?=[\w.+-]+\.o(?![\w.]))")
RULE = re.compile(r"^([^\t#=\n]*[^\t#=\n:]:)([^=\n]*)$", re.M)
SYS = re.compile(r"(?<=\s)/(?=(?:usr/(?!include|src|lib/lib)|etc/|var/|"
                 r"bin/|sbin/)\S)")
LIB = re.compile(r"(?<=[\s=])/(?=(?:usr/lib|lib)/(?:[ml]/)?(?:lib\S+\.a|"
                 r"crt0\.o)(?:\s|$))")
ADVENT = """advent:	a.out
	rm -f advent
	echo y | ./a.out
"""
ADVENTURE = """install: advent
	install -c -s -m 755 advent /usr/games/adventure
"""
KIT = """install: a.out
	install -c -s -m 755 a.out /usr/games/lib/adventure/a.out
	install -c -m 644 glorkz /usr/games/lib/adventure/glorkz
	install -c -m 644 %s /usr/games/lib/adventure/Makefile
""" % os.path.join(top, "tools", "build", "adventure.mk")


def makefile(text, rel, here):
    text = INC.sub(root + "/usr/include", text)
    text = DOT.sub(here + "/", text)
    text = VPATH.sub(lambda m: m.group(1).replace("/usr/src",
                                                  root + "/usr/src"), text)
    if rel.split("/")[0] not in ("lib", "usr.lib"):
        text = LIB.sub(dest + "/", text)
    if rel == "games/adventure":
        assert ADVENT in text and ADVENTURE in text, \
            "adventure's Makefile is not as it was"
        text = text.replace(ADVENT, "advent:\ta.out\n")
        text = text.replace(ADVENTURE, KIT)
    return RULE.sub(lambda m: m.group(1) +
                    SYS.sub(dest + "/", m.group(2)), text)


def maker(rel):
    """REL, or the nearest directory above it, that has a Makefile, but
    for usr/src itself: None."""
    while rel:
        if any(os.path.isfile(os.path.join(src, rel, m))
               for m in ("Makefile", "makefile")):
            return rel
        rel = os.path.dirname(rel)
    return None


def same(a, b):
    """Whether files A and B have the same contents."""
    if os.path.getsize(a) != os.path.getsize(b):
        return False
    with open(a, "rb") as f, open(b, "rb") as g:
        return f.read() == g.read()


before = set()
if os.path.exists(out + "/staged"):
    before = set(open(out + "/staged").read().split("\n")) - {""}
staged, remade = [], []
for dp, dn, fn in os.walk(src):
    dn.sort()
    rel = os.path.relpath(dp, src)
    rel = "" if rel == "." else rel
    to = os.path.join(usrsrc, rel)
    os.makedirs(to, exist_ok=True)
    for d in srcmk.models(rel):
        os.makedirs(os.path.join(usrsrc, d), exist_ok=True)
    # A link to a directory (sendmail's cf, cf.hosttable) is a link
    # there too: without it, `cd cf; make clean' makes clean where it is.
    for f in [d for d in dn if os.path.islink(os.path.join(dp, d))]:
        s, t = os.path.join(dp, f), os.path.join(to, f)
        staged.append(os.path.join(rel, f))
        if not os.path.islink(t) or os.readlink(t) != os.readlink(s):
            if os.path.lexists(t):
                os.unlink(t)
            os.symlink(os.readlink(s), t)
    for f in sorted(fn):
        s, t = os.path.join(dp, f), os.path.join(to, f)
        name = os.path.join(rel, f)
        if f == "GNUmakefile":
            continue
        staged.append(name)
        if os.path.islink(s):
            if not os.path.islink(t) or os.readlink(t) != os.readlink(s):
                if os.path.lexists(t):
                    os.unlink(t)
                os.symlink(os.readlink(s), t)
        elif f in ("Makefile", "makefile"):
            text = open(s, errors="surrogateescape").read()
            text = makefile(text, rel, to)
            if not os.path.isfile(t) or os.path.islink(t) or \
                    open(t, errors="surrogateescape").read() != text:
                if os.path.lexists(t):
                    if name in before:
                        remade.append(rel or ".")
                    os.unlink(t)
                open(t, "w", errors="surrogateescape").write(text)
        elif not os.path.isfile(t) or os.path.islink(t) or not same(s, t):
            if os.path.lexists(t):
                if name in before and maker(rel):
                    remade.append(maker(rel))
                os.unlink(t)
            shutil.copy2(s, t)
for name in sorted(before - set(staged)):
    t = os.path.join(usrsrc, name)
    if os.path.islink(t) or os.path.lexists(t) and not os.path.isdir(t):
        os.unlink(t)
with open(out + "/staged", "w") as f:
    f.write("".join(n + "\n" for n in staged))
with open(out + "/remade", "w") as f:
    f.write("".join(d + "\n" for d in dict.fromkeys(remade)))

inc = os.path.join(root, "usr", "include")
if not os.path.lexists(inc):
    os.symlink(os.path.join(top, "usr", "include"), inc)
