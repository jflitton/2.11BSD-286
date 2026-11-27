#!/usr/bin/env python3
"""The system's cc, where the system is built, for tools/build/build.sh.

usage: cc.py TOP OUT MODE LIBROOT [cc's arguments]

Without -c, -E or -S it compiles and links as the system's cc does,
with the start-up code and the libraries of the root LIBROOT, as the
system's cc and ld find them there (tools/cc -sysroot); with one of
them it is the compiler alone.  A file is named to the compiler as the
Makefile names it, as it is on the system.  -i, -n and -z, the
PDP-11's, are nothing here, as they are nothing to the system's ld.

MODE says whose compiler it is:

    native  the system's own, as compiled for this machine (tools/cc
            -ccom, and tools/ncc a file at a time), with the libraries
            of the root being made, OUT/dest, which is LIBROOT
    cross   the cross compiler (tools/cc), with the libraries of
            LIBROOT, the cross compiler's, which the same Makefiles
            made with it (MODE xlib); but in the directories of
            native.py, the system's own, as for native, with
            OUT/dest's
    xlib    the cross compiler, for those libraries, with LIBROOT's

The cross compiler is told what the system's preprocessor defines of
itself, unix, BSD2_11 and __BSD2_11__ (lib/cpp/cpp.c), which programs
ask what system they are on by; a library a Makefile names by where the
system has it, which make finds in OUT/dest, is LIBROOT's; and xstr's
x.c, which a Makefile has compiled for each of its files and renames
the object of, is compiled under a name of its own: the cross compiler
names a medium or large program's code after the file, and its linker
puts what has one name in one segment.

Each C file it is given is written in OUT/sources, a line each, with the
directory of usr/src it is compiled in: the directory, the file, named
from usr/src (tests/self/select.py), and the model, s, m or l, and the
-D, -U and -I it is compiled with, an -I's directory by its whole name
(tools/layouts.py), each quoted as the shell would have it.
"""
import hashlib
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile

top, out, mode, libroot = sys.argv[1:5]
args = sys.argv[5:]
root = out + "/root"
dest = out + "/dest"
src = root + "/usr/src"
here = os.path.relpath(os.getcwd(), src)
sys.path.insert(0, os.path.join(top, "tools/build"))
from native import native  # noqa: E402
cross = mode == "xlib" or mode == "cross" and not native(here)
if mode == "cross" and not cross:
    libroot = dest
what = None
o = None
files = []
rest = []
i = 0
while i < len(args):
    a = args[i]
    if a in ("-c", "-E", "-S"):
        what = a
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


def lib(a):
    """A library the Makefile names by where the system has it, which is
    the one of LIBROOT."""
    for p in ("/usr/lib/", "/lib/"):
        if a.startswith(p):
            return libroot + a
    if cross and a.startswith(dest + "/") and a.endswith(".a"):
        return libroot + a[len(dest):]
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


if not here.startswith("..") and files:
    model = "s"
    given = []
    for a in rest:
        if a in ("-ms", "-mm", "-ml"):
            model = a[2]
        elif a[:2] == "-I" and len(a) > 2:
            given.append("-I" + os.path.abspath(a[2:]))
        elif a[:2] in ("-D", "-U"):
            given.append(a)
    with open(out + "/sources", "a") as f:
        for c in files:
            p = os.path.relpath(os.path.abspath(c), src)
            if not p.startswith(".."):
                f.write(" ".join([here, p, model] +
                                 [shlex.quote(a) for a in given]) + "\n")
cc = [top + "/tools/cc"] + (["-Dunix=1", "-DBSD2_11=1", "-D__BSD2_11__=1"]
                            if cross else ["-ccom"])
if what is None:
    # An object is linked once, as the system's cc has it (nodup): one
    # named again, or one a C file given too is compiled to, as pi's
    # Makefile names Version.o and Version.c.
    seen = set(os.path.splitext(os.path.basename(f))[0] + ".o" for f in files)
    once = []
    for a in rest:
        if a.endswith(".o") and not a.startswith("-"):
            if a in seen:
                continue
            seen.add(a)
        once.append(a)
    cmd = cc + ["-sysroot", libroot] + [found(lib(a)) for a in once] + files
    if o:
        cmd += ["-o", o]
    sys.exit(subprocess.call(cmd))
flags = [a for a in rest if a[:2] in ("-D", "-U", "-I", "-O", "-m", "-n")]
if cross:
    if what == "-S":
        sys.exit("cc: -S is not the cross compiler's")
    for f in files + [a for a in rest if a.endswith(".s")]:
        tmp = None
        if what == "-c" and os.path.basename(f) == "x.c":
            tmp = tempfile.mkdtemp(prefix="cc.")
            data = open(f, "rb").read()
            g = os.path.join(tmp, "x%s.c" % hashlib.sha256(data).hexdigest()[:8])
            shutil.copyfile(f, g)
            o = o or "x.o"
            f = g
        cmd = cc + [what] + flags + [f]
        if what == "-c" and o:
            cmd += ["-o", o]
        if what == "-E" and o:
            with open(o, "wb") as w:
                r = subprocess.call(cmd, stdout=w)
        else:
            r = subprocess.call(cmd)
        if tmp:
            shutil.rmtree(tmp)
        if r:
            sys.exit(r)
    sys.exit(0)
for f in files:
    cmd = [top + "/tools/ncc"] + flags
    if what != "-c":
        cmd.append(what)
    if o:
        cmd += ["-o", o]
    elif what == "-E":
        cmd += ["-o", "/dev/stdout"]
    r = subprocess.call(cmd + [f])
    if r:
        sys.exit(r)
for f in [a for a in rest if a.endswith(".s")]:
    r = subprocess.call([top + "/tools/cc", "-ccom", "-c"] + flags + [f] +
                        (["-o", o] if o else []))
    if r:
        sys.exit(r)
