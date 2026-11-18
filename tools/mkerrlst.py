#!/usr/bin/env python3
"""Write /etc/syserrlst, the system's error messages, as mkerrlst(1) does.

usage: mkerrlst.py ERRLST.C OUTPUT

ERRLST.C is usr/src/usr.lib/liberrlst/errlst.c, whose array sys_errlist[]
has the message for each error number in order.  2.11BSD keeps the
messages in a file rather than in every program's data, and strerror(),
perror() and the rest read them from it (lib/libc/gen/syserrlst.c).  The
layout is <errlst.h>'s, as the 286 has it: a header of eight shorts
(magic, the highest message number, the longest message's length, and
five reserved); then for each message its offset in the file, a long,
and its length, an int; then the messages, each followed by a newline.
"""
import re
import struct
import sys

ERRMAGIC = 0o12345


def messages(path):
    text = open(path).read()
    body = text[text.index("sys_errlist[]"):]
    body = body[body.index("{") + 1:body.index("};")]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    return [bytes(m, "latin-1").decode("unicode_escape")
            for m in re.findall(r'"((?:[^"\\]|\\.)*)"', body)]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    msgs = messages(sys.argv[1])
    out = struct.pack("<hhh5h", ERRMAGIC, len(msgs) - 1,
                      max(len(m) for m in msgs), 0, 0, 0, 0, 0)
    off = len(out) + 6 * len(msgs)
    for m in msgs:
        out += struct.pack("<lh", off, len(m))
        off += len(m) + 1
    for m in msgs:
        out += m.encode("latin-1") + b"\n"
    open(sys.argv[2], "wb").write(out)


if __name__ == "__main__":
    main()
