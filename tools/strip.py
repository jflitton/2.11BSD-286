#!/usr/bin/env python3
"""Copy an executable file without its symbol table, as strip(1) leaves one.

usage: strip.py IN OUT

IN is in the system's format (usr/src/sys/h/exec.h): a header, a table of
segments, the segments, each made up to an even length, and after them the
symbol table and its strings.  OUT is IN to the end of the last segment,
with the sizes of the two tables in the header zero.  It is what the
system's strip makes of the file, to the byte (tests/programs).

The kernel loads neither table, so a program runs the same without them;
they are a file's bulk and nothing else.  /unix keeps its own: ps and the
other programs that read the kernel look its symbols up there.
"""
import os
import struct
import sys

HEADER = "<HHHHHHHHLL"
SEGMENT = "<HHLL"
A_MAGIC = 0x0286


def strip(data, name="file"):
    n = struct.calcsize(HEADER)
    if len(data) < n:
        sys.exit("strip.py: %s: not an executable file" % name)
    head = list(struct.unpack(HEADER, data[:n]))
    if head[0] != A_MAGIC:
        sys.exit("strip.py: %s: not an executable file" % name)
    end = n + head[2] * struct.calcsize(SEGMENT)
    for i in range(head[2]):
        at = n + i * struct.calcsize(SEGMENT)
        filesz = struct.unpack(SEGMENT, data[at:at + struct.calcsize(SEGMENT)])[2]
        end += (filesz + 1) & ~1
    if end > len(data):
        sys.exit("strip.py: %s: shorter than its segments" % name)
    head[8] = head[9] = 0
    return struct.pack(HEADER, *head) + data[n:end]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.split("\n\n")[1])
    src, dst = sys.argv[1:]
    with open(src, "rb") as f:
        data = f.read()
    with open(dst, "wb") as f:
        f.write(strip(data, src))
    os.chmod(dst, os.stat(src).st_mode & 0o777)


if __name__ == "__main__":
    main()
