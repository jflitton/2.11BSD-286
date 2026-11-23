#!/usr/bin/env python3
"""The shell that make runs a Makefile's commands with, for
tests/make/run.sh.

usage: sh.py HOSTBIN -c COMMAND

A command that the Makefile names by where the system has it,
/usr/ucb/xstr or /lib/cpp, is the one of that name here (commands.sh):
at the start of COMMAND, or after ; | & ( or `, the directory is taken
off.  A name that is not a command's, such as the place `make install`
puts a file, is left as it is.  Then COMMAND is the shell's.

A COMMAND that is no more than a program the Makefile made, ./setfiles,
is the 286's, and does not run here: it is said so, and make goes on
with what is after it, as it would when the program had done what it
does on the system, where the files that it writes are left to the 286.
But one that is compiled for this machine as well, in HOSTBIN, from the
same source (commands.sh), is that one, wherever the command names it:
./mkstubs, which libc's Makefile makes and runs, and without which
there is no library.
"""
import os
import re
import sys

CMD = re.compile(r"(^\s*|[;|&(`]\s*)/(?:bin|sbin|usr/bin|usr/ucb|usr/sbin|"
                 r"lib|usr/lib|usr/man|usr/libexec|etc)/"
                 r"([A-Za-z0-9_.+-]+)(?=[\s;|&)`]|$)", re.M)
ALONE = re.compile(r"\s*(\./[A-Za-z0-9_.+-]+)\s*$")
# What is compiled for this machine as well, from the same source.
MADE = re.compile(r"(^\s*|[;|&(`]\s*)\./(mkstubs)(?=[\s;|&)`]|$)", re.M)
A_MAGIC = b"\x86\x02"   # the 286's executable (tools/strip.py)


def theirs(path):
    """Whether PATH is a program of the 286's."""
    try:
        with open(path, "rb") as f:
            return f.read(2) == A_MAGIC
    except OSError:
        return False


hostbin = sys.argv[1]
args = sys.argv[2:]


def here(m):
    """./NAME, or the one compiled for this machine."""
    p = os.path.join(hostbin, m.group(2))
    if theirs("./" + m.group(2)) and os.path.isfile(p):
        return m.group(1) + p
    return m.group(0)


if len(args) >= 2 and args[0] == "-c":
    args[1] = MADE.sub(here, args[1])
    alone = ALONE.match(args[1])
    if alone and theirs(alone.group(1)):
        print("sh: %s is the 286's, and is not run here" % alone.group(1),
              file=sys.stderr)
        sys.exit(0)
    args[1] = CMD.sub(lambda m: m.group(1) + m.group(2), args[1])
os.execv("/bin/sh", ["sh"] + args)
