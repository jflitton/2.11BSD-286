#!/usr/bin/env python3
"""Put in the root what it has of 2.11BSD's distribution that nothing of
/usr/src makes, for tools/build/build.sh.

usage: tape.py TOP OUT

tape.proto, beside this, lists it, as mkfs.py's lines: each file is
copied from TOP into OUT/dest, and a line of OUT/meta says its mode,
owner and group, as install.py's do; a symbolic link is made there.
"""
import os
import shutil
import sys

top, out = sys.argv[1:3]
dest = out + "/dest"
here = os.path.dirname(os.path.abspath(__file__))
with open(out + "/meta", "a") as meta:
    for line in open(os.path.join(here, "tape.proto")):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        t = dest + w[1]
        os.makedirs(os.path.dirname(t), exist_ok=True)
        if os.path.lexists(t):
            os.unlink(t)
        if w[0] == "l":
            os.symlink(w[2], t)
            continue
        mode, _, who = w[2].partition(":")
        owner, _, group = who.rpartition(".")
        shutil.copyfile(os.path.join(top, w[3]), t)
        os.chmod(t, int(mode, 8) & 0o777 | 0o600)
        meta.write("%s %s %s %s %d -\n" % (w[1], mode, owner or "0",
                                           group or "0", os.stat(t).st_ino))
