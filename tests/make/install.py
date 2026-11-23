#!/usr/bin/env python3
"""The system's install(1), where the system is built, for
tests/make/run.sh.

usage: install.py OUT [-c] [-s] [-m mode] [-o owner] [-g group] file... to

The root is OUT/dest: to, which is a name in the system's root, is the
file or the directory there, a directory if the root file system has
one of the name (OUT/dirs).  What install would have made the mode,
owner and group is a line of OUT/dest.meta, since only the system can
give a file to another user: to mode owner group, with install's own
where it is told none.  -s is the system's strip (tools/strip.py).
"""
import os
import shutil
import subprocess
import sys

out = sys.argv[1]
a = sys.argv[2:]
mode, owner, group, strip = "755", "root", "staff", False
files = []
i = 0
while i < len(a):
    x = a[i]
    if x == "-c":
        pass
    elif x == "-s":
        strip = True
    elif x in ("-m", "-o", "-g", "-f"):
        i += 1
        if x == "-m":
            mode = a[i]
        elif x == "-o":
            owner = a[i]
        elif x == "-g":
            group = a[i]
    else:
        files.append(x)
    i += 1
if len(files) < 2:
    sys.exit("install: usage: install [-cs] [-m mode] [-o owner] "
             "[-g group] file ... to")
to = files.pop().rstrip("/") or "/"
if not to.startswith("/"):
    sys.exit("install: %s: not a name in the root" % to)
dirs = open(out + "/dirs").read().split()
isdir = to in dirs or os.path.isdir(out + "/dest" + to)
if len(files) > 1 and not isdir:
    sys.exit("install: %s: not a directory" % to)
for f in files:
    name = to + "/" + os.path.basename(f) if isdir else to
    t = out + "/dest" + name
    os.makedirs(os.path.dirname(t), exist_ok=True)
    if os.path.lexists(t):
        os.unlink(t)
    shutil.copyfile(f, t)
    if strip and subprocess.call(["strip", t]):
        sys.exit(1)
    with open(out + "/dest.meta", "a") as m:
        m.write("%s %s %s %s\n" % (name, mode, owner, group))
