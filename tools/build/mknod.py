#!/usr/bin/env python3
"""The system's mknod(8), where the system is built, for MAKEDEV
(tools/build/commands.sh).

usage: mknod.py OUT NAME b|c MAJOR MINOR

Only the system makes a device.  NAME is made a file, which chmod and
chgrp then see to as any other (chmod.py, chown.py), and a line of
OUT/devices says it is the device: the number of its inode, b or c, and
the two numbers.  Its mode is as mknod(2) gives it, 666 less the umask,
which a line of OUT/meta says, as install.py's do.
"""
import os
import sys

out = sys.argv[1]
a = sys.argv[2:]
if len(a) != 4 or a[1] not in ("b", "c"):
    sys.exit("usage: mknod name b/c major minor")
name, kind, major, minor = a
dest = out + "/dest"
t = os.path.abspath(name)
if not t.startswith(dest + "/"):
    sys.exit("mknod: %s is not in the root being made" % name)
if os.path.lexists(t):
    sys.exit("mknod: %s: File exists" % name)
mask = os.umask(0)
os.umask(mask)
open(t, "w").close()
ino = os.stat(t).st_ino
with open(out + "/devices", "a") as f:
    f.write("%d %s %d %d\n" % (ino, kind, int(major), int(minor)))
with open(out + "/meta", "a") as f:
    f.write("%s %o = = %d -\n" % (t[len(dest):], 0o666 & ~mask, ino))
