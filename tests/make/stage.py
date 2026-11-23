#!/usr/bin/env python3
"""Lay out what the system's Makefiles are run in where the system is
built, for tests/make/run.sh.

usage: stage.py TOP OUT

OUT/root/usr/src is a copy of the tree's usr/src but for the kernel
(sys): a copy and not links, since a Makefile writes files the tree has
(a version's number, a header that a program of its own makes).
OUT/root/usr/include is the tree's, and OUT/root/lib and usr/lib have
the libraries of obj/nlib as the system has them: the small model's,
and the others' in m and l.

A Makefile is changed as it is copied where make itself is to find a
file the system has: /usr/include, where a file depends on a header
there, is OUT/root/usr/include; a directory of /usr/src in VPATH is the
one of OUT/root; and anything else in the system's root that a target
depends on, a file or a directory that was installed, is the one in
OUT/dest.  A command it names by where the system has it is seen to as
it is run (sh.py), and where `make install` puts a file is left as it
is: the commands that are given such a name put it in OUT/dest
(commands.sh).
"""
import os
import re
import shutil
import subprocess
import sys

top, out = [os.path.abspath(a) for a in sys.argv[1:3]]
root = os.path.join(out, "root")
src = os.path.join(top, "usr", "src")
usrsrc = os.path.join(root, "usr", "src")

INC = re.compile(r"(?<![\w\"'=-])/usr/include(?=[/\s]|$)", re.M)


VPATH = re.compile(r"^(VPATH\s*=.*)$", re.M)
RULE = re.compile(r"^([^\t#=\n]*[^\t#=\n:]:)([^=\n]*)$", re.M)
SYS = re.compile(r"(?<=\s)/(?=(?:usr/(?!include|src)|etc/|var/|bin/|sbin/|"
                 r"lib/)\S)")


def makefile(text):
    text = INC.sub(root + "/usr/include", text)
    text = VPATH.sub(lambda m: m.group(1).replace("/usr/src",
                                                  root + "/usr/src"), text)
    return RULE.sub(lambda m: m.group(1) +
                    SYS.sub(out + "/dest/", m.group(2)), text)


os.makedirs(os.path.dirname(usrsrc), exist_ok=True)
subprocess.check_call(["cp", "-cR", src + "/", usrsrc])
shutil.rmtree(os.path.join(usrsrc, "sys"))
for dp, dn, fn in os.walk(usrsrc):
    for f in fn:
        t = os.path.join(dp, f)
        if f in ("Makefile", "makefile") and not os.path.islink(t):
            text = open(t, errors="surrogateescape").read()
            open(t, "w", errors="surrogateescape").write(makefile(text))

if not os.path.lexists(os.path.join(root, "usr", "include")):
    os.symlink(os.path.join(top, "usr", "include"),
               os.path.join(root, "usr", "include"))
nlib = os.path.join(top, "obj", "nlib")
for d in ("lib", os.path.join("usr", "lib")):
    d = os.path.join(root, d)
    if os.path.isdir(d):
        continue
    os.makedirs(d)
    for f in os.listdir(os.path.join(nlib, "s")):
        if os.path.isfile(os.path.join(nlib, "s", f)):
            os.symlink(os.path.join(nlib, "s", f), os.path.join(d, f))
    for m in "ml":
        os.symlink(os.path.join(nlib, m), os.path.join(d, m))
