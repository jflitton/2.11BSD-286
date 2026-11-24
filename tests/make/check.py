#!/usr/bin/env python3
"""What the root that was made has against what it is to have, for
tests/make/run.sh.

usage: tools/fsck.py -l root.fs | check.py LIST SECONDS [-u]

The root is listed as tools/fsck.py -l lists it: a name, its type, its
mode, its owner and group, and after them a file's size and sum, which
are left out here, a device's numbers, what a symbolic link names, and
= and the first of a file's names where it has more than one.  LIST is
the same of the root it is to be, without the size and the sum; with
-u it is written so.  The time made was in SECONDS.
"""
import sys

path, took = sys.argv[1:3]
update = sys.argv[3:] == ["-u"]


def entry(w):
    """W, a line of fsck.py -l, as LIST has it."""
    if w[1] == "f":
        return w[:4] + w[6:]
    return w


got = [" ".join(entry(line.split())) for line in sys.stdin if line.strip()]
if update:
    with open(path, "w") as f:
        f.write("".join(g + "\n" for g in got))
    print("make: %s has the root's %d names" % (path, len(got)))
    sys.exit(0)
want = [line.rstrip("\n") for line in open(path)]
gotn = {g.split()[0]: g for g in got}
wantn = {w.split()[0]: w for w in want}
bad = []
for n in sorted(set(gotn) | set(wantn)):
    if n not in wantn:
        bad.append("NOT LISTED   %s" % gotn[n])
    elif n not in gotn:
        bad.append("NOT MADE     %s" % wantn[n])
    elif gotn[n] != wantn[n]:
        bad.append("DIFFERS      %s\n    listed   %s" % (gotn[n], wantn[n]))
for line in bad:
    print(line, file=sys.stderr)
if bad:
    sys.exit("make: %d names are not as %s has them" % (len(bad), path))
print("make: the Makefiles made the root of %d names as the list has it, "
      "in %s s" % (len(got), took))
