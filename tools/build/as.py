#!/usr/bin/env python3
"""The system's as, where the system is built with the cross compiler,
for tools/build/build.sh (TOOLS=cross).

usage: as.py TOP OUT MODE [-o object] [-Dname[=text]] [-Idir] [-ms|-mm|-ml]
             file.s

A file is assembled as the system's assembler would, but by NASM, into
the cross compiler's objects (OMF), which its linker reads: what the
system's as takes is NASM's syntax (docs/toolchain.md), and -D and -I
are NASM's too, but that a directory there ends with a slash.  The model
is nothing to NASM: a file says FAR and FARDATA for it, which the
library's Makefiles give it.  The C library's conventions,
lib/libc/i286/crt/DEFS.inc, are found as the system's cc has the
assembler find them, in /usr/include.  MODE is cross or xlib, as
cc.py's: with cross, in the directories of native.py, whose objects are
the system's own, it is the system's as, as compiled for this machine.
"""
import os
import sys

top, out, mode = sys.argv[1:4]
args = sys.argv[4:]
src = os.path.join(out, "root", "usr", "src")
here = os.path.relpath(os.getcwd(), src)
sys.path.insert(0, os.path.join(top, "tools/build"))
from native import native  # noqa: E402
if mode == "cross" and native(here):
    os.execv(top + "/obj/host/ccom/as", ["as"] + args)
cmd = ["nasm", "-f", "obj",
       "-I" + top + "/usr/src/lib/libc/i286/crt/"]
o = None
files = []
i = 0
while i < len(args):
    a = args[i]
    if a == "-o":
        i += 1
        o = args[i]
    elif a in ("-ms", "-mm", "-ml"):
        pass
    elif a == "-b":
        sys.exit("as: -b is for the boot blocks, which NASM makes "
                 "(-f bin) where they are made")
    elif a[:2] == "-D":
        cmd.append(a if len(a) > 2 else "-D" + args[i + 1])
        i += 0 if len(a) > 2 else 1
    elif a[:2] == "-I":
        d = a[2:] if len(a) > 2 else args[i + 1]
        i += 0 if len(a) > 2 else 1
        cmd.append("-I" + d.rstrip("/") + "/")
    elif a.startswith("-"):
        sys.exit("as: unknown flag %s" % a)
    else:
        files.append(a)
    i += 1
if len(files) != 1:
    sys.exit("usage: as [-o object] [-Dname] [-Idir] [-ms|-mm|-ml] file.s")
f = files[0]
o = o or os.path.splitext(os.path.basename(f))[0] + ".o"
os.execvp("nasm", cmd + ["-o", o, f])
