#!/usr/bin/env python3
"""A file whose every byte says where in the file it is, for the kernel's
test to read back (tests/kernel/init.c has the same rule).

usage: pattern.py FILE SIZE
       pattern.py FILE IMAGE OFFSET

The first makes FILE, of SIZE bytes.  The second fails unless the byte at
OFFSET of IMAGE, a file system that has FILE in it, is one of FILE's: its
sector is looked for in FILE.
"""
import sys


def main():
    if len(sys.argv) == 3:
        with open(sys.argv[1], "wb") as f:
            f.write(bytes((i * 7 + (i >> 8)) & 0xFF
                          for i in range(int(sys.argv[2], 0))))
        return
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    pattern = open(sys.argv[1], "rb").read()
    offset = int(sys.argv[3], 0)
    with open(sys.argv[2], "rb") as f:
        f.seek(offset & ~511)
        sector = f.read(512)
    if len(sector) < 512 or pattern.find(sector) < 0:
        sys.exit("%s: what is at %#x of %s is not from it"
                 % (sys.argv[1], offset, sys.argv[2]))


if __name__ == "__main__":
    main()
