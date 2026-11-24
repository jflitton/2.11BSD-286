#!/usr/bin/env python3
"""What `make install` put in the root against what the root file system
has, for tests/make/run.sh.

usage: check.py TOP OUT SECONDS DIR...

OUT/dest is the root the Makefiles of the DIRs installed in, with each
of the root's directories made there first (dest.py: OUT/dirs.ino);
OUT/dest.meta what install, chmod and chown were to make of a file's
mode, owner and group, which only the system can give it (install.py,
chmod.py, chown.py); and OUT/root.list the root file system's own list
(tools/fsck.py -l).

Each file of OUT/dest is to be one the root has, with those contents,
and each directory one it has, and each with the root's mode, owner and
group, by their numbers in usr/src/etc/master.passwd and group.  Those
are what install, chmod and chown made them, in that order, of the file
that is there, by any name it had or has: a line of dest.meta is of
the file whose inode it names.
Before them, a directory that was there is as the root has it; anything
else is as the system makes a new one: of root, who runs make, and the
group of its directory, with the mode this machine gave it, which is
the system's, since what is copied is the system's sources.  Each of the
root's directories is to be there still.
A page of the manual says at its foot the day it was formatted: one
that differs from the root's is compared again without that.
Of the root's programs whose sources are in a DIR (programs.py), each
must have been installed, and so each of its pages of the manual whose
source is, and of its libraries those the DIR's Makefiles install, and of its kernel, /boot and boot blocks those of sys/PC's and
sys/i286stand's (STAND); those of `left`, beside this, are made with a
program their
Makefile makes and runs, and are the 286's to make.
Those of `differs` are made, and are not yet what the other build makes
of them.  What a Makefile keeps of a file it installs over, NAME.old or
oNAME, is not the root's, and is passed over (programs.kept).
"""
import collections
import hashlib
import os
import re
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import programs  # noqa: E402

# What the kernel's Makefile and /boot's install.
STAND = {"sys/PC": ["/unix"],
         "sys/i286stand": ["/boot", "/mdec/wduboot", "/mdec/sduboot",
                           "/mdec/fluboot"]}

top, out, took = sys.argv[1:4]
dirs = sys.argv[4:]
dest = out + "/dest"

want = {}       # a file's mode, sum, and owner and group
wantdir = {}    # a directory's mode, and owner and group
for line in open(out + "/root.list"):
    w = line.split()
    if len(w) >= 6 and w[1] == "f":
        want[w[0]] = (w[2], w[5], w[3])
    elif len(w) >= 4 and w[1] == "d":
        wantdir[w[0]] = (w[2], w[3])
made = {}       # the inode of each directory as dest.py made it
for line in open(out + "/dirs.ino"):
    d, n = line.split()
    made[d] = int(n)
meta = collections.defaultdict(list)    # what was done to each inode
for line in open(out + "/dest.meta"):
    w = line.split()
    meta[int(w[4])].append(w[1:4] + [os.path.normpath(w[0])])


def numbers(path, field):
    """The number of each name in PATH, a file of lines of fields with
    colons between them: FIELD is the number's."""
    return {w[0]: w[field] for w in
            (line.split(":") for line in open(path)) if len(w) > field}


uid = numbers(top + "/usr/src/etc/master.passwd", 2)
gid = numbers(top + "/usr/src/etc/group", 2)
WHO = {"u": (0o4700, 6, 0o4000), "g": (0o2070, 3, 0o2000),
       "o": (0o1007, 0, 0)}


def chmod(mode, spec):
    """MODE as chmod SPEC leaves it: a number, or [ugoa]+-=[rwxst],..."""
    if spec.isdigit():
        return int(spec, 8)
    for clause in spec.split(","):
        m = re.match(r"([ugoa]*)([-+=])([rwxst]*)$", clause)
        if not m:
            sys.exit("check.py: chmod %s is not one it knows" % spec)
        who, op, perm = m.groups()
        bits = mask = 0
        for c in (who or "a").replace("a", "ugo"):
            mask |= WHO[c][0]
            for p in perm:
                if p in "rwx":
                    bits |= {"r": 4, "w": 2, "x": 1}[p] << WHO[c][1]
                elif p == "s":
                    bits |= WHO[c][2]
                elif p == "t":
                    bits |= 0o1000
        if op == "+":
            mode |= bits
        elif op == "-":
            mode &= ~bits
        else:
            mode = mode & ~mask | bits
    return mode


