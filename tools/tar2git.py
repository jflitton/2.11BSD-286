#!/usr/bin/env python3
"""Turn tar archives into one git commit without touching the file system.

usage: tar2git.py --branch upstream --message-file MSG [--date ISO8601]
                  [--parent REF] [--symlink PATH=TARGET ...]
                  PREFIX=ARCHIVE[:MEMBERPREFIX] ...  | git fast-import

Each PREFIX=ARCHIVE puts the members of ARCHIVE under PREFIX in the commit.
With :MEMBERPREFIX only members under that path are taken, and the member
prefix is stripped.  File names go into git exactly as the archive has them,
so names that differ only by case survive on a case-insensitive host.

git keeps only the executable bit of the mode, and no empty directories;
the empty directories are listed on stderr.
"""
import argparse
import sys
import tarfile
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--branch", required=True)
    ap.add_argument("--message-file", required=True)
    ap.add_argument("--date")
    ap.add_argument("--parent")
    ap.add_argument("--name", default="2.11BSD")
    ap.add_argument("--email", default="upstream@localhost")
    ap.add_argument("--symlink", action="append", default=[])
    ap.add_argument("maps", nargs="+")
    a = ap.parse_args()

    out = sys.stdout.buffer
    when = int(time.mktime(time.strptime(a.date, "%Y-%m-%dT%H:%M:%S"))) \
        if a.date else int(time.time())
    msg = open(a.message_file, "rb").read()

    entries = {}          # path -> (mode, mark)
    dirs = set()
    mark = 0

    def blob(data):
        nonlocal mark
        mark += 1
        out.write(b"blob\nmark :%d\ndata %d\n" % (mark, len(data)))
        out.write(data)
        out.write(b"\n")
        return mark

    for m in a.maps:
        prefix, _, rest = m.partition("=")
        archive, _, want = rest.partition(":")
        prefix = prefix.strip("/")
        want = want.strip("/")
        with tarfile.open(archive, "r:") as tf:
            for ti in tf:
                name = ti.name.strip("/")
                while name.startswith("./"):
                    name = name[2:]
                if name in ("", "."):
                    continue
                if ".." in name.split("/"):
                    sys.exit("refusing member with ..: %r" % ti.name)
                if want:
                    if name != want and not name.startswith(want + "/"):
                        continue
                    name = name[len(want):].strip("/")
                    if not name:
                        continue
                path = (prefix + "/" + name) if prefix else name
                if ti.isdir():
                    dirs.add(path)
                elif ti.issym():
                    entries[path] = ("120000", blob(ti.linkname.encode()))
                elif ti.isreg():
                    data = tf.extractfile(ti).read()
                    mode = "100755" if ti.mode & 0o111 else "100644"
                    entries[path] = (mode, blob(data))
                else:
                    sys.exit("unhandled member type %r: %s" % (ti.type, ti.name))

    for s in a.symlink:
        path, _, target = s.partition("=")
        entries[path.strip("/")] = ("120000", blob(target.encode()))

    who = b"%s <%s> %d +0000" % (a.name.encode(), a.email.encode(), when)
    out.write(b"commit refs/heads/%s\n" % a.branch.encode())
    out.write(b"author " + who + b"\ncommitter " + who + b"\n")
    out.write(b"data %d\n" % len(msg))
    out.write(msg)
    out.write(b"\n")
    if a.parent:
        out.write(b"from %s\n" % a.parent.encode())
    out.write(b"deleteall\n")
    for path in sorted(entries):
        mode, mk = entries[path]
        out.write(b"M %s :%d " % (mode.encode(), mk))
        out.write(path.encode() + b"\n")
    out.write(b"\n")

    used = set()
    for path in entries:
        parts = path.split("/")
        for i in range(1, len(parts)):
            used.add("/".join(parts[:i]))
    for d in sorted(dirs - used):
        print("empty directory not kept: " + d, file=sys.stderr)
    print("%d files" % len(entries), file=sys.stderr)


if __name__ == "__main__":
    main()
