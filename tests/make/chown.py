#!/usr/bin/env python3
"""The system's chown(8) and chgrp(1), where the system is built, for
tests/make/run.sh.

usage: chown.py OUT chown|chgrp [-R] [-f] OWNER[.GROUP]|GROUP file...

Only the system gives a file away.  For each file that is one install
put in the root, OUT/dest, or with -R is under a directory there, this
adds a line to OUT/dest.meta as install.py does, the file's name, = for
the mode it has, and the owner and group, with = for the one that is
not changed; tests/make/check.py reads them after install's.
"""
import os
import sys

out, command = sys.argv[1:3]
a = [x for x in sys.argv[3:] if x not in ("-R", "-f")]
deep = "-R" in sys.argv[3:]
if len(a) < 2:
    sys.exit("usage: %s [-R] [-f] owner file ..." % command)
if command == "chgrp":
    owner, group = "=", a[0]
else:
    owner, _, group = a[0].partition(".")
    group = group or "="
dest = out + "/dest"
with open(out + "/dest.meta", "a") as m:
    for f in a[1:]:
        if not f.startswith("/"):
            continue
        t = dest + "/" + f.strip("/")
        if os.path.isdir(t):
            found = [os.path.join(dp, n) for dp, _, fn in os.walk(t)
                     for n in fn] if deep else []
        else:
            found = [t] if os.path.isfile(t) else []
        for t in found:
            m.write("%s = %s %s\n" % (t[len(dest):], owner, group))
