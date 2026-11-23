#!/usr/bin/env python3
"""The system's tar(1), where the system is built, for tests/make/run.sh.

usage: tar.py OUT KEY [ARG...]

This machine's tar does it.  Run by root, as make install is on the
system, tar x keeps the owner and group a file had where it was archived,
and what share/learn's Makefile archives is the system's /usr/src, whose
files are root's and wheel's: of each file and directory it writes in
OUT/dest, a line in OUT/dest.meta says so, as chown.py's do.  It reads
the archive from a file of its own, to list it.
"""
import os
import subprocess
import sys
import tempfile

out = sys.argv[1]
args = sys.argv[2:]
dest = out + "/dest"
if not args or "x" not in args[0]:
    os.execv("/usr/bin/tar", ["tar"] + args)
key, rest = args[0], args[1:]
if "f" not in key or rest[:1] != ["-"]:
    sys.exit("tar.py: x only from the standard input, xf -")
with tempfile.NamedTemporaryFile(dir=out) as a:
    a.write(sys.stdin.buffer.read())
    a.flush()
    status = subprocess.call(["/usr/bin/tar", key, a.name] + rest[1:])
    names = subprocess.run(["/usr/bin/tar", "tf", a.name], check=True,
                           capture_output=True, text=True).stdout.split()
with open(out + "/dest.meta", "a") as m:
    for n in names:
        t = os.path.abspath(n)
        if t.startswith(dest + "/") and os.path.lexists(t):
            m.write("%s = root wheel %d\n" % (t[len(dest):],
                                              os.lstat(t).st_ino))
sys.exit(status)