whose = {}


def state(p):
    """The mode, owner and group the system would give P, of OUT/dest."""
    if p in whose:
        return whose[p]
    st = os.lstat(dest + p)
    if p in made and made[p] == st.st_ino:
        mode = int(wantdir[p][0], 8)
        owner, group = wantdir[p][1].split(".")
    else:
        mode = st.st_mode & 0o777
        owner, group = "0", state(os.path.dirname(p))[2]
    for m, o, g, q in meta[st.st_ino]:
        if m != "=":
            mode = chmod(mode, m)
        if o != "=":
            owner = "0" if o == "-" else uid.get(o, o)
        if g != "=":
            group = state(os.path.dirname(q))[2] if g == "-" else \
                gid.get(g, g)
    whose[p] = (mode, owner, group)
    return whose[p]


source = programs.sources()
where = programs.installed(source)
left = programs.Names(here + "/left")
differs = programs.Names(here + "/differs")


def page(p, t):
    """Whether P is a page of the manual and T, as it was installed,
    is the root's but for the day it was formatted."""
    if not (p.startswith(("/usr/man/", "/usr/new/man/", "/usr/local/man/")) and
            p.endswith(".0")):
        return False
    day = re.compile(rb"Printed [0-9]+/[0-9]+/[0-9]+")
    root = subprocess.run(
        [sys.executable, "-I", top + "/tools/fsck.py", "-x", p,
         top + "/obj/root/root.fs"], capture_output=True, check=True).stdout
    return day.sub(b"Printed", root) == day.sub(b"Printed",
                                                open(t, "rb").read())


def compare(p, mode, owner):
    """Whether P's mode and owner are MODE and OWNER, the root's."""
    m, o, g = state(p)
    if m != int(mode, 8):
        bad.append("MODE %s: %o, and the root's is %s" % (p, m, mode))
    elif "%s.%s" % (o, g) != owner:
        bad.append("OWNER %s: %s.%s, and the root's is %s" % (p, o, g, owner))
    else:
        return True
    return False


bad = []
same = 0
for dp, dn, fn in os.walk(dest):
    for d in dn:
        p = os.path.join(dp, d)[len(dest):]
        if p not in wantdir and not os.path.islink(dest + p):
            bad.append("NOT THE ROOT'S %s/" % p)
    for f in fn:
        t = os.path.join(dp, f)
        p = t[len(dest):]
        if os.path.islink(t):
            continue
        if p not in want:
            if p not in differs and not programs.kept(p, want):
                bad.append("NOT THE ROOT'S %s" % p)
            continue
        if hashlib.sha256(open(t, "rb").read()).hexdigest()[:16] != \
                want[p][1]:
            if not page(p, t):
                if p not in left and p not in differs:
                    bad.append("DIFFERS %s" % p)
                continue
        if compare(p, want[p][0], want[p][2]):
            same += 1
for p, (mode, owner) in sorted(wantdir.items()):
    if not os.path.isdir(dest + p):
        bad.append("GONE %s/" % p)
    else:
        compare(p, mode, owner)
nleft = 0
for p, name in sorted(where.items()):
    if not programs.within(source[name], dirs) or os.path.isfile(dest + p):
        continue
    if p in left:
        nleft += 1
    else:
        bad.append("NOT INSTALLED %s" % p)
bad += ["NOT INSTALLED %s" % p for p in programs.libraries(sorted(want), dirs)
        if not os.path.isfile(dest + p)]
bad += ["NOT INSTALLED %s" % p for p, d in sorted(programs.pages().items())
        if programs.within(d, dirs) and not os.path.isfile(dest + p) and
        p not in left]
bad += ["NOT INSTALLED %s" % p for d in dirs for p in STAND.get(d, [])
        if not os.path.isfile(dest + p)]
for line in sorted(bad):
    print(line, file=sys.stderr)
if bad:
    sys.exit("make: %d files are not what the root has (%s/log)" %
             (len(bad), out))
print("make: the Makefiles of %s installed %d files where the system is "
      "built, in %s s, each what the root has and whose, and left its %d "
      "directories as it has them; %d programs are left to the 286" %
      (" ".join(dirs), same, took, len(wantdir), nleft))
