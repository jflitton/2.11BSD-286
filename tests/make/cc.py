#!/usr/bin/env python3
"""The system's cc, where the system is built, for tests/make/run.sh.

usage: cc.py TOP OUT [cc's arguments]

Without -c, -E or -S it compiles and links as the system's cc does
(tools/cc -ccom), with the libraries of OUT/root; with one of them it
is the compiler alone, a file at a time (tools/ncc).  A file is named
to the compiler as the Makefile names it, as it is on the system.  -i,
-n and -z, the PDP-11's, are nothing here, as they are nothing to the
system's ld; -ltermlib and -ll are those libraries' other names.
"""
import os
import re
import subprocess
import sys

top, out = sys.argv[1:3]
args = sys.argv[3:]
root = out + "/root"
mode = None
o = None
files = []
rest = []
i = 0
while i < len(args):
    a = args[i]
    if a in ("-c", "-E", "-S"):
        mode = a
    elif a == "-o":
        i += 1
        o = args[i]
    elif a in ("-i", "-n", "-z", "-w", "-g"):
        pass
    elif a.endswith(".c") and not a.startswith("-"):
        files.append(a)
    else:
        rest.append(a)
    i += 1


# A library's other name, which is a link to it on the system.
OTHER = {"-ltermlib": "-ltermcap", "-ll": "-lln"}


def lib(a):
    if a in OTHER:
        return OTHER[a]
    for p in ("/usr/lib/", "/lib/"):
        if a.startswith(p):
            return root + a
    return a


def found(a):
    """An object that is not here is the one in a directory of the
    Makefile's VPATH, as the system's make has it: that make gives a
    command the name it found the file by."""
    if not a.endswith(".o") or "/" in a or os.path.exists(a):
        return a
    try:
        text = open("Makefile").read()
    except OSError:
        return a
    m = re.search(r"^VPATH\s*=\s*(\S+)", text, re.M)
    for d in m.group(1).split(":") if m else []:
        d = d.replace("/usr/src", root + "/usr/src", 1) \
            if d.startswith("/usr/src") else d
        if os.path.exists(os.path.join(d, a)):
            return os.path.join(d, a)
    return a


if mode is None:
    cmd = [top + "/tools/cc", "-ccom"] + [found(lib(a)) for a in rest] + files
    if o:
        cmd += ["-o", o]
    sys.exit(subprocess.call(cmd))
flags = [a for a in rest if a[:2] in ("-D", "-U", "-I", "-O", "-m", "-n")]
for f in files:
    cmd = [top + "/tools/ncc"] + flags
    if mode != "-c":
        cmd.append(mode)
    if o:
        cmd += ["-o", o]
    elif mode == "-E":
        cmd += ["-o", "/dev/stdout"]
    r = subprocess.call(cmd + [f])
    if r:
        sys.exit(r)
for f in [a for a in rest if a.endswith(".s")]:
    r = subprocess.call([top + "/tools/cc", "-ccom", "-c"] + flags + [f] +
                        (["-o", o] if o else []))
    if r:
        sys.exit(r)
