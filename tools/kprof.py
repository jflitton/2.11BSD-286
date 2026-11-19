#!/usr/bin/env python3
"""Say where the system spent its time: a profile by sampling, taken from
outside the machine.

usage: kprof.py [-n LINES] [-b DIRECTORY] [-m MARKS] KERNEL PROFILE

PROFILE is what tests/harness/runtest.py --profile wrote: where the
emulated processor was every few milliseconds of emulated time, and the
name of the program that was running.  KERNEL is the kernel that ran,
with its symbol table (obj/PC/unix), and DIRECTORY, by default obj/bin
beside it, has the programs with theirs.  For each sample the report
finds the function it fell in, and prints how the time divides between
processes, the kernel and the idle loop, and the LINES functions (20 if
not said) that had most of it.

With -m the report is in parts: MARKS is the marks.txt of the same run
(runtest.py --mark), and each pair of marks, the first and second, the
third and fourth and so on, bounds a part, named by what the machine had
written on the line before the first.

The machine is stopped while a sample is taken, so sampling costs the
system nothing and changes nothing it does, and a sample can fall
anywhere, in a handler or with interrupts off as well as not.  What it
cannot do:

- A function is the nearest symbol at or below the place, and the
  tables have only the names that are not static.  A static function's
  time goes to whichever one comes before it in its segment.
- A sample can only be taken a whole number of milliseconds after the
  machine started, and the clock interrupts every 16.6665 ms, 60 times
  a second.  So a sample falls at one of 50 distances from the last
  tick, a third of a millisecond apart, and they move a whole third
  only in half a minute.  What the system does at every tick in less
  time than that, the clock's own handler above all, is seen too often
  or too seldom over a shorter part: it may show as nothing, or as
  twice what it is.  Samples at even intervals would be far worse, 5 ms
  apart at one of 10 distances; hence the uneven ones runtest.py takes.
- A program is known by its name alone (u_comm): two programs with the
  same last component are one, and one that is not in DIRECTORY has no
  functions.
- The times are the emulator's notion of a 286.
"""
import collections
import os
import struct
import sys

HDR = "<HHHHHHHHLL"
SEG = "<HHLL"
GDT_KERNEL = 16         # usr/src/sys/i286/seg.h


class Exe:
    """An executable in the system's format (docs/executable-format.md):
    its segments and its symbols."""

    def __init__(self, path):
        self.b = b = open(path, "rb").read()
        (magic, _m, nseg, _es, _eo, _ds, _st, _r, symsize, strsize) = \
            struct.unpack_from(HDR, b, 0)
        if len(b) < 24 or magic != 0x0286:
            raise ValueError("%s: not an executable" % path)
        if not symsize:
            raise ValueError("%s: has no symbol table" % path)
        off = struct.calcsize(HDR) + nseg * struct.calcsize(SEG)
        self.segs = []          # (code?, file offset, bytes in file, in memory)
        for i in range(nseg):
            kind, _r, filesz, memsz = struct.unpack_from(
                SEG, b, struct.calcsize(HDR) + i * struct.calcsize(SEG))
            self.segs.append((kind == 1, off, filesz, memsz))
            off += filesz + (filesz & 1)
        stroff = off + symsize
        self.syms = {}          # name: (segment from 1, offset)
        self.text = collections.defaultdict(list)   # segment: [(offset, name)]
        for o in range(off, off + symsize, 8):
            strx, _type, seg, value = struct.unpack_from("<LBbH", b, o)
            end = b.index(b"\0", stroff + strx)
            name = b[stroff + strx:end].decode("latin-1")
            self.syms[name] = (seg, value)
            if 1 <= seg <= nseg and self.segs[seg - 1][0]:
                self.text[seg].append((value, name))
        for seg in self.text:
            self.text[seg].sort()

    def word(self, name):
        """The value the file gives the 16-bit variable NAME."""
        seg, at = self.syms[name]
        _code, off, filesz, _memsz = self.segs[seg - 1]
        if at + 2 > filesz:
            raise ValueError("%s has no value in the file" % name)
        return struct.unpack_from("<H", self.b, off + at)[0]

    def function(self, seg, ip):
        """The name of the function at IP in segment SEG, or None."""
        if not 1 <= seg <= len(self.segs) or ip >= self.segs[seg - 1][3]:
            return None
        found = None
        for value, name in self.text.get(seg, ()):
            if value > ip:
                break
            found = name
        return found


