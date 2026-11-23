#!/usr/bin/env python3
"""What the system installed from the Makefiles of /usr/src against what
its root had, for tests/self/progs.sh.

usage: progs.py TOP OUT NAME DIR...

In OUT: before.list and after.list, the root file system before the
system made anything and after it installed what it made
(tools/fsck.py -l), and newer.list, the files it installed.  Each name
of the root is to be after as it was before: a file with its contents,
mode, owner and group, and the names it is linked by, a directory with
its mode, owner and group, a device the same and its numbers, a
symbolic link with what it names.  Nothing of the root is to be gone,
and nothing is to be there that the root did not have.  But for: the
contents of the files of tests/make/differs, which the two builds do
not make the same way, and of a page of the manual, which has the day
in it; what the system changes by running (running, beside this); and
NAME.old, where a Makefile keeps the NAME it installs over.  And each
of the root's programs whose sources are in a DIR (tests/make/programs.py)
is to be among the files installed.
"""
import os
import sys

top, out, name = sys.argv[1:4]
dirs = sys.argv[4:]
sys.path.insert(0, os.path.join(top, "tests", "make"))
import programs  # noqa: E402


def listing(path):
    """Each name's type, mode, owner and group, and the rest: a file's
    size, sum and the first of its names; a device's numbers; what a
    symbolic link names."""
    entries = {}
    for line in open(path):
        w = line.split()
        if len(w) >= 2:
            entries[w[0]] = w[1:]
    return entries


before = listing(out + "/before.list")
after = listing(out + "/after.list")
newer = set(p for p in open(out + "/newer.list").read().split()
            if p in after)
differs = programs.Names(top + "/tests/make/differs")
running = programs.Names(os.path.join(os.path.dirname(
    os.path.abspath(__file__)), "running"))
source = programs.sources()
where = programs.installed(source)

bad = []
same = pages = kept = 0
for p in sorted(set(before) | set(after)):
    if p in running:
        continue
    b, a = before.get(p), after.get(p)
    page = p.startswith(("/usr/man/", "/usr/new/man/")) and \
        p.endswith(".0")
    if a is None:
        bad.append("GONE %s" % p)
    elif b is None:
        if p not in differs and not (p.endswith(".old") and
                                     p[:-4] in newer):
            bad.append("NOT THE ROOT'S %s" % p)
    elif a[0] != b[0] or a[0] == "l" and a != b:
        bad.append("DIFFERS %s: %s, and the root's was %s" %
                   (p, " ".join(a), " ".join(b)))
    elif a[0] == "l":
        kept += 1
    elif a[1] != b[1]:
        bad.append("MODE %s: %s, and the root's was %s" % (p, a[1], b[1]))
    elif a[2] != b[2]:
        bad.append("OWNER %s: %s, and the root's was %s" % (p, a[2], b[2]))
    elif a[3:] != b[3:] and not (a[0] == "f" and (page or p in differs)):
        bad.append("DIFFERS %s" % p)
    elif p in newer and page:
        pages += 1
    elif p in newer:
        same += 1
    else:
        kept += 1
for p, prog in sorted(where.items()):
    if programs.within(source[prog], dirs) and p not in newer and \
            p not in differs:
        bad.append("NOT INSTALLED %s" % p)
for line in sorted(bad):
    print(line, file=sys.stderr)
if bad:
    sys.exit("%s: %d of the root are not what it had (%s/self.img, "
             "/usr/src/log.*)" % (name, len(bad), out))
print("%s: the system made and installed %d files from the Makefiles of "
      "%s, each what its root had and whose, and %d pages of the manual "
      "with the mode and owner the root's had; the rest of the root, %d "
      "names, as it was" % (name, same, " ".join(dirs), pages, kept))
