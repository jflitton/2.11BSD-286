#!/usr/bin/env python3
"""Say what of tests/self a change calls for on the 286.

usage: select.py MAKEFILE < NAMES

NAMES is the files changed, a line each, as git names them.  MAKEFILE is
the one tools/srcmk.py wrote for /usr/src/i286, which says what each
program and library is made of.  What is written is for the shell:

    PARTS="..."     the parts of tests/self to make there
    ONLY="..."      of the programs, these
    CCOM=1          and the system is to make its compiler (tests/ccom)
    WHY="..."       what led to that

A file that the tools which do the making are made of, or a header
every program reads, calls for the sample: the compiler made by itself,
the kernel, and a program of each model.  The Makefiles and what writes
them call for the programs the host cannot check (tests/self/host.sh),
since they are made with programs the Makefiles make and run.  Any
other source calls for what is made of it.  Nothing here calls for the
manual and the other files that are not programs: PARTS=data is for
whoever changed them to ask for.
"""
import re
import sys

TOOLS = ("usr/src/lib/ccom/", "usr/src/lib/cpp/", "usr/src/lib/c2/",
         "usr/src/bin/as/", "usr/src/bin/ld/", "usr/src/bin/cc.c",
         "usr/src/bin/make/", "usr/src/bin/sh/", "usr/src/bin/ar/",
         "usr/src/usr.bin/yacc/", "usr/src/usr.bin/lex/",
         "usr/src/usr.bin/ranlib", "usr/include/")
BUILD = ("tools/srcmk.py", "tools/cc", "tools/ncc", "tests/self/",
         "usr/src/i286/Makefile", "usr/src/sys/PC/Makefile",
         "usr/src/sys/i286stand/Makefile", "usr/src/lib/libc/i286/Makefile")
SAMPLE = ["cat", "csh", "make"]
UNSEEN = ["ex", "tcsh", "awk", "battlestar", "hack"]

# What each target of the Makefile is made of, through what is made on
# the way (a grammar's C).
rules = {}
text = open(sys.argv[1]).read().replace("\\\n", " ")
for m in re.finditer(r"^(\S+):[ \t]*(.*)$", text, re.M):
    rules.setdefault(m.group(1), []).extend(m.group(2).split())


def sources(target, seen):
    for p in rules.get(target, ()):
        if p in seen:
            continue
        seen.add(p)
        if p in rules:
            sources(p, seen)
    return seen


made_of = {}
for t in rules:
    if t.startswith("bin/") or re.match(r"lib/[sml]/lib.*\.a$", t):
        for p in sources(t, set()):
            if p.startswith("$S/"):
                made_of.setdefault("usr/src/" + p[3:], set()).add(t)

parts, only, why = set(), set(), []
ccom = False
for name in sys.stdin.read().split("\n"):
    if not name:
        continue
    if name.startswith(TOOLS):
        ccom = True
        parts.update(("kernel", "progs"))
        only.update(SAMPLE)
        why.append(name + ": the tools")
    elif name.startswith(BUILD):
        parts.add("progs")
        only.update(UNSEEN)
        why.append(name + ": how it is made")
    elif name.startswith("usr/src/sys/i286stand/"):
        parts.add("stand")
        why.append(name + ": the boot programs")
    elif name.startswith("usr/src/lib/libc/"):
        parts.add("libc")
        why.append(name + ": the C library")
    elif name in made_of:
        for t in made_of[name]:
            if t.startswith("bin/"):
                parts.add("progs")
                only.add(t[4:])
            else:
                parts.add("libs")
        why.append(name + ": what is made of it")
    elif name.startswith("usr/src/sys/"):
        parts.add("kernel")
        why.append(name + ": the kernel")
order = ["stand", "kernel", "libc", "libs", "progs"]
print('PARTS="%s"' % " ".join(p for p in order if p in parts))
print('ONLY="%s"' % " ".join(sorted(only)))
print("CCOM=%s" % ("1" if ccom else ""))
print('WHY="%s"' % "; ".join(why[:12]).replace('"', ""))
