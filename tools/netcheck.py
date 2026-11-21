#!/usr/bin/env python3
"""Check the joins between the kernel and the network.

usage: netcheck.py --net NET.o... --kernel KERNEL.o...

The network has a data segment of its own (docs/network.md): in its code
DS is that segment, in the kernel's DS is the kernel's, and SS is always
the kernel's.  A call from one to the other that does not go through
KScall or SKcall runs with the wrong DS, and a variable one side names
that the other defines is addressed in the wrong segment.  Neither is
anything the compiler or the linker can see, and either goes wrong
quietly; this looks for both in the objects:

- the network calls a kernel routine directly that is not on the list
  of those that do not depend on DS (ALLOWED);
- the network names a kernel variable;
- the kernel calls a network routine directly, or names a network
  variable.

Taking a routine's address, as the argument of KScall or SKcall, is
neither, and is fine.

The objects are the cross compiler's, which are read by way of wdis, or
the system's own compiler's and assembler's (a.out.h), which are read
here: their symbols, and their relocation, which says of each word that
names a symbol where it is; a far call is the word after 9A whose
segment the next word is, and the name is reached through SS if the
instruction has the prefix for it.

And a third thing the compiler lets through: the network's locals are
on the kernel's stack, so the address of one is in SS, and if the code
makes it an ordinary near pointer it reaches into the network's segment
instead.  An explicit cast such as (caddr_t)&local does that without a
word.  The two compilers differ in what can be seen of it:

- The cross compiler makes the address of a local far, SS and offset,
  unless the code makes it a near KSPACE pointer or casts it.  Each
  address of a stack slot that the code forms with no use of SS beside
  it is counted, by function, and the counts must be those in BARE.
- The system's compiler makes it an __ss pointer, near, as KSPACE is
  there, so that it is formed the same way, lea r,[bp-n], whether it is
  kept so or is cast: neither the object nor the assembly shows the
  difference, and the compiler does.  It says, where SS is not DS, of
  each such pointer made an ordinary one, by a cast, an assignment or
  an argument, or passed to a function with no prototype, "pointer into
  SS made near, in FUNC"; the kernel's Makefile keeps what it said of
  each file in NAME.log beside NAME.o, and those are counted, by
  function, and the counts must be those in SSNEAR.

Each count has been read and found to be KSPACE pointers, or near
pointers handed to the kernel, on purpose.  A change in them is to be
read again before the table is changed to match.

Exits 1, listing what it found, if anything.
"""
import argparse
import os
import re
import struct
import subprocess
import sys

# Kernel routines the network may call directly: they reach the kernel's
# data, if they need it, through SS (mch_copy.s, mch_trap.s, mch_net.s),
# or work on the caller's DS or far pointers alone.
ALLOWED = {
    "_SKcall", "_setsoftnet",
    "_bcopy", "_bzero", "_bcmp", "_copyin", "_copyout", "_copyinstr",
    "_copyoutstr", "_copystr", "_fubyte", "_subyte", "_fuword", "_suword",
    "_splsoftclock", "_splnet", "_splbio", "_splimp", "_spltty", "_spl5",
    "_splclock", "_splhigh", "_splx", "_spl0", "_splpend",
    "_insque", "_remque", "_strlen",
    "_inb", "_outb", "_inw", "_outw", "_insb", "_insw", "_outsb", "_outsw",
    "__I4M", "__U4M", "__I4D", "__U4D",
}

# Kernel variables the network may name, in instructions that reach them
# through SS and in no others: the level interrupts are held off at and
# what is waiting for it to drop, for the spl routines, which are in
# line in the network's code as in the kernel's (machparam.h).
VIA_SS = {"_cpl", "_ipending", "_spending"}

