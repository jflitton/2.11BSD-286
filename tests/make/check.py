#!/usr/bin/env python3
"""What `make install` put in the root against what the root file system
has, for tests/make/run.sh.

usage: check.py TOP OUT SECONDS DIR...

OUT/dest is where the Makefiles of the DIRs installed, OUT/dest.meta
the mode each file was to have (install.py), and OUT/root.list the
root file system's own list (tools/fsck.py -l).  A file that was
installed must be one the root has, with those contents and that mode.
A page of the manual says at its foot the day it was formatted: one
that differs from the root's is compared again without that.
Of the root's programs whose sources are in a DIR (programs.py), each
must have been installed; those of `left`, beside this, are made with a
program their Makefile makes and runs, and are the 286's to make.
Those of `differs` are made, and are not yet what the other build makes
of them.
"""
import hashlib
import os
import re
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import programs  # noqa: E402

top, out, took = sys.argv[1:4]
dirs = sys.argv[4:]

want = {}
for line in open(out + "/root.list"):
    w = line.split()
    if len(w) >= 6 and w[1] == "f":
        want[w[0]] = (w[2], w[5])
mode = {}
for line in open(out + "/dest.meta"):
    p, m = line.split()[:2]
    mode[p] = m.lstrip("0") or "0"
source = programs.sources()
where = programs.installed(source)
left = programs.Names(here + "/left")
differs = programs.Names(here + "/differs")



def page(p, t):
    """Whether P is a page of the manual and T, as it was installed,
    is the root's but for the day it was formatted."""
    if not p.startswith(("/usr/man/", "/usr/new/man/")):
        return False
    day = re.compile(rb"Printed [0-9]+/[0-9]+/[0-9]+")
    root = subprocess.run(
        [sys.executable, "-I", top + "/tools/fsck.py", "-x", p,
         top + "/obj/root/root.fs"], capture_output=True, check=True).stdout
    return day.sub(b"Printed", root) == day.sub(b"Printed",
                                                open(t, "rb").read())


bad = []
same = 0
dest = out + "/dest"
for dp, dn, fn in os.walk(dest):
    for f in fn:
        t = os.path.join(dp, f)
        p = t[len(dest):]
        if os.path.islink(t):
            continue
        if p not in want:
            if p not in differs:
                bad.append("NOT THE ROOT'S %s" % p)
        elif hashlib.sha256(open(t, "rb").read()).hexdigest()[:16] != \
                want[p][1]:
            if page(p, t):
                same += 1
            elif p not in left and p not in differs:
                bad.append("DIFFERS %s" % p)
        elif p in mode and mode[p] != want[p][0]:
            bad.append("MODE %s: %s, and the root's is %s" %
                       (p, mode[p], want[p][0]))
        else:
            same += 1
nleft = 0
for p, name in sorted(where.items()):
    if not programs.within(source[name], dirs) or os.path.isfile(dest + p):
        continue
    if p in left:
        nleft += 1
    else:
        bad.append("NOT INSTALLED %s" % p)
for line in sorted(bad):
    print(line, file=sys.stderr)
if bad:
    sys.exit("make: %d files are not what the root has (%s/log)" %
             (len(bad), out))
print("make: the Makefiles of %s installed %d files where the system is "
      "built, in %s s, each what the root has; %d programs are left to "
      "the 286" % (" ".join(dirs), same, took, nleft))
