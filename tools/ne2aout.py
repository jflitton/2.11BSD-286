#!/usr/bin/env python3
"""Turn wlink's segmented output into a 2.11BSD/286 executable.

usage: ne2aout.py [--model s|m|l] [--gdt FIRST | --real BASE] [--map FILE]
                  [--stack N] IN.ne OUT
       ne2aout.py --dump FILE

wlink is run with `format os2`, which writes a "new executable": a segment
table, and for each segment its contents and a list of places that hold a
segment number still to be filled in.  This fills them in for good and
writes the system's own format, described in docs/executable-format.md.

The selector of segment N (counting from 1) is fixed here, not at load
time:

    a program:   (N << 3) | 7        slot N of the process's own LDT, ring 3
    the kernel:  (FIRST + N - 1) << 3   with --gdt FIRST: GDT slots, ring 0
    real mode:   BASE + (N - 1) * 0x1000  with --real BASE: for the test
                                     harness, which loads segment N there

--map names wlink's map file; its symbols become the symbol table.
--stack sets the stack size recorded in the header (default: what wlink
recorded).
"""
import argparse
import re
import struct
import sys

MAGIC = 0x0286
MODELS = {"s": 1, "m": 2, "l": 3}
MODEL_NAMES = {1: "small", 2: "medium", 3: "large"}
HDR = "<HHHHHHHHLL"            # 24 bytes
SEG = "<HHLL"                  # 12 bytes
SF_CODE, SF_DATA = 1, 2
N_TEXT, N_DATA, N_EXT = 0x02, 0x03, 0x20


def u16(b, o):
    return struct.unpack_from("<H", b, o)[0]


def read_ne(path):
    b = open(path, "rb").read()
    ne = struct.unpack_from("<L", b, 0x3C)[0] if b[:2] == b"MZ" else 0
    if b[ne:ne + 2] != b"NE":
        sys.exit("%s: not a new executable" % path)
    nseg = u16(b, ne + 0x1C)
    segtab = ne + u16(b, ne + 0x22)
    shift = u16(b, ne + 0x32) or 9
    info = {
        "entry_off": u16(b, ne + 0x14), "entry_seg": u16(b, ne + 0x16),
        "sp": u16(b, ne + 0x18), "stack_seg": u16(b, ne + 0x1A),
        "auto_data": u16(b, ne + 0x0E), "stack": u16(b, ne + 0x12),
    }
    segs = []
    for i in range(nseg):
        sector, length, flags, alloc = struct.unpack_from("<HHHH", b, segtab + 8 * i)
        off = sector << shift
        if sector == 0:
            length_b = 0
        else:
            length_b = length or 0x10000
        alloc_b = alloc or 0x10000
        data = bytearray(b[off:off + length_b])
        relocs = []
        if flags & 0x0100 and sector:
            r = off + length_b
            n = u16(b, r)
            for k in range(n):
                relocs.append(b[r + 2 + 8 * k:r + 10 + 8 * k])
        segs.append({"data": data, "alloc": max(alloc_b, length_b),
                     "is_data": bool(flags & 1), "relocs": relocs})
    return info, segs


def apply_relocs(segs, selector):
    """Fill in every segment reference; returns how many places changed."""
    done = 0
    for si, seg in enumerate(segs):
        data = seg["data"]
        for r in seg["relocs"]:
            src, flags, where = r[0], r[1], u16(r, 2)
            if flags & 3 != 0:
                sys.exit("segment %d: reference to something outside the "
                         "program (type %d); link with nodefaultlibs"
                         % (si + 1, flags & 3))
            tseg, toff = r[4], u16(r, 6)
            if tseg == 0xFF:
                sys.exit("segment %d: reference through the entry table; "
                         "not supported" % (si + 1))
            sel = selector(tseg)
            additive = flags & 4
            while True:
                nxt = u16(data, where)
                if src == 2:                      # 16-bit segment
                    val = (nxt if additive else 0) + sel
                    struct.pack_into("<H", data, where, val & 0xFFFF)
                elif src == 3:                    # 16:16 pointer
                    off = (nxt if additive else 0) + toff
                    struct.pack_into("<HH", data, where, off & 0xFFFF, sel)
                elif src == 5:                    # 16-bit offset
                    off = (nxt if additive else 0) + toff
                    struct.pack_into("<H", data, where, off & 0xFFFF)
                else:
                    sys.exit("segment %d: fixup type %d not handled"
                             % (si + 1, src))
                done += 1
                if additive or nxt == 0xFFFF:
                    break
                where = nxt
    return done


