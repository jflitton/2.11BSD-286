#!/usr/bin/env python3
"""The system's ld, where the system is built, for tools/build/build.sh.

usage: ld.py TOP OUT MODE LIBROOT [ld's arguments]

A Makefile that links a program itself, as qterm's does with ld,
/lib/crt0.o and -lc, has the system's ld, as compiled for this machine
(obj/host/ccom/ld), find each -l library where the system's finds it,
in /lib, /usr/lib and /usr/local/lib, here of the root LIBROOT; sh.py
has named the start-up code in OUT/dest already.  MODE and LIBROOT are
cc.py's.  The cross compiler's objects that ld does not read: a program
of them is linked as cc.py links one, which gives it the start-up code
and the C library of its own, and the Makefile's are left out.
"""
import os
import sys

top, out, mode, libroot = sys.argv[1:5]
args = sys.argv[5:]
here = os.path.relpath(os.getcwd(), out + "/root/usr/src")
cross = mode == "xlib" or mode == "cross" and \
    here.split("/")[0] not in ("lib", "usr.lib")
if not cross:
    if mode == "cross":
        libroot = out + "/dest"
    dirs = []
    for d in ("lib", "usr/lib", "usr/local/lib"):
        dirs += ["-L", os.path.join(libroot, d)]
    ld = os.path.join(top, "obj/host/ccom/ld")
    os.execv(ld, [ld] + dirs + args)
keep = [a for a in args if not a.endswith("/crt0.o") and a != "-lc"]
os.execv(sys.executable, [sys.executable, "-I",
         os.path.join(top, "tools/build/cc.py"), top, out, mode, libroot]
         + keep)
