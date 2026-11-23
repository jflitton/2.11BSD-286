#!/usr/bin/env python3
"""The system's chmod(1), where the system is built, for
tests/make/run.sh.

usage: chmod.py OUT [-R] [-f] MODE file...

A file that is one in the system's root is the one in OUT/dest, as
commands.sh's other commands have it.  This machine's chmod changes the
mode, and for each file or directory of OUT/dest, and with -R each
under a directory there, a line in OUT/dest.meta as chown.py's says
what MODE, a number or u+s and the like, was to be: this machine may
not give a file the bits of setuid, setgid and sticky, as the system
does.
"""
import os
import re
import subprocess
import sys

ROOT = re.compile(r"/(bin|sbin|etc|lib|usr|var)(/|$)")

out = sys.argv[1]
dest = out + "/dest"
a = []
for x in sys.argv[2:]:
    if x.startswith("//"):
        x = x[1:]
    if ROOT.match(x):
        x = dest + x
    a.append(x)
flags = [x for x in a if x in ("-R", "-f")]
rest = [x for x in a if x not in ("-R", "-f")]
status = subprocess.call(["/bin/chmod"] + flags + rest)
if len(rest) >= 2:
    mode = rest[0]
    with open(out + "/dest.meta", "a") as m:
        for f in rest[1:]:
            t = os.path.abspath(f)
            if not t.startswith(dest + "/") or not os.path.lexists(t):
                continue
            found = [t]
            if "-R" in flags and os.path.isdir(t):
                found += [os.path.join(dp, n) for dp, dn, fn in os.walk(t)
                          for n in dn + fn]
            for t in found:
                m.write("%s %s = = %d\n" % (t[len(dest):], mode,
                                            os.lstat(t).st_ino))
sys.exit(status)
