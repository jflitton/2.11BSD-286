#!/usr/bin/env python3
"""Check the joins between the kernel and the network.

usage: netcheck.py --net NET.o... --kernel KERNEL.o...

The network has a data segment of its own (docs/network.md): in its code
DS is that segment, in the kernel's DS is the kernel's, and SS is always
the kernel's.  A call from one to the other that does not go through
KScall or SKcall runs with the wrong DS, and a variable one side names
that the other defines is addressed in the wrong segment.  Neither is
anything the compiler or the linker can see, and either goes wrong
quietly; this looks for both in the objects, by way of wdis:

- the network calls a kernel routine directly that is not on the list
  of those that do not depend on DS (ALLOWED);
- the network names a kernel variable;
- the kernel calls a network routine directly, or names a network
  variable.

Taking a routine's address, as the argument of KScall or SKcall, is
neither, and is fine.

And a third thing the compiler lets through: the network's locals are
on the kernel's stack, so the address of one is far (SS) unless the code
makes it a near KSPACE pointer, and a near one that is used as an
ordinary pointer reaches into the network's segment instead.  An explicit
cast such as (caddr_t)&local does that without a word.  Each address of
a stack slot that the code forms with no use of SS beside it is counted,
by function, and the counts must be those in BARE, which have been read
and found to be KSPACE pointers on purpose.  A change in them is to be
read again before BARE is changed to match.

Exits 1, listing what it found, if anything.
"""
import argparse
import re
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
    "_splclock", "_splhigh", "_splx", "_spl0",
    "_insque", "_remque", "_strlen",
    "_inb", "_outb", "_inw", "_outw", "_insb", "_insw", "_outsb", "_outsw",
    "__I4M", "__U4M", "__I4D", "__U4D",
}

# Functions that form near addresses of their stack slots on purpose, as
# KSPACE pointers or as near pointers handed to the kernel (net_mac.h),
# and how many times: read and checked.
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

# (lea sp,N[bp] is an epilogue putting the stack pointer back.)
STACKADDR = re.compile(r"\slea\s+(?!sp,)\w+,(-?0x[0-9A-Fa-f]+)?\[bp\]")
USES_SS = re.compile(r"\bss\b")
FUNC = re.compile(r"^[0-9A-F]+\s+(_\w+):$")

SEG = re.compile(r"^(\S+)\s+SEGMENT\s.*'(\w+)'")
LABEL = re.compile(r"^(\w+):|^(\w+)\s+LABEL\s")
PUBLIC = re.compile(r"^\s+PUBLIC\s+(\S+)")
EXTRN = re.compile(r"^\s+EXTRN\s+([^:\s]+)")
CALL = re.compile(r"^\s+call\s+(?:far|near) ptr (\S+)")


def scan(path):
    """What an object defines (name -> kind), refers to, and calls."""
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


def bare(path):
    """Near addresses of stack slots formed with no SS beside them."""
    out = subprocess.run(["wdis", path], capture_output=True, text=True,
                         check=True).stdout.splitlines()
    counts = {}
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
                if kind == "kernel data":
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
    found = {}
    for p in a.net:
        for fn, n in bare(p).items():
            found[fn] = found.get(fn, 0) + n
    for fn in sorted(set(found) | set(BARE)):
        if found.get(fn, 0) != BARE.get(fn, 0):
            bad.append("%s forms %d near addresses of its stack slots, not "
                       "%d: read them (docs/network.md) and change BARE"
                       % (fn, found.get(fn, 0), BARE.get(fn, 0)))
    for b in bad:
        print("netcheck: " + b, file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
