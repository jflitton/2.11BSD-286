#!/usr/bin/env python3
"""The system's chown(8) and chgrp(1), where the system is built, for
tests/make/run.sh.

usage: chown.py OUT chown|chgrp [-R] [-f] OWNER[.GROUP]|GROUP file...

Only the system gives a file away.  For each file or directory that is
one of the root, in OUT/dest, and with -R each under a directory there,
this adds a line to OUT/dest.meta as install.py does: its name, = for
the mode it has, the owner and group, with = for the one that is not
changed, and the number of its inode, which says it is the one given
away; tests/make/check.py reads them in order after install's.
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
        if f.startswith("/") and not f.startswith(dest + "/"):
            f = dest + f
        t = os.path.abspath(f)
        if not t.startswith(dest + "/") or not os.path.lexists(t):
            continue
        found = [t]
        if deep and os.path.isdir(t):
            found += [os.path.join(dp, n) for dp, dn, fn in os.walk(t)
                      for n in dn + fn]
        for t in found:
            m.write("%s = %s %s %d\n" % (t[len(dest):], owner, group,
                                         os.lstat(t).st_ino))
