#!/usr/bin/env python3
"""Split a SimH .tap tape image into its tape files.

usage: untap.py IMAGE.tap OUTDIR

Writes OUTDIR/file0, file1, ... (one per tape file, split at tape marks)
and prints the record count, byte count and record sizes of each.
"""
import os
import struct
import sys


def main():
    src, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    n = 0
    out = None
    recs = total = 0
    sizes = set()

    def close():
        nonlocal out, n, recs, total, sizes
        if out:
            out.close()
            print("file%d: %d records, %d bytes, record sizes %s"
                  % (n, recs, total, sorted(sizes)))
            n += 1
        out = None
        recs = total = 0
        sizes = set()

    with open(src, "rb") as f:
        while True:
            h = f.read(4)
            if len(h) < 4:
                break
            length = struct.unpack("<I", h)[0]
            if length == 0:              # tape mark
                close()
                continue
            if length == 0xFFFFFFFF:     # end of medium
                break
            length &= 0x00FFFFFF
            data = f.read(length)
            if length & 1:               # records are padded to even length
                f.read(1)
            f.read(4)                    # trailing length
            if out is None:
                out = open(os.path.join(outdir, "file%d" % n), "wb")
            out.write(data)
            recs += 1
            total += length
            sizes.add(length)
    close()


if __name__ == "__main__":
    main()
