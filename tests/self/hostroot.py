#!/usr/bin/env python3
"""Lay out, where the system is built, what its Makefiles are run in
there: for tests/self with HOST set (tests/self/host.sh).

usage: hostroot.py TOP SRC ROOT

SRC is what tools/srcmk.py wrote.  ROOT/usr/src gets the tree SRC/proto
describes, each file a link to the one it names; ROOT/usr/include is
the tree's; ROOT/lib and ROOT/usr/lib have the libraries of obj/nlib,
the small model's in them and the others' in m and l, as the system
has them.  The Makefiles srcmk.py wrote are copied, with ROOT before
each of /usr/src, /usr/include, /lib and /usr/lib.
"""
import os
import re
import sys

top, src, root = [os.path.abspath(a) for a in sys.argv[1:4]]
usrsrc = os.path.join(root, "usr", "src")
os.makedirs(usrsrc)
ABS = re.compile(r"(?<=[\s])/(usr/src|usr/include|usr/lib|lib)(?=[/\s])")

for line in open(os.path.join(src, "proto")):
    w = line.split()
    if not w:
        continue
    path = usrsrc + w[1]
    if w[0] == "d":
        os.makedirs(path, exist_ok=True)
    elif w[0] == "l":
        os.symlink(w[2], path)
    elif w[3].startswith(src + os.sep) and os.path.basename(path) == "Makefile":
        text = open(w[3]).read()
        open(path, "w").write(ABS.sub(lambda m: root + m.group(0), text))
    else:
        os.symlink(w[3], path)

os.symlink(os.path.join(top, "usr", "include"),
           os.path.join(root, "usr", "include"))
nlib = os.path.join(top, "obj", "nlib")
for d in ("lib", os.path.join("usr", "lib")):
    d = os.path.join(root, d)
    os.makedirs(d)
    for f in os.listdir(os.path.join(nlib, "s")):
        if os.path.isfile(os.path.join(nlib, "s", f)):
            os.symlink(os.path.join(nlib, "s", f), os.path.join(d, f))
    for m in "ml":
        os.symlink(os.path.join(nlib, m), os.path.join(d, m))
