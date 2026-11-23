#!/usr/bin/env python3
"""The system's ar, where the system is built, for tests/make/run.sh.

usage: ar.py TOP KEY [POSNAME] LIB [FILE ...]

The library is made whole each time by tools/ar.py, with the table that
ranlib makes, which ranlib then has nothing to add to (commands.sh).
The keys that the Makefiles give:

    r   each FILE is put in LIB, in place of the member of its name or
        after the last; with c LIB is made if it is not there, which r
        does too, with u only a newer FILE is to be put there, which
        here is any, and with v ar says what it does, which here it
        does not
    x   each member, or each one named, is copied out to a file of its
        name here
    ma  each FILE, a member, is moved to after POSNAME
    t   the members' names are listed
"""
import os
import sys

top = sys.argv[1]
sys.path.insert(0, os.path.join(top, "tools"))
import ar  # noqa: E402

a = sys.argv[2:]
if not a:
    sys.exit("usage: ar key [posname] afile [name ...]")
key = a[0].lstrip("-")
if "m" in key:
    if "a" not in key or len(a) < 4:
        sys.exit("ar: only ma here, not %s" % key)
    pos, lib, names = a[1], a[2], a[3:]
else:
    lib, names = a[1], a[2:]
members = ar.read(lib) if os.path.exists(lib) else []
if "r" in key:
    for f in names:
        new = (os.path.basename(f), open(f, "rb").read())
        for i, m in enumerate(members):
            if m[0] == new[0]:
                members[i] = new
                break
        else:
            members.append(new)
    ar.write(lib, members)
elif "x" in key:
    for name, data in members:
        if not names or name in names:
            with open(name, "wb") as f:
                f.write(data)
elif "m" in key:
    moved = [m for m in members if m[0] in names]
    members = [m for m in members if m[0] not in names]
    at = [m[0] for m in members].index(pos) + 1
    ar.write(lib, members[:at] + moved + members[at:])
elif "t" in key:
    for name, _ in members:
        print(name)
else:
    sys.exit("ar: only r, x, ma and t here, not %s" % key)
