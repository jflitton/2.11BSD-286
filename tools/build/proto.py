#!/usr/bin/env python3
"""Write the proto of the root that was installed, for tools/mkfs.py, for
tools/build/build.sh.

usage: proto.py TOP OUT > PROTO

OUT/dest is the root as the Makefiles and etc's distribution installed
it, and OUT/meta what install, chmod, chown, chgrp, tar, mknod and
tape.py were to make of each file's mode, owner and group, which only
the system can give it: a line each, of the file's name, its mode, its
owner and its group, with - for whoever runs make and = for what is
not changed, and the number of its inode, which says which file it
was, by any name it had or has.  They are taken in order, after what
the system makes a new file: root's, of the group of its directory,
and with the mode this machine gave it, which is the system's, the
system's sources being copied as they are.  OUT/devices says which
files are devices (mknod.py).  A name of a file with another's inode
is another name for it, a link.

Owners and groups are named as usr/src/etc's master.passwd and group
name them, or by number.  A file goes in the root without its symbol
table if install -s was given it, which install.py has seen to.
"""
import os
import re
import sys

top, out = sys.argv[1:3]
dest = out + "/dest"
WHO = {"u": (0o4700, 6, 0o4000), "g": (0o2070, 3, 0o2000),
       "o": (0o1007, 0, 0)}


def numbers(path, field):
    return {w[0]: w[field] for w in
            (line.split(":") for line in open(path)) if len(w) > field}


uid = numbers(top + "/usr/src/etc/master.passwd", 2)
gid = numbers(top + "/usr/src/etc/group", 2)


def chmod(mode, spec):
    """MODE as chmod SPEC leaves it: a number, or [ugoa]+-=[rwxst],..."""
    if spec.isdigit():
        return int(spec, 8)
    for clause in spec.split(","):
        m = re.match(r"([ugoa]*)([-+=])([rwxst]*)$", clause)
        if not m:
            sys.exit("proto.py: chmod %s is not one it knows" % spec)
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


meta = {}
for line in open(out + "/meta"):
    w = line.split()
    meta.setdefault(int(w[4]), []).append(w[1:4])
devices = {}
if os.path.exists(out + "/devices"):
    for line in open(out + "/devices"):
        w = line.split()
        devices[int(w[0])] = w[1:]

whose = {"": (0o755, "0", "0")}


def state(p):
    """The mode, owner and group the system would give P, of OUT/dest."""
    if p in whose:
        return whose[p]
    st = os.lstat(dest + p)
    mode, owner, group = st.st_mode & 0o777, "0", \
        state(os.path.dirname(p).rstrip("/"))[2]
    for m, o, g in meta.get(st.st_ino, []):
        if m != "=":
            mode = chmod(mode, m)
        if o != "=":
            owner = "0" if o == "-" else uid.get(o, o)
        if g != "=":
            group = state(os.path.dirname(p).rstrip("/"))[2] if g == "-" \
                else gid.get(g, g)
    whose[p] = (mode, owner, group)
    return whose[p]


def mode(p):
    m, o, g = state(p)
    return "%o" % m + ("" if (o, g) == ("0", "0") else ":%s.%s" % (o, g))


names = {}
lines = []
for dp, dn, fn in os.walk(dest):
    dn.sort()
    for n in sorted(dn + fn):
        t = os.path.join(dp, n)
        p = t[len(dest):]
        st = os.lstat(t)
        if os.path.islink(t):
            lines.append("l %s %s" % (p, os.readlink(t)))
        elif os.path.isdir(t):
            lines.append("d %s %s" % (p, mode(p)))
        elif st.st_ino in names:
            lines.append("h %s %s" % (p, names[st.st_ino]))
        elif st.st_ino in devices:
            kind, major, minor = devices[st.st_ino]
            names[st.st_ino] = p
            lines.append("%s %s %s %s %s" % (kind, p, mode(p), major, minor))
        else:
            names[st.st_ino] = p
            lines.append("f %s %s %s" % (p, mode(p), t))
print("\n".join(lines))
