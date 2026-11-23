#!/usr/bin/env python3
"""The system's programs, for tests/make and tests/self: where the root
file system has each, and the directory of usr/src its sources are in,
whose Makefile makes it.

usage: programs.py NAME...

prints the directories of the programs of those names, each once: what
tests/self/progs.sh is given to have the system make them.  It is the
Makefile of usr/src/i286 that says what a program is made of (make
table) and root.proto, there, where the root has it.
"""
import os
import re
import subprocess
import sys

TOP = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))

# Programs whose first source is not in their own directory, and that
# directory: the source is one the Makefile of usr/src/i286 makes, in
# obj, or, ranlib's, another program's.
GENERATED = {
    "ranlib": "usr.bin/ranlib",
    "awk": "bin/awk", "tcsh": "bin/tcsh", "expr": "bin", "hack": "games/hack",
    "egrep": "usr.bin", "bc": "usr.bin", "eqn": "usr.bin/eqn",
    "neqn": "usr.bin/neqn", "style1": "usr.bin/diction",
    "style2": "usr.bin/diction", "style3": "usr.bin/diction",
}


def sources():
    """The directory of each program's sources, by the program's name."""
    table = subprocess.run(["make", "-s", "-C", TOP + "/usr/src/i286",
                            "table"], capture_output=True, text=True,
                           check=True).stdout
    out = {}
    for line in table.splitlines():
        if line.startswith("PROG "):
            head, flags, srcs, libs = line[5:].split("|")
            name = head.split()[0]
            first = srcs.split()[0].replace(TOP + "/usr/src/", "")
            out[name] = GENERATED.get(name, os.path.dirname(first))
    return out


def installed(source):
    """Where the root has each program, and the program's name."""
    out = {}
    for line in open(TOP + "/usr/src/i286/root.proto"):
        w = line.split()
        if len(w) == 4 and w[0] == "f" and os.path.basename(w[3]) in source \
                and ("/obj/root/bin/" in w[3] or "/obj/nbin/" in w[3]):
            out[w[1]] = os.path.basename(w[3])
    return out


def pages():
    """Where the root has each page of the manual, and the directory of
    usr/src its source is in, whose Makefile installs it: those of
    usr/src/man's directories, man's (tools/mkman.py)."""
    sys.path.insert(0, TOP + "/tools")
    import mkman
    src = TOP + "/usr/src"
    wanted = mkman.pages(src, src + "/i286/man.list")
    out = {}
    for d, name, path in wanted:
        rel = os.path.dirname(os.path.relpath(path, src))
        out[os.path.normpath("/usr/man/%s/%s" % (d, name))] = \
            "man" if rel.startswith("man/") else rel
    for (d, name), (pd, page) in mkman.others(wanted).items():
        out["/usr/man/%s/%s" % (d, name)] = out["/usr/man/%s/%s" % (pd, page)]
    return out


# The libraries a directory's Makefile installs: the C library and its
# start-up code, of each model, and the others.
LIBRARIES = {"lib/libc": re.compile(r"/lib/(?:[ml]/)?(?:libc\.a|crt0\.o)$"),
             "usr.lib": re.compile(r"/usr/lib/(?:[ml]/)?lib\w+\.a$")}


def libraries(names, dirs):
    """Of the root's NAMES, the libraries the Makefiles of DIRS install."""
    return [p for p in names
            if any(within(d, dirs) and r.match(p)
                   for d, r in LIBRARIES.items())]


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


if __name__ == "__main__":
    source = sources()
    dirs = []
    for name in sys.argv[1:]:
        if name not in source:
            sys.exit("programs.py: %s is not a program the system has" % name)
        if source[name] not in dirs:
            dirs.append(source[name])
    print(" ".join(dirs))
