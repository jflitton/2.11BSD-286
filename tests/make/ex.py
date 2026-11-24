#!/usr/bin/env python3
"""ex, where the system is built, for the two Makefiles that have it
edit: tests/make/run.sh's.

usage: ex.py TOP [ex's arguments]

`ex - termcap.src`, share/termcap's, has it run reorder on termcap.src:
tools/mktermcap.py does what reorder does.  `ex - <tokenscript`,
bin/awk's, has it put in token.c the table of the tokens' names and
numbers, a line of y.tab.h each, done here as the script does it:
of each line, `# define NAME N` is `"NAME", N,`, and the lines between
the table's `{` and `};` are those.
"""
import os
import re
import subprocess
import sys

top = sys.argv[1]
args = sys.argv[2:]
if args == ["-", "termcap.src"]:
    sys.exit(subprocess.call(["python3", "-I", top + "/tools/mktermcap.py",
                              ".", "termcap"]))
script = sys.stdin.read()
if args != ["-"] or not os.path.isfile("tokenscript") or \
        script != open("tokenscript").read():
    sys.exit("ex: only termcap's reorder and awk's tokenscript are done here")
table = []
for line in open("y.tab.h").read().splitlines():
    line = re.sub(r"# *define *", "", line, count=1)
    line = '"' + line
    line = line.replace(" ", '", ', 1)
    table.append(line + ",")
lines = open("token.c").read().splitlines()
start = next(i for i, l in enumerate(lines) if re.search(r"= *{$", l))
end = next(i for i in range(start + 1, len(lines))
           if re.match(r"^};$", lines[i]))
lines[start + 1:end] = table
with open("token.c", "w") as f:
    f.write("\n".join(lines) + "\n")
