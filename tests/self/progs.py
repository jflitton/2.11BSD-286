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
not make the same way; the day a page of the manual says it was
formatted; what the system changes by running (running, beside this);
and NAME.old, where a Makefile keeps the NAME it installs over.  A library
has the day it was made in it too, and whose its members were, and is
compared by its members, each by its name and its contents, and their
order, the table of contents among them.  And each of the root's
programs whose sources are in a DIR (tests/make/programs.py) is to be
among the files installed, and so is each of its libraries, where the
DIR is lib/libc, of /lib, or usr.lib, of /usr/lib, and each of its
pages of the manual whose source is in a DIR (programs.pages).
"""
import os
import re
import sys

top, out, name = sys.argv[1:4]
dirs = sys.argv[4:]
sys.path.insert(0, os.path.join(top, "tests", "make"))
sys.path.insert(0, os.path.join(top, "tools"))
import fsck  # noqa: E402
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


images = {"self": fsck.Fs(fsck.partition(out + "/self.img", "a")),
          "root": fsck.Fs(fsck.partition(out + "/root.fs", "a"))}


def contents(path, image):
    """What the file PATH has in it, in the file system IMAGE, self or
    root."""
    fs = images[image]
    n = fs.lookup(path)
    return fs.read(n, fs.inode(n))


DAY = re.compile(rb"Printed [0-9]+/[0-9]+/[0-9]+")


def formatted(p):
    """Whether the page P is the root's but for the day it was
    formatted."""
    return DAY.sub(b"Printed", contents(p, "self")) == \
        DAY.sub(b"Printed", contents(p, "root"))


def members(data):
    """A library's members, each its name and its contents, or None if
    it is not one."""
    if data[:8] != b"!<arch>\n":
        return None
    pos, out = 8, []
    while pos + 60 <= len(data):
        h = data[pos:pos + 60]
        size = int(h[48:58])
        name, d, n = h[:16].rstrip(), pos + 60, 0
        if name.startswith(b"#1/"):
            n = int(name[3:])
            name = data[d:d + n]
        out.append((name, data[d + n:pos + 60 + size]))
        pos += 60 + size + (size & 1)
    return out


def library(p):
    """Whether the library P is as it was, member for member."""
    a = members(contents(p, "self"))
    return a is not None and a == members(contents(p, "root"))


bad = []
same = pages = kept = libs = 0
for p in sorted(set(before) | set(after)):
    if p in running:
        continue
    b, a = before.get(p), after.get(p)
    page = p.startswith(("/usr/man/", "/usr/new/man/",
                         "/usr/local/man/")) and p.endswith(".0")
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
    elif a[3:] != b[3:] and not (a[0] == "f" and p in differs) \
            and not (a[0] == "f" and page and formatted(p)) \
            and not (a[0] == "f" and a[5:] == b[5:] and p.endswith(".a") and
                     library(p)):
        bad.append("DIFFERS %s" % p)
    elif p in newer and p.endswith(".a") and a[3:] != b[3:]:
        libs += 1
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
bad += ["NOT INSTALLED %s" % p
        for p in programs.libraries(sorted(before), dirs)
        if before[p][0] == "f" and p not in newer]
bad += ["NOT INSTALLED %s" % p for p, d in sorted(programs.pages().items())
        if programs.within(d, dirs) and p not in newer]
for line in sorted(bad):
    print(line, file=sys.stderr)
if bad:
    sys.exit("%s: %d of the root are not what it had (%s/self.img, "
             "/usr/src/log.*)" % (name, len(bad), out))
print("%s: the system made and installed %d files from the Makefiles of "
      "%s, each what its root had and whose, %d libraries with the "
      "members the root's had, and %d pages of the manual, each the "
      "root's but for the day it says, and whose; the rest of the root, "
      "%d names, as it "
      "was" % (name, same, " ".join(dirs), libs, pages, kept))
