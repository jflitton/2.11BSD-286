#!/usr/bin/env python3
"""The system's install(1), where the system is built, for
tools/build/build.sh.

usage: install.py OUT [-c] [-s] [-m mode] [-o owner] [-g group] file... to

The root is OUT/dest: to, which is a name in the system's root, or
that name in OUT/dest (sh.py), is the file or the directory there, a
directory if the root has one of the name (OUT/dirs) or one is there.
What install would have made the mode, owner and group is a line of
OUT/meta, since only the system can give a file to another user: to
mode owner group, with - where it is told none, when the file is
whoever runs install's and of its directory's group; the number of its
inode, as chown.py's; and the directory of usr/src whose Makefile
installed it, or - for what etc's distribution installs (DISTRIBUTION
in the environment), which is no directory's make install
(tests/self/built.py).  The file here has the mode but for setuid,
setgid and sticky, which this machine may not give it, and its owner
may read and write it.

-s is the system's strip (tools/strip.py), but with STRIP=no in the
environment, when the root's programs keep their symbols.  A program
is kept with its symbols as well, in OUT/syms under the name it is
installed by, for what reads them (tools/kprof.py).
"""
import os
import shutil
import subprocess
import sys

out = sys.argv[1]
a = sys.argv[2:]
mode, owner, group, strip = "755", "-", "-", False
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
to = files.pop()
if to.startswith(out + "/dest/"):
    to = to[len(out + "/dest"):]
to = "/" + to.strip("/")
dirs = open(out + "/dirs").read().split()
isdir = to in dirs or os.path.isdir(out + "/dest" + to)
if len(files) > 1 and not isdir:
    sys.exit("install: %s: not a directory" % to)
src = os.path.join(out, "root", "usr", "src")
here = os.path.relpath(os.getcwd(), src)
if here.startswith("..") or os.environ.get("DISTRIBUTION"):
    here = "-"
for f in files:
    name = to + "/" + os.path.basename(f) if isdir else to
    t = out + "/dest" + name
    os.makedirs(os.path.dirname(t), exist_ok=True)
    if os.path.lexists(t):
        os.unlink(t)
    shutil.copyfile(f, t)
    if strip:
        os.makedirs(out + "/syms", exist_ok=True)
        shutil.copyfile(f, out + "/syms/" + os.path.basename(name))
        if os.environ.get("STRIP") != "no" and subprocess.call(["strip", t]):
            sys.exit(1)
    os.chmod(t, int(mode, 8) & 0o777 | 0o600)
    with open(out + "/meta", "a") as m:
        m.write("%s %s %s %s %d %s\n" % (name, mode, owner, group,
                                          os.stat(t).st_ino, here))
