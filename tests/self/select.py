#!/usr/bin/env python3
"""Say what of tests/self a change calls for on the 286.

usage: select.py < NAMES

NAMES is the files changed, a line each, as git names them.  What is
written is for the shell:

    PARTS="..."     what tests/self/run.sh is to make there: of stand
                    and kernel, /boot and the boot blocks, and the kernel
    DIRS="..."      the directories of usr/src that tests/self/progs.sh
                    is to have it run make in, in the order of usr/src's
                    Makefile
    CCOM=1          and the system is to make its compiler (tests/ccom)
    WHY="..."       what led to that

A file that the tools which do the making are made of, or a header
every program reads, calls for the sample: the compiler made by itself,
with its directories, the kernel, and a program of each model, sed, csh
and make.  The build where the system is built (tools/build), and what
writes the system's /usr/src, call for programs made with programs
their Makefiles make and run, which that build runs its own builds of,
and only the 286 its own.  Any other file of usr/src calls for what is
made of it: of a directory that make is run in when usr/src's Makefile
runs it (tools/srcmk.py), the deepest it is in, and each directory that
compiles it, as the build where the system is built did
(tests/self/built.py); of the kernel's sources and its configuration
(sys/conf, sys/PC), the kernel; and of /boot's, those.  A file of what
upstream has and does not make, which that build makes and does not
install (EXTRA in tools/build/build.sh), calls for nothing here: the
test that installs it tests it.
"""
import os
import sys

TOP = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(TOP, "tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import built  # noqa: E402
import srcmk  # noqa: E402

TOOLS = ("usr/src/lib/ccom/", "usr/src/lib/cpp/", "usr/src/lib/c2/",
         "usr/src/bin/as/", "usr/src/bin/ld/", "usr/src/bin/cc.c",
         "usr/src/bin/make/", "usr/src/bin/sh/", "usr/src/bin/ar/",
         "usr/src/usr.bin/yacc/", "usr/src/usr.bin/lex/",
         "usr/src/usr.bin/ranlib/", "usr/include/", "usr/src/sys/h/")
BUILD = ("tools/srcmk.py", "tools/cc", "tools/ncc", "tests/self/",
         "tools/build/")
SAMPLE = ["bin/sed", "bin/csh", "bin/make"]
UNSEEN = ["bin/awk", "bin/tcsh", "ucb/ex", "games/battlestar", "games/hack"]

# The directories make is run in, and the order of usr/src's Makefile:
# its libraries' and then the rest.
entered = []
srcmk.top(entered)
text = open(os.path.join(srcmk.SRC, "Makefile")).read()
order = []
for macro in ("LIBDIR", "SRCDIR"):
    for line in text.splitlines():
        if line.startswith(macro + "="):
            order += line.split("=", 1)[1].split()


# What the build makes and does not install (tools/build/build.sh).
extra = []
for line in open(os.path.join(TOP, "tools", "build", "build.sh")):
    if line.startswith("EXTRA="):
        extra += [d + "/" for d in line.split("=", 1)[1].split()]

# The directories each source is compiled in.
among = built.sources(built.build(TOP))


def directory(name):
    """The deepest directory make is run in that NAME, of usr/src, is
    in, or None."""
    d = os.path.dirname(name)
    while d:
        if d in entered:
            return d
        d = os.path.dirname(d)
    return None


parts, dirs, why = set(), set(), []
ccom = False
for name in sys.stdin.read().split("\n"):
    if not name:
        continue
    rel = name[len("usr/src/"):] if name.startswith("usr/src/") else None
    if name.startswith(TOOLS):
        ccom = True
        parts.add("kernel")
        dirs.update(SAMPLE)
        d = rel and directory(rel)
        if d and "/" in d:
            dirs.add(d)
        why.append(name + ": the tools")
    elif name.startswith(BUILD):
        dirs.update(UNSEEN)
        why.append(name + ": how it is made")
    elif rel and rel.startswith(tuple(extra)):
        why.append(name + ": made and not installed")
    elif name.startswith("usr/src/sys/i286stand/"):
        parts.add("stand")
        why.append(name + ": the boot programs")
    elif name.startswith("usr/src/sys/"):
        parts.add("kernel")
        why.append(name + ": the kernel")
    elif rel and (directory(rel) or rel in among):
        made = among.get(rel, set()) | set([directory(rel)]) - set([None])
        dirs.update(made)
        why.append(name + ": " + " ".join(sorted(made)))
# A directory under another that is made is made with it.
dirs = [d for d in dirs if not any(d.startswith(o + "/") for o in dirs)]


def place(d):
    first = d.split("/")[0]
    return (order.index(first) if first in order else len(order), d)


print('PARTS="%s"' % " ".join(p for p in ["stand", "kernel"] if p in parts))
print('DIRS="%s"' % " ".join(sorted(dirs, key=place)))
print("CCOM=%s" % ("1" if ccom else ""))
print('WHY="%s"' % "; ".join(why[:12]).replace('"', ""))