class Sampler:
    """Takes the samples: runtest.py makes one of these for --profile and
    calls sample() each time the machine has run its few milliseconds.
    ASK sends a request to 86Box's automation socket and returns the
    reply."""

    def __init__(self, ask, kernel, path):
        exe = Exe(kernel)
        self.ask = ask
        self.out = open(path, "w")
        dseg, self.uptr = exe.syms["_uptr"]
        _seg, uslot = exe.syms["_uslot"]
        # The slots end where the next thing in that segment begins.
        after = min((v for s, v in exe.syms.values() if s == dseg and v > uslot),
                    default=exe.segs[dseg - 1][3])
        self.span = after - self.uptr
        self.ucomm = exe.word("_ucomm")
        self.dslot = GDT_KERNEL + dseg - 1      # the data segment's descriptor
        self.dlimit = exe.segs[dseg - 1][3] - 1
        self.gdt = None         # where the kernel's descriptor table is,
        self.base = None        # and its data segment
        self.out.write("# kprof 1: ms, mode, cs, ip, program\n")

    def kernel(self, r):
        """Is the machine, with registers R, running the kernel or a
        process of its?  The first time it is, find the data segment."""
        if not r["cr0"] & 1:
            return False
        if self.gdt is None and r["gdtr_limit"] >= self.dslot * 8 + 7:
            d = bytes.fromhex(self.ask("memr %d 8" % (r["gdtr_base"] + self.dslot * 8))["data"])
            limit, lo, mid, access = struct.unpack_from("<HHBB", d)
            if limit == self.dlimit and access & 0x98 == 0x90:
                self.gdt, self.base = r["gdtr_base"], lo | mid << 16
        return self.gdt is not None and r["gdtr_base"] == self.gdt

    def sample(self, ms):
        r = self.ask("regs")
        if not self.kernel(r):
            # The BIOS, /boot, or the kernel before it has its tables.
            self.out.write("%d %s %04x %04x -\n" % (
                ms, "p" if r["cr0"] & 1 else "r", r["cs"], r["eip"] & 0xffff))
            return
        m = bytes.fromhex(self.ask("memr %d %d" % (self.base + self.uptr, self.span))["data"])
        at = struct.unpack_from("<H", m)[0] + self.ucomm - self.uptr
        name = m[at:at + 16].split(b"\0")[0].decode("latin-1") if 0 <= at < len(m) else ""
        if not name or not all(33 <= ord(c) < 127 for c in name):
            name = "-"
        self.out.write("%d %s %04x %04x %s\n" % (
            ms, "u" if r["cpl"] == 3 else "k", r["cs"], r["eip"] & 0xffff, name))

    def close(self):
        self.out.close()


def parts(marks, samples):
    """The samples divided by the marks: [(name, seconds, samples)]."""
    if marks is None:
        if not samples:
            return []
        return [("the whole run", (samples[-1][0] - samples[0][0]) / 1000, samples)]
    ms = []
    for line in open(marks):
        f = line.rstrip("\n").split(" ", 2)
        ms.append((int(f[0]), f[2].strip() if len(f) > 2 else ""))
    out = []
    for (t0, name), (t1, _n) in zip(ms[0::2], ms[1::2]):
        out.append((name or "?", (t1 - t0) / 1000,
                    [s for s in samples if t0 < s[0] <= t1]))
    return out


def report(name, seconds, samples, kernel, programs, lines):
    """Print where SAMPLES fell."""
    where = collections.Counter()       # (who, function): samples
    who = collections.Counter()         # (program, "user" or "system")
    kinds = collections.Counter()
    for _ms, mode, cs, ip, comm in samples:
        if mode in "rp":
            kinds["other"] += 1
            where[("before the kernel", "%s mode" % ("real" if mode == "r" else "protected"))] += 1
            continue
        if mode == "k":
            f = kernel.function((cs >> 3) - GDT_KERNEL + 1, ip)
            if f == "_idle":
                kinds["idle"] += 1
                continue
            kinds["system"] += 1
            who[(comm, "system")] += 1
            where[("kernel", f.lstrip("_") if f else "%04x:%04x" % (cs, ip))] += 1
            continue
        kinds["user"] += 1
        who[(comm, "user")] += 1
        exe = programs(comm)
        f = exe.function(cs >> 3, ip) if exe else None
        where[(comm, f.lstrip("_") if f else "?")] += 1
    n = len(samples)
    print("%s: %d samples in %.3f s" % (name, n, seconds))
    if not n:
        return
    pc = lambda k: 100.0 * k / n
    print("  " + ", ".join("%s %.1f%%" % (k, pc(kinds[k]))
                           for k in ("user", "system", "idle", "other") if kinds[k]))
    names = sorted(set(c for c, _k in who), key=lambda c: -(who[(c, "user")] + who[(c, "system")]))
    for c in names[:lines]:
        # No name: the swapper, which is also what the kernel is when
        # it has nothing to run and takes an interrupt.
        print("  %-16s user %5.1f%%  system %5.1f%%" % (
            "(no process)" if c == "-" else c, pc(who[(c, "user")]), pc(who[(c, "system")])))
    for (prog, f), k in where.most_common(lines):
        print("  %5.1f%% %6d  %s: %s" % (pc(k), k, prog, f))


def main():
    args = sys.argv[1:]
    lines, bindir, marks = 20, None, None
    while len(args) > 1 and args[0] in ("-n", "-b", "-m"):
        if args[0] == "-n":
            lines = int(args[1])
        elif args[0] == "-b":
            bindir = args[1]
        else:
            marks = args[1]
        del args[:2]
    if len(args) != 2:
        sys.exit(__doc__)
    kernel = Exe(args[0])
    if bindir is None:
        bindir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(args[0]))), "bin")
    cache = {}

    def programs(comm):
        if comm not in cache:
            try:
                cache[comm] = Exe(os.path.join(bindir, comm))
            except (OSError, ValueError):
                cache[comm] = None
        return cache[comm]

    samples = []
    for line in open(args[1]):
        if line.startswith("#"):
            continue
        ms, mode, cs, ip, comm = line.split()
        samples.append((int(ms), mode, int(cs, 16), int(ip, 16), comm))
    first = True
    for name, seconds, some in parts(marks, samples):
        if not first:
            print()
        first = False
        report(name, seconds, some, kernel, programs, lines)


if __name__ == "__main__":
    main()
