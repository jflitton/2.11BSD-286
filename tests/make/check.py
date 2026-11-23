#!/usr/bin/env python3
"""What `make install` put in the root against what the root file system
has, for tests/make/run.sh.

usage: check.py TOP OUT SECONDS DIR...

OUT/dest is where the Makefiles of the DIRs installed, OUT/dest.meta
the mode each file was to have (install.py), and OUT/root.list the
root file system's own list (tools/fsck.py -l).  A file that was
installed must be one the root has, with those contents and that mode.
Of the root's programs whose sources are in a DIR (usr/src/i286's
Makefile: make table), each must have been installed; those of `left`,
beside this, are made with a program their Makefile makes and runs, and
are the 286's to make.  Those of `differs` are made, and are not yet
what the other build makes of them.
"""
import hashlib
import os
import subprocess
import sys

top, out, took = sys.argv[1:4]
dirs = sys.argv[4:]
here = os.path.dirname(os.path.abspath(__file__))

want = {}
for line in open(out + "/root.list"):
    w = line.split()
    if len(w) >= 6 and w[1] == "f":
        want[w[0]] = (w[2], w[5])
mode = {}
for line in open(out + "/dest.meta"):
    p, m = line.split()[:2]
    mode[p] = m.lstrip("0") or "0"

# Programs whose first source is one the Makefile of usr/src/i286 makes,
# in obj, and where their own sources are.
GENERATED = {
    "awk": "bin/awk", "tcsh": "bin/tcsh", "expr": "bin", "ex": "ucb/ex",
    "hack": "games/hack", "egrep": "usr.bin", "bc": "usr.bin",
    "eqn": "usr.bin/eqn", "neqn": "usr.bin/neqn",
    "style1": "usr.bin/diction", "style2": "usr.bin/diction",
    "style3": "usr.bin/diction",
}

# Where the root has each program, and the directory of its sources.
table = subprocess.run(["make", "-s", "-C", top + "/usr/src/i286", "table"],
                       capture_output=True, text=True, check=True).stdout
source = {}
for line in table.splitlines():
    if line.startswith("PROG "):
        head, flags, srcs, libs = line[5:].split("|")
        s = srcs.split()[0].replace(top + "/usr/src/", "")
        source[head.split()[0]] = GENERATED.get(head.split()[0], s)
where = {}
for line in open(top + "/usr/src/i286/root.proto"):
    w = line.split()
    if len(w) == 4 and w[0] == "f" and "/obj/root/bin/" in w[3]:
        where[w[1]] = os.path.basename(w[3])
left = set()
for line in open(here + "/left"):
    line = line.split("#")[0].split()
    if line:
        left.add(line[0])

differs = set()
for line in open(here + "/differs"):
    line = line.split("#")[0].split()
    if line:
        differs.add(line[0])

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
            if p not in left and p not in differs:
                bad.append("DIFFERS %s" % p)
        elif p in mode and mode[p] != want[p][0]:
            bad.append("MODE %s: %s, and the root's is %s" %
                       (p, mode[p], want[p][0]))
        else:
            same += 1
nleft = 0
for p, name in sorted(where.items()):
    s = source.get(name, "")
    if not any(s == d or s.startswith(d + "/") for d in dirs):
        continue
    if os.path.isfile(dest + p):
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
      "built, in %s s, each what the root has; %d are left to the 286" %
      (" ".join(dirs), same, took, nleft))
