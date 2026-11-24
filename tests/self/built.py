#!/usr/bin/env python3
"""What the Makefiles of usr/src made and installed where the system is
built (tools/build/build.sh), for tests/self: which directory's
Makefile installed each file of the root, and which directories each
source is compiled in.

OUT is the build, obj/nbuild or obj/build.  OUT/meta has a line for each
file installed, and the last word of the line is the directory of
usr/src whose Makefile installed it, or - for what etc's distribution
and the build itself put there (install.py); OUT/sources a line for
each C file compiled, the directory it was compiled in and the file,
named from usr/src, and how (cc.py).
"""
import os
import stat


def build(top):
    """The build the root that is booted is of (tests/harness/system.sh):
    the system's own tools', unless TOOLS_PROGRAMS or TOOLS says the
    cross compiler's."""
    tools = os.environ.get("TOOLS_PROGRAMS") or \
        os.environ.get("TOOLS") or "native"
    return os.path.join(top, "obj", "nbuild" if tools == "native" else
                        "build")


def installed(out):
    """Each name of the root, OUT/dest, of a file a Makefile installed,
    and the directory of usr/src whose Makefile it was.  The name is the
    one the file has when the build is done, which may be another than
    it was installed by: a Makefile may move it (bin's strip, me's
    tmac.e) or link it, which keeps its inode, and the inode is in
    OUT/meta."""
    whose = {}
    for line in open(os.path.join(out, "meta")):
        w = line.split()
        if len(w) == 6 and w[5] != "-":
            whose[int(w[4])] = w[5]
    made = {}
    dest = os.path.join(out, "dest")
    for d, _, files in os.walk(dest):
        for f in files:
            p = os.path.join(d, f)
            st = os.lstat(p)
            if stat.S_ISREG(st.st_mode) and st.st_ino in whose:
                made["/" + os.path.relpath(p, dest)] = whose[st.st_ino]
    return made


def sources(out):
    """Each C file compiled, named from usr/src, and the directories it
    was compiled in."""
    among = {}
    for line in open(os.path.join(out, "sources")):
        d, f = line.split()[:2]
        among.setdefault(os.path.normpath(f), set()).add(d)
    return among


def kept(p, names):
    """Whether P is what a Makefile keeps of one of NAMES as it installs
    another over it: NAME.old, as tcsh's and init's do, or oNAME, as the
    compiler's do (lib/ccom, lib/c2)."""
    d, b = os.path.split(p)
    return p.endswith(".old") and p[:-4] in names or \
        b.startswith("o") and os.path.join(d, b[1:]) in names


def within(directory, dirs):
    """Whether DIRECTORY is one of DIRS or under one."""
    return any(directory == d or directory.startswith(d + "/") for d in dirs)


class Names(set):
    """The names a file lists, each the first word of a line, with #
    for a comment: one that ends with * is all that begin so."""
    def __init__(self, path):
        set.__init__(self)
        for line in open(path):
            line = line.split("#")[0].split()
            if line:
                self.add(line[0])

    def __contains__(self, p):
        return set.__contains__(self, p) or any(
            n.endswith("*") and p.startswith(n[:-1]) for n in self)