def read_map(path, segs):
    """Symbols from wlink's map: lines like `0001:0028+     _f2`."""
    syms = []
    pat = re.compile(r"^([0-9a-fA-F]{4}):([0-9a-fA-F]{4,8})([*+ ])\s+(\S+)\s*$")
    for line in open(path, errors="replace"):
        m = pat.match(line)
        if not m:
            continue
        seg, off, name = int(m.group(1), 16), int(m.group(2), 16), m.group(4)
        if not 1 <= seg <= len(segs):
            continue
        ntype = (N_DATA if segs[seg - 1]["is_data"] else N_TEXT) | N_EXT
        syms.append((name, ntype, seg, off & 0xFFFF))
    return syms


def write_out(path, model, info, segs, syms, stack):
    # Zeros at the end of a data segment need not be in the file: whatever
    # loads the program clears what the file does not cover.  The linker
    # leaves out a program's own uninitialized data already; this does
    # the same for far data, which it writes in full.
    for s in segs:
        if s["is_data"]:
            s["data"] = s["data"].rstrip(b"\0")
    strtab = bytearray(struct.pack("<L", 0))
    symtab = bytearray()
    for name, ntype, seg, off in syms:
        symtab += struct.pack("<LBbH", len(strtab), ntype, seg, off)
        strtab += name.encode() + b"\0"
    struct.pack_into("<L", strtab, 0, len(strtab))
    if not syms:
        strtab = bytearray()
    with open(path, "wb") as f:
        f.write(struct.pack(HDR, MAGIC, model, len(segs),
                            info["entry_seg"], info["entry_off"],
                            info["auto_data"], stack, 0,
                            len(symtab), len(strtab)))
        for s in segs:
            f.write(struct.pack(SEG, SF_DATA if s["is_data"] else SF_CODE, 0,
                                len(s["data"]), s["alloc"]))
        for s in segs:
            f.write(s["data"])
            if len(s["data"]) & 1:
                f.write(b"\0")
        f.write(symtab)
        f.write(strtab)


def dump(path):
    b = open(path, "rb").read()
    (magic, model, nseg, eseg, eoff, dseg, stack, _r,
     nsym, nstr) = struct.unpack_from(HDR, b, 0)
    if magic != MAGIC:
        sys.exit("%s: bad magic %#06x" % (path, magic))
    print("model %s, %d segments, entry %d:%04x, data segment %d, stack %d"
          % (MODEL_NAMES.get(model, model), nseg, eseg, eoff, dseg, stack))
    off = struct.calcsize(HDR) + nseg * struct.calcsize(SEG)
    for i in range(nseg):
        flags, _r, filesz, memsz = struct.unpack_from(
            SEG, b, struct.calcsize(HDR) + i * struct.calcsize(SEG))
        print("  %2d  %-4s  file %5d  memory %5d  at %#x"
              % (i + 1, "code" if flags & SF_CODE else "data", filesz, memsz, off))
        off += filesz + (filesz & 1)
    print("%d symbols" % (nsym // 8))
    strs = b[off + nsym:off + nsym + nstr]
    for k in range(nsym // 8):
        strx, ntype, seg, val = struct.unpack_from("<LBbH", b, off + 8 * k)
        name = strs[strx:strs.index(b"\0", strx)].decode()
        print("  %d:%04x %s %s" % (seg, val, "T" if ntype & 0x1f == N_TEXT else "D", name))


def main():
    ap = argparse.ArgumentParser(add_help=False)
    ap.add_argument("--dump")
    ap.add_argument("--model", default="s", choices=sorted(MODELS))
    ap.add_argument("--gdt", type=int)
    ap.add_argument("--real", type=lambda x: int(x, 0))
    ap.add_argument("--map")
    ap.add_argument("--stack", type=int)
    ap.add_argument("files", nargs="*")
    a = ap.parse_args()
    if a.dump:
        return dump(a.dump)
    if len(a.files) != 2:
        sys.exit(__doc__)
    info, segs = read_ne(a.files[0])
    ncode = sum(1 for s in segs if not s["is_data"])
    if a.model == "s" and ncode != 1:
        sys.exit("small model wants one code segment; this has %d" % ncode)
    # A process's table of segments has 32 slots (NSEG, sys/i286/seg.h),
    # and segment N is slot N; slot 0 is never used.
    if a.gdt is None and a.real is None and len(segs) > 31:
        sys.exit("%d segments; a program may have 31" % len(segs))
    if a.real is not None:
        selector = lambda n: a.real + (n - 1) * 0x1000
    elif a.gdt is not None:
        selector = lambda n: (a.gdt + n - 1) << 3
    else:
        selector = lambda n: (n << 3) | 7
    n = apply_relocs(segs, selector)
    syms = read_map(a.map, segs) if a.map else []
    stack = a.stack if a.stack is not None else info["stack"]
    write_out(a.files[1], MODELS[a.model], info, segs, syms, stack)
    print("%s: %d segments, %d segment references filled in, %d symbols"
          % (a.files[1], len(segs), n, len(syms)))


if __name__ == "__main__":
    main()
