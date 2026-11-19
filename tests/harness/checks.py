#!/usr/bin/env python3
"""Lay out a file of checks for a test that has the system run them.

usage: checks.py CHECKS DIR

CHECKS holds data files and checks.  "=file NAME" begins a data file,
and "== NAME" a check: lines for the shell up to a line "--", then what
they must write, to the byte.  DIR gets each check's lines in t/NAME,
what they must write in x/NAME, the data files in d, the checks' names
in names, in the order given, and check.sh, which the system runs once
DIR is mounted on /mnt: it runs each check in an empty directory with
nothing to read, compares what it wrote, and says "ok NAME", or "BAD
NAME", what it did write, and "END NAME"; and at the end how many of
each there were.  tests/programs and tests/network are made of these.
"""
import os
import sys

CHECK = """PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
ok=0 bad=0
for n in `cat /mnt/names`
do
	rm -rf /tmp/w; mkdir /tmp/w
	(cd /tmp/w; sh /mnt/t/$n) >/tmp/out 2>&1 </dev/null
	if cmp -s /tmp/out /mnt/x/$n
	then
		ok=`expr $ok + 1`; echo "ok $n"
	else
		bad=`expr $bad + 1`; echo "BAD $n"; cat /tmp/out; echo "END $n"
	fi
done
echo "checks: $ok ok, $bad bad"
"""


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.split("\n\n")[1])
    src, dst = sys.argv[1:]
    for d in ("t", "x", "d"):
        os.makedirs(os.path.join(dst, d))
    out = None
    names = []
    for line in open(src):
        if line.startswith("=file "):
            out = open(os.path.join(dst, "d", line.split()[1]), "w")
        elif line.startswith("== "):
            name = line.split()[1]
            if name in names:
                sys.exit("checks.py: %s: two checks named %s" % (src, name))
            names.append(name)
            out = open(os.path.join(dst, "t", name), "w")
        elif line == "--\n" and out and out.name.startswith(
                os.path.join(dst, "t")):
            out = open(os.path.join(dst, "x", name), "w")
        elif out:
            out.write(line)
    with open(os.path.join(dst, "names"), "w") as f:
        f.write("".join(n + "\n" for n in names))
    with open(os.path.join(dst, "check.sh"), "w") as f:
        f.write(CHECK)


if __name__ == "__main__":
    main()
