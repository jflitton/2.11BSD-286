#!/usr/bin/env python3
"""Make a bootable 1.44 MB diskette image from an executable.

usage: mkfd.py [--spread] BOOT.bin PROGRAM IMAGE

BOOT.bin is usr/src/sys/i286stand/fdboot.s assembled; PROGRAM is in the
system's executable format (docs/executable-format.md).  The image is the
boot sector and then each segment, starting on a sector.  The boot sector's
table is filled in with where each segment is to be loaded, as
docs/boot.md describes:

    by default   one after another from 1000:0, each on a multiple of 512
                 bytes; for a program converted with ne2aout.py --gdt
    --spread     64K apart from 1000:0; for a real-mode program converted
                 with ne2aout.py --real 0x1000
"""
import struct
import sys

MAGIC = 0x0286
HDR = "<HHHHHHHHLL"
SEG = "<HHLL"
MAXSEG = 16
FIRST = 0x1000          # real-mode segment of the first one loaded
TOP = 0x9FC0            # 639K: the BIOS keeps the last 1K of the 640
INFO = 3                # offset of the table in the boot sector
SIZE = 1474560


def main():
    args = sys.argv[1:]
    spread = "--spread" in args
    if spread:
        args.remove("--spread")
    if len(args) != 3:
        sys.exit(__doc__)
    boot = bytearray(open(args[0], "rb").read())
    b = open(args[1], "rb").read()
    if len(boot) != 512:
        sys.exit("%s: not one sector" % args[0])
    magic, _model, nseg, eseg, eoff, dseg = struct.unpack_from("<HHHHHH", b, 0)
    if magic != MAGIC:
        sys.exit("%s: bad magic %#06x" % (args[1], magic))
    if nseg > MAXSEG:
        sys.exit("%s: %d segments; the boot sector takes %d"
                 % (args[1], nseg, MAXSEG))

    off = struct.calcsize(HDR) + nseg * struct.calcsize(SEG)
    table = struct.pack("<HHHHHH", MAGIC, nseg, eseg, eoff, dseg, 0)
    body = b""
    para = FIRST
    for i in range(nseg):
        kind, _r, filesz, memsz = struct.unpack_from(
            SEG, b, struct.calcsize(HDR) + i * struct.calcsize(SEG))
        if not 0 < memsz <= 0x10000:
            sys.exit("%s: segment %d is %d bytes" % (args[1], i + 1, memsz))
        sectors = (filesz + 511) // 512
        body += b[off:off + filesz].ljust(sectors * 512, b"\0")
        off += filesz + (filesz & 1)
        table += struct.pack("<HHHH", para, sectors, memsz - 1, kind)
        para += 0x1000 if spread else (memsz + 511) // 512 * 32
        if para > TOP:
            sys.exit("%s: does not fit below 640K" % args[1])
    boot[INFO:INFO + len(table)] = table
    if 512 + len(body) > SIZE:
        sys.exit("%s: does not fit on the diskette" % args[1])
    with open(args[2], "wb") as f:
        f.write(boot)
        f.write(body)
        f.truncate(SIZE)


if __name__ == "__main__":
    main()
