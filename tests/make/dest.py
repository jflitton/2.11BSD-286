#!/usr/bin/env python3
"""Make the root's directories in OUT/dest, for tests/make/run.sh.

usage: dest.py OUT

Each directory of the root file system's list (OUT/root.list) is made
in OUT/dest before anything is installed, as it is there on the system.
OUT/dirs has their names, a line each; OUT/dirs.ino the name and the
number of the inode of each, which tests/make/check.py looks at to see
whether the Makefiles made one anew.
"""
import os
import sys

out = sys.argv[1]
dest = out + "/dest"
names = [w[0] for w in (line.split() for line in open(out + "/root.list"))
         if len(w) >= 2 and w[1] == "d"]
for d in names:
    os.makedirs(dest + d, exist_ok=True)
with open(out + "/dirs", "w") as f:
    f.write("".join(d + "\n" for d in names))
with open(out + "/dirs.ino", "w") as f:
    f.write("".join("%s %d\n" % (d, os.stat(dest + d).st_ino)
                    for d in names))
