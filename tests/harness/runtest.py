#!/usr/bin/env python3
"""Build a small-model test program and run it in QEMU.

usage: runtest.py NAME SOURCE...

Each .c is compiled with tools/cc286 and each .s assembled with NASM; they
are linked behind tests/harness/start.s and converted with tools/ne2aout.py,
as any program is.  The code and data segments are then put on a diskette
image after tests/harness/boot.s, which loads them and runs the program in
real mode.  What the program writes to the serial port is printed, and the
value main returns becomes the exit status.  Everything built goes to
obj/tests/NAME.

Needs `. tools/env.sh`, nasm and qemu-system-i386.
"""
import os
import struct
import subprocess
import sys

TOP = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HARNESS = os.path.join(TOP, "tests", "harness")


def run(*cmd, **kw):
    r = subprocess.run(cmd, text=True, capture_output=True, **kw)
    if r.returncode != 0:
        sys.exit("FAILED: %s\n%s%s" % (" ".join(cmd), r.stdout, r.stderr))
    return r.stdout + r.stderr


def main():
    name, sources = sys.argv[1], sys.argv[2:]
    out = os.path.join(TOP, "obj", "tests", name)
    os.makedirs(out, exist_ok=True)

    objs = []
    for src in [os.path.join(HARNESS, "start.s")] + sources:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(out, base + ".o")
        if src.endswith(".c"):
            log = run(os.path.join(TOP, "tools", "cc286"), "-ms", "-o", obj, src)
            if "Error" in log:
                sys.exit(log)
        else:
            run("nasm", "-f", "obj", "-I", os.path.dirname(src) + "/", "-o", obj, src)
        objs.append(obj)

    ne = os.path.join(out, name + ".ne")
    mapfile = os.path.join(out, name + ".map")
    log = run("wlink", "op", "q", "format", "os2", "name", ne,
              "op", "nodefaultlibs,map=" + mapfile, "file", ",".join(objs))
    if "Error" in log:
        sys.exit(log)
    exe = os.path.join(out, name)
    run(sys.executable, "-I", os.path.join(TOP, "tools", "ne2aout.py"),
        "--model", "s", "--real", "0x1000", "--map", mapfile, ne, exe)

    # The boot sector loads 64K to 1000:0 and 64K to 2000:0 and jumps to
    # 1000:0; start.s is linked first, so that is the entry point.
    b = open(exe, "rb").read()
    magic, model, nseg, eseg, eoff = struct.unpack_from("<HHHHH", b, 0)
    if nseg != 2 or (eseg, eoff) != (1, 0):
        sys.exit("%s: want two segments and entry 1:0000, have %d and %d:%04x"
                 % (name, nseg, eseg, eoff))
    sizes = [struct.unpack_from("<L", b, 24 + 12 * i + 4)[0] for i in range(2)]
    off = 24 + 12 * 2
    boot = os.path.join(out, "boot.bin")
    run("nasm", "-f", "bin", "-o", boot, os.path.join(HARNESS, "boot.s"))
    img = os.path.join(out, name + ".img")
    with open(img, "wb") as f:
        f.write(open(boot, "rb").read())
        for size in sizes:
            f.write(b[off:off + size].ljust(0x10000, b"\0"))
            off += size + (size & 1)
        f.truncate(1474560)

    serial = os.path.join(out, "serial.txt")
    if os.path.exists(serial):
        os.remove(serial)
    try:
        r = subprocess.run(
            ["qemu-system-i386", "-display", "none", "-no-reboot",
             "-drive", "if=floppy,format=raw,file=" + img, "-boot", "a",
             "-serial", "file:" + serial,
             "-device", "isa-debug-exit,iobase=0xf4,iosize=0x01"],
            timeout=60, capture_output=True, text=True)
        code = r.returncode
    except subprocess.TimeoutExpired:
        code = None
    text = open(serial, errors="replace").read() if os.path.exists(serial) else ""
    sys.stdout.write(text)
    if code is None:
        sys.exit("%s: timed out" % name)
    # isa-debug-exit turns the byte written into (byte << 1) | 1
    status = code >> 1
    print("%s: program returned %d" % (name, status))
    sys.exit(0 if status == 0 else 1)


if __name__ == "__main__":
    main()
