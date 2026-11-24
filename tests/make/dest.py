#!/usr/bin/env python3
"""Make the root's directories in OUT/dest, for tests/make/run.sh.

usage: dest.py TOP OUT

Each directory of the root file system's list (OUT/root.list) is made
in OUT/dest before anything is installed, as it is there on the system.
OUT/dirs has their names, a line each; OUT/dirs.ino the name and the
number of the inode of each, which tests/make/check.py looks at to see
whether the Makefiles made one anew.  So is what the system has that no
Makefile installs and the Makefiles read: /usr/dict, of the tree's
usr/dict (usr/src/i286/root.proto), spell's and boggle's words and
refer's papers, with the mode, owner and group the root has, which
OUT/dest.meta says, as install.py's lines do.
"""
import os
import shutil
import sys

top, out = sys.argv[1:3]
dest = out + "/dest"
listed = {w[0]: w for w in (line.split() for line in open(out + "/root.list"))
          if len(w) >= 2}
names = [p for p, w in listed.items() if w[1] == "d"]
for d in names:
    os.makedirs(dest + d, exist_ok=True)
with open(out + "/dest.meta", "a") as meta:
    for line in open(top + "/usr/src/i286/root.proto"):
        w = line.split()
        if len(w) == 4 and w[0] == "f" and w[3].startswith("../../dict/"):
            shutil.copyfile(top + "/usr/" + w[3][len("../../"):],
                            dest + w[1])
            mode, owner = listed[w[1]][2:4]
            uid, gid = owner.split(".")
            meta.write("%s %s %s %s %d\n" % (w[1], mode, uid, gid,
                                             os.stat(dest + w[1]).st_ino))
with open(out + "/dirs", "w") as f:
    f.write("".join(d + "\n" for d in names))
with open(out + "/dirs.ino", "w") as f:
    f.write("".join("%s %d\n" % (d, os.stat(dest + d).st_ino)
                    for d in names))
