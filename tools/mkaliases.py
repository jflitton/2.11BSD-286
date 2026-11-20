#!/usr/bin/env python3
"""Write the database of /etc/aliases, as newaliases(1) does.

usage: mkaliases.py ALIASES DIRECTORY

sendmail looks an alias up in an ndbm database, aliases.dir and
aliases.pag beside the file.  Until the database is there and has its
last key, "@", every sendmail waits for someone to finish making it, as
long as the configuration's option `a` says: ten minutes, in upstream's
sendmail.cf.  On the system `newaliases` (sendmail -bi) makes it; this
makes in DIRECTORY the same two files, byte for byte as newaliases on
the 286 would, which tests/system checks.

The file is read as sendmail's readaliases does (sendmail/src/alias.c).
A line that begins with `#`, and an empty one, is passed over; one that
begins with a blank or a tab continues the line before.  The name is
what comes before the first colon, in small letters, and the alias is
the rest, with what continues it appended as it is, less each newline.
Each is stored with its terminating null, in the order of the file, a
later one in place of an earlier of the same name; and last "@" with
the value "@".  sendmail parses the name as an address, which this does
not: a name here must be a simple local one, of letters, digits and
`.`, `-` and `_`.

The database is lib/libc/gen/ndbm.c's, made as tools/mkpasswd.py makes
the password file's.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mkpasswd         # noqa: E402

NAME = re.compile(rb"[A-Za-z0-9._-]+\Z")


def store(db, key, dat):
    """dbm_store(..., DBM_REPLACE), as libdbm's store does it."""
    while True:
        db.access(mkpasswd.calchash(key))
        i = mkpasswd.finddatum(db.pagbuf, key)
        if i >= 0:
            mkpasswd.delitem(db.pagbuf, i)
        if db.store(key, dat):
            return


def aliases(text):
    """The aliases in TEXT, the file, in order: (name, alias)."""
    lines = text.split(b"\n")
    if lines and lines[-1] == b"":
        lines.pop()
    i = 0
    while i < len(lines):
        line = lines[i]
        i += 1
        if line[:1] in (b"#", b""):
            continue
        if line[:1] in (b" ", b"\t"):
            sys.exit("mkaliases: line %d: Non-continuation line starts "
                     "with space" % i)
        if b":" not in line:
            sys.exit("mkaliases: line %d: missing colon" % i)
        name, rhs = line.split(b":", 1)
        name = name.strip(b" \t")
        if not NAME.match(name):
            sys.exit("mkaliases: line %d: %r is not a simple name; make "
                     "the database with newaliases" % (i, name))
        while i < len(lines) and lines[i][:1] in (b" ", b"\t"):
            rhs += lines[i]
            i += 1
        yield name.lower(), rhs


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    db = mkpasswd.Dbm()
    for name, rhs in aliases(open(sys.argv[1], "rb").read()):
        store(db, name + b"\0", rhs + b"\0")
    store(db, b"@\0", b"@\0")
    for suffix, data in (".dir", db.dir), (".pag", db.pag):
        with open(os.path.join(sys.argv[2], "aliases" + suffix), "wb") as out:
            out.write(data)


if __name__ == "__main__":
    main()