# The cross compiler's: functions that form near addresses of their
# stack slots on purpose, as KSPACE pointers or as near pointers handed
# to the kernel (net_mac.h), and how many times: read and checked.
BARE = {
    "_ifconf": 1,           # ep, a KSPACE pointer into ifr
    "_rtalloc": 1,          # &h, for the hash routine's KSPACE argument
    "_rtrequest": 1,        # likewise
    "_sosetopt": 1,         # &m0, for the option routine's KSPACE argument
    "_ip_output": 1,        # &len, for ip_insertoptions' KSPACE argument
    "_unp_bind": 1,         # &ip, for UNPBIND: the kernel's near pointer
    "_unp_connect": 2,      # &so2, &ip, for UNPCONN
    "_unp_gc": 5,           # &xf, for FPFETCH; &file, &fileNFILE for unpgc1
    "_unp_mark": 1,         # &xf, for FPFETCH
    "_netprintf": 1,        # &fmt: the arguments, KSPACE
    "_printn": 2,           # buf, through a KSPACE pointer
}

# The system's compiler's: functions with pointers into SS that are made
# near, or passed to a function that has no prototype, on purpose, and
# how many: read and checked.  Those KSPACE pointers that go to a
# function the compiler checks with a prototype are not counted.
SSNEAR = {
    "_rtalloc": 1,          # &h, to the hash routine, whose hp is KSPACE
    "_rtrequest": 1,        # likewise
    "_in_makeaddr": 1,      # *(struct in_addr *)&addr: [bp-n], in SS
    "_ip_output": 1,        # &len, to ip_insertoptions' KSPACE phlen
    "_tcp_ctloutput": 1,    # mp, to ip_ctloutput's KSPACE m
    "_ksobind": 1,          # knam, to kmcopyin's KSPACE km
    "_ksoconnect": 1,       # likewise
    "_knameout": 1,         # knam, to kmcopyout's KSPACE km
    "_ksoaccept": 1,        # knam, to knameout's KSPACE knam
    "_ksogetnam": 1,        # likewise
    "_ksogetpeer": 1,       # likewise
    "_ksosetopt": 1,        # km, to kmcopyin
    "_ksosend": 3,          # knam and krights to kmcopyin, uio to sosend
    "_net_sysctl": 2,       # name, oldlenp, to the protocol's KSPACE ones
    "_sosetopt": 1,         # &m0, to the option routine's KSPACE m
    "_sogetopt": 1,         # mp, likewise
    "_unp_bind": 1,         # &ip, for UNPBIND: the kernel's near pointer
    "_unp_connect": 2,      # &so2, &ip, for UNPCONN
    "_unp_gc": 5,           # &file, &fileNFILE for unpgc1; &xf for FPFETCH
    "_unp_mark": 1,         # &xf, for FPFETCH
}

# (lea sp,N[bp] is an epilogue putting the stack pointer back.)
STACKADDR = re.compile(r"\slea\s+(?!sp,)\w+,(-?0x[0-9A-Fa-f]+)?\[bp\]")
USES_SS = re.compile(r"\bss\b")
FUNC = re.compile(r"^[0-9A-F]+\s+(_\w+):$")

SEG = re.compile(r"^(\S+)\s+SEGMENT\s.*'(\w+)'")
LABEL = re.compile(r"^(\w+):|^(\w+)\s+LABEL\s")
PUBLIC = re.compile(r"^\s+PUBLIC\s+(\S+)")
EXTRN = re.compile(r"^\s+EXTRN\s+([^:\s]+)")
CALL = re.compile(r"^\s+call\s+(?:far|near) ptr (\S+)")

SSNOTE = re.compile(r": warning: pointer into SS made near, in (\w+)$")


def native(path):
    """Whether an object is one of the system's own (A_OBJECT)."""
    with open(path, "rb") as f:
        return f.read(2) == b"\x86\x01"


