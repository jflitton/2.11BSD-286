#!/usr/bin/env python3
"""The system's nroff, where the system is built, for tests/make/run.sh.

usage: nroff.py TOP [FLAG...] [FILE...]

The tree's nroff as it is compiled for this machine, TOP/obj/host/nroff,
which looks for its macros and tables in tmac/ and term/ where it is,
and so is run there.  What it reads, the FILEs or, with none, the
standard input, has each `.so NAME` put in its place first, as the
system's nroff reads NAME: from the directory it is run in, and
/usr/include from the tree's, which is the system's (tools/mkman.py).
"""
import os
import re
import subprocess
import sys

SO = re.compile(rb"\.so[ \t]+(\S+)")
top = sys.argv[1]
flags = [a for a in sys.argv[2:] if a.startswith("-")]
files = [a for a in sys.argv[2:] if not a.startswith("-")]


def source(data):
    """DATA with each `.so NAME` replaced by NAME."""
    out = []
    for line in data.splitlines(True):
        m = SO.match(line)
        if not m:
            out.append(line)
            continue
        name = m.group(1).decode()
        if name.startswith("/usr/include/"):
            name = os.path.join(top, "usr", "include",
                                name[len("/usr/include/"):])
        with open(name, "rb") as f:
            out.append(source(f.read()))
    return b"".join(out)


if files:
    data = b""
    for name in files:
        with open(name, "rb") as f:
            data += f.read()
else:
    data = sys.stdin.buffer.read()
sys.stdout.flush()
sys.exit(subprocess.run(["./nroff"] + flags, input=source(data),
                        cwd=os.path.join(top, "obj", "host")).returncode)
