#!/usr/bin/env python3
"""The shell that make runs a Makefile's commands with, for
tests/make/run.sh.

usage: sh.py -c COMMAND

A command that the Makefile names by where the system has it,
/usr/ucb/xstr or /lib/cpp, is the one of that name here (commands.sh):
at the start of COMMAND, or after ; | & ( or `, the directory is taken
off.  A name that is not a command's, such as the place `make install`
puts a file, is left as it is.  Then COMMAND is the shell's.
"""
import os
import re
import sys

CMD = re.compile(r"(^\s*|[;|&(`]\s*)/(?:bin|sbin|usr/bin|usr/ucb|usr/sbin|"
                 r"lib|usr/lib|usr/man|usr/libexec|etc)/"
                 r"([A-Za-z0-9_.+-]+)(?=[\s;|&)`]|$)", re.M)

args = sys.argv[1:]
if len(args) >= 2 and args[0] == "-c":
    args[1] = CMD.sub(lambda m: m.group(1) + m.group(2), args[1])
os.execv("/bin/sh", ["sh"] + args)
