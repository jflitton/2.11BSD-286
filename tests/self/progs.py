#!/usr/bin/env python3
"""What the system installed from the Makefiles of /usr/src against what
its root had, for tests/self/progs.sh.

usage: progs.py TOP OUT NAME DIR...

In OUT: before.list and after.list, the root file system before the
system made anything and after it installed what it made
(tools/fsck.py -l), and newer.list, the files it installed.  Each of
those is to have the contents and the mode it had before, but for the
manual's pages and the files of tests/make/differs; and each of the
root's programs whose sources are in a DIR (tests/make/programs.py) is
to be among them.
"""
import os
import sys

top, out, name = sys.argv[1:4]
dirs = sys.argv[4:]
sys.path.insert(0, os.path.join(top, "tests", "make"))
import programs  # noqa: E402


def listing(path):
    files = {}
    for line in open(path):
        w = line.split()
        if len(w) >= 6 and w[1] == "f":
            files[w[0]] = (w[2], w[3], w[5])
    return files


before = listing(out + "/before.list")
after = listing(out + "/after.list")
newer = [p for p in open(out + "/newer.list").read().split() if p in after]
differs = programs.Names(top + "/tests/make/differs")
source = programs.sources()
where = programs.installed(source)

bad = []
same = pages = owners = 0
for p in newer:
    if p.startswith("/usr/man/") or p.startswith("/usr/new/man/"):
        pages += 1
    elif p in differs:
        pass
    elif p not in before:
        bad.append("NOT THE ROOT'S %s" % p)
    elif after[p][2] != before[p][2]:
        bad.append("DIFFERS %s" % p)
    elif after[p][0] != before[p][0]:
        bad.append("MODE %s: %s, and the root's was %s" %
                   (p, after[p][0], before[p][0]))
    else:
        same += 1
        owners += after[p][1] != before[p][1]
for p, prog in sorted(where.items()):
    if programs.within(source[prog], dirs) and p not in newer and \
            p not in differs:
        bad.append("NOT INSTALLED %s" % p)
for line in sorted(bad):
    print(line, file=sys.stderr)
if bad:
    sys.exit("%s: %d files are not what the root had (%s/self.img, "
             "/usr/src/log.*)" % (name, len(bad), out))
print("%s: the system made and installed %d files from the Makefiles of "
      "%s, each what its root had; %d pages of the manual besides, not "
      "compared; %d have another owner or group than the root's" %
      (name, same, " ".join(dirs), pages, owners))