class Object:
    """One of the system's objects (sys/exec.h, a.out.h): its segments,
    their contents and relocation, and its symbols."""

    N_UNDF, N_TEXT, N_DATA, N_BSS = 0, 2, 3, 4
    N_GRPNAME, N_SEGNAME, N_EXT = 0x1D, 0x1E, 0x20
    R_OFF, R_SEG, R_PCREL, R_EXT = 1, 2, 3, 0x10

    def __init__(self, path):
        data = open(path, "rb").read()
        nseg, = struct.unpack_from("<H", data, 4)
        syms, strs = struct.unpack_from("<2L", data, 16)
        segs = [struct.unpack_from("<2H2L", data, 24 + 12 * i)
                for i in range(nseg)]
        off = 24 + 12 * nseg
        self.code = []          # (is code, contents) of each segment
        for kind, _, filesz, _ in segs:
            self.code.append((kind == 1, data[off:off + filesz]))
            off += (filesz + 1) & ~1
        self.rels = []          # (segment from 0, addr, symbol, type)
        for i, (_, nrel, _, _) in enumerate(segs):
            for j in range(nrel):
                a, s, t = struct.unpack_from("<3H", data, off + 6 * j)
                self.rels.append((i, a, s, t))
            off += 6 * nrel
        strtab = data[off + syms:off + syms + strs]
        self.syms = []          # (name, type, segment, value)
        for i in range(syms // 8):
            x, t, ov, v = struct.unpack_from("<LBbH", data, off + 8 * i)
            name = strtab[x:strtab.index(b"\0", x)].decode() if x else ""
            self.syms.append((name, t, ov, v))
        self.segname = {ov: n for n, t, ov, _ in self.syms
                        if t & 0x1F == self.N_SEGNAME}

    def kind(self, t, ov):
        """Whether a defined symbol is code, the network's data or the
        kernel's."""
        t &= 0x1F
        if t == self.N_TEXT:
            return "code"
        if t == self.N_DATA and self.segname.get(ov, "").startswith("NET"):
            return "network data"
        return "kernel data"

    def func(self, seg, addr):
        """The function of the text that addr is in."""
        best = None
        for n, t, _, v in self.syms:
            if t & 0x1F == self.N_TEXT and v <= addr and n and \
                    (best is None or v > best[1]):
                best = (n, v)
        return best[0] if best else "?"

    def refs(self):
        """Each word that names a symbol by its name: (name, segment,
        addr, type, whether it is the address of a far call or jump)."""
        segat = {(i, a): s for i, a, s, t in self.rels
                 if t == self.R_SEG | self.R_EXT}
        out = []
        for i, a, s, t in self.rels:
            if not t & self.R_EXT:
                continue
            name = self.syms[s][0]
            code, text = self.code[i]
            call = t & 0xF == self.R_PCREL or (
                code and t & 0xF == self.R_OFF and a > 0
                and text[a - 1] in (0x9A, 0xEA)
                and segat.get((i, a + 2)) == s)
            out.append((name, i, a, t & 0xF, call))
        return out

    def via_ss(self, seg, a):
        """Whether the instruction with its operand's address at a has
        the prefix that makes the segment SS: 36 A0-A3 (mov with the
        accumulator), or 36, the opcode and a ModRM byte of a 16-bit
        address alone or after a register."""
        _, text = self.code[seg]
        if a >= 2 and 0xA0 <= text[a - 1] <= 0xA3:
            return text[a - 2] == 0x36
        if a >= 3 and (text[a - 1] & 0xC7 == 0x06 or
                       text[a - 1] & 0xC0 == 0x80):
            return text[a - 3] == 0x36
        return False


def scan(path):
    """What an object defines (name -> kind), refers to, and calls."""
    if native(path):
        o = Object(path)
        defs, externs, calls = {}, set(), set()
        for n, t, ov, v in o.syms:
            if not t & o.N_EXT:
                continue
            if t & 0x1F == o.N_UNDF:
                if v:           # common: the kernel's, as the network
                    defs[n] = "kernel data"   # has none (-nd)
                externs.add(n)
            elif t & 0x1F in (o.N_TEXT, o.N_DATA, o.N_BSS):
                defs[n] = o.kind(t, ov)
        for n, _, _, _, call in o.refs():
            if call:
                calls.add(n)
        return defs, externs, calls
    out = subprocess.run(["wdis", "-a", path], capture_output=True,
                         text=True, check=True).stdout
    publics, kinds, externs, calls = set(), {}, set(), set()
    seg = cls = None
    for line in out.splitlines():
        m = SEG.match(line)
        if m:
            seg, cls = m.group(1), m.group(2)
            continue
        m = PUBLIC.match(line)
        if m:
            publics.add(m.group(1))
            continue
        m = EXTRN.match(line)
        if m:
            externs.add(m.group(1))
            continue
        m = CALL.match(line)
        if m:
            calls.add(m.group(1))
            continue
        m = LABEL.match(line)
        if m and seg:
            name = m.group(1) or m.group(2)
            if cls == "CODE":
                kinds[name] = "code"
            elif seg.startswith("NET"):
                kinds[name] = "network data"
            else:
                kinds[name] = "kernel data"
    defs = {n: kinds.get(n, "?") for n in publics}
    return defs, externs, calls


def notss(path, name):
    """Instructions that name a VIA_SS variable without going through SS."""
    if native(path):
        o = Object(path)
        return ["%s+0x%x" % (o.func(i, a), a)
                for n, i, a, t, call in o.refs()
                if n == name and not o.via_ss(i, a)]
    out = subprocess.run(["wdis", path], capture_output=True, text=True,
                         check=True).stdout.splitlines()
    names = re.compile(r"\b%s\b" % name)
    return [line.strip() for line in out
            if names.search(line) and re.match(r"^[0-9A-F]{4}\s", line)
            and not USES_SS.search(line)]


def bare(path):
    """Near addresses of stack slots formed with no SS beside them, by
    the cross compiler; or, by the system's, pointers into SS that it
    said it made near."""
    counts = {}
    if native(path):
        log = os.path.splitext(path)[0] + ".log"
        if os.path.exists(log):
            for line in open(log):
                m = SSNOTE.search(line.rstrip("\n"))
                if m:
                    fn = "_" + m.group(1)
                    counts[fn] = counts.get(fn, 0) + 1
        return counts
    out = subprocess.run(["wdis", path], capture_output=True, text=True,
                         check=True).stdout.splitlines()
    fn = None
    for i, line in enumerate(out):
        m = FUNC.match(line)
        if m:
            fn = m.group(1)
            continue
        if not STACKADDR.search(line):
            continue
        near = out[max(0, i - 2):i + 3]
        if any(USES_SS.search(l.split(";")[0]) for l in near):
            continue
        counts[fn] = counts.get(fn, 0) + 1
    return counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--net", nargs="+", required=True)
    ap.add_argument("--kernel", nargs="+", required=True)
    a = ap.parse_args()
    objs = {}
    owner = {}
    for side, paths in (("network", a.net), ("kernel", a.kernel)):
        for p in paths:
            objs[p] = (side,) + scan(p)
            for name, kind in objs[p][1].items():
                if name not in owner or kind != "kernel data":
                    owner[name] = (side, kind, p)
    bad = []
    for p, (side, defs, externs, calls) in sorted(objs.items()):
        for name in sorted(externs):
            if name not in owner:
                continue
            oside, kind, where = owner[name]
            if oside == side:
                continue
            if side == "network":
                if kind == "kernel data" and name in VIA_SS:
                    for line in notss(p, name):
                        bad.append("%s: names the kernel's %s, not through "
                                   "SS: %s" % (p, name, line))
                elif kind == "kernel data":
                    bad.append("%s: names the kernel's variable %s (%s)"
                               % (p, name, where))
                elif kind == "code" and name in calls and name not in ALLOWED:
                    bad.append("%s: calls the kernel's %s directly (%s)"
                               % (p, name, where))
            else:
                if kind == "network data":
                    bad.append("%s: names the network's variable %s (%s)"
                               % (p, name, where))
                elif kind == "code" and name in calls:
                    bad.append("%s: calls the network's %s directly (%s)"
                               % (p, name, where))
    kinds = {native(p) for p in a.net}
    if len(kinds) > 1:
        bad.append("the network's objects are of both compilers")
    if True in kinds:
        table, name = SSNEAR, "SSNEAR"
        what = "%s makes a pointer into SS near %d times, not %d"
    else:
        table, name = BARE, "BARE"
        what = "%s forms %d near addresses of its stack slots, not %d"
    found = {}
    for p in a.net:
        for fn, n in bare(p).items():
            found[fn] = found.get(fn, 0) + n
    for fn in sorted(set(found) | set(table)):
        if found.get(fn, 0) != table.get(fn, 0):
            bad.append(what % (fn, found.get(fn, 0), table.get(fn, 0)) +
                       ": read them (docs/network.md) and change " + name)
    for b in bad:
        print("netcheck: " + b, file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
