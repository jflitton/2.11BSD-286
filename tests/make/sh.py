#!/usr/bin/env python3
"""The shell that make runs a Makefile's commands with, for
tests/make/run.sh.

usage: sh.py HOSTBIN OUT -c COMMAND

A command that the Makefile names by where the system has it,
/usr/ucb/xstr or /lib/cpp, is the one of that name here (commands.sh):
at the start of COMMAND, or after ; | & ( or `, the directory is taken
off.  Any other name of the system's root that COMMAND has after a
space, < > or (, such as the place `make install` puts a file, a
directory it goes into, one a redirection writes or a pattern of names,
is that name in OUT/dest, the root here, as commands.sh's own commands
have it; but /usr/include and /usr/src, which are OUT/root's
(stage.py).  Then COMMAND is the shell's, in the C locale, as the
system has no other.

A COMMAND that is no more than a program the Makefile made, ./setfiles,
or one it installed that is not one of commands.sh's, /usr/bin/mkerrlst,
is the 286's, and does not run here: it is said so, and make goes on with what is after it, as it
would when the program had done what it does on the system, where the
files that it writes are left to the 286.  But one that is compiled for
this machine as well, in HOSTBIN, from the same source (commands.sh),
is that one, wherever the command names it: ./mkstubs, which libc's
Makefile makes and runs, and without which there is no library, and
./zic, which writes the time zones' files.
"""
import os
import re
import sys

CMD = re.compile(r"(^\s*|[;|&(`]\s*)/(?:bin|sbin|usr/bin|usr/ucb|usr/sbin|"
                 r"lib|usr/lib|usr/man|usr/libexec|etc)/"
                 r"([A-Za-z0-9_.+-]+)(?=[\s;|&)`]|$)", re.M)
ALONE = re.compile(r"\s*(\.?/[A-Za-z0-9_./+-]+)\s*$")
# What is compiled for this machine as well, from the same source.
MADE = re.compile(r"(^\s*|[;|&(`]\s*)\./(mkstubs|zic)(?=[\s;|&)`]|$)", re.M)
# A name, and whether it is one of the root's.
NAME = re.compile(r"(?<=[\s<>(])/[^\s;|&()`'\"<>]*")
ROOT = re.compile(r"/(?:(?:bin|sbin|etc|lib|var|mdec|usr)(?:/|$)|"
                  r"(?:unix|unix\.old|boot)$)")
OWN = re.compile(r"/usr/(?:include|src)(?:/|$)")
A_MAGIC = b"\x86\x02"   # the 286's executable (tools/strip.py)


def theirs(path):
    """Whether PATH is a program of the 286's."""
    try:
        with open(path, "rb") as f:
            return f.read(2) == A_MAGIC
    except OSError:
        return False


hostbin = sys.argv[1]
out = sys.argv[2]
dest = out + "/dest"
args = sys.argv[3:]


def root(m):
    """The name in OUT/dest, if it is one of the root's."""
    p = m.group(0)
    return dest + p if ROOT.match(p) and not OWN.match(p) else p


def here(m):
    """./NAME, or the one compiled for this machine."""
    p = os.path.join(hostbin, m.group(2))
    if theirs("./" + m.group(2)) and os.path.isfile(p):
        return m.group(1) + p
    return m.group(0)


if len(args) >= 2 and args[0] == "-c":
    args[1] = MADE.sub(here, args[1])
    alone = ALONE.match(args[1])
    if alone and (theirs(alone.group(1)) if alone.group(1).startswith(".")
                  else theirs(dest + alone.group(1)) and not os.path.exists(
                      os.path.join(out, "bin",
                                   os.path.basename(alone.group(1))))):
        print("sh: %s is the 286's, and is not run here" % alone.group(1),
              file=sys.stderr)
        sys.exit(0)
    args[1] = CMD.sub(lambda m: m.group(1) + m.group(2), args[1])
    args[1] = NAME.sub(root, args[1])
os.execve("/bin/sh", ["sh"] + args, dict(os.environ, LC_ALL="C"))
