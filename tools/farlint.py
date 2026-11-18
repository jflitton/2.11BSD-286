#!/usr/bin/env python3
"""Look for far pointers the compiler will let the kernel truncate.

usage: farlint.py FILE.c ...

The kernel's own pointers are 16 bits, and an address in a process or in
memory outside the kernel's data segment is a far pointer, 32.  The
compiler refuses to narrow one implicitly (the kernel's Makefile makes
that warning an error), but it says nothing about two things, which this
finds by pattern:

  - a cast of something far to a near pointer or an integer;
  - a far argument to a function that has no prototype, which is pushed
    as four bytes where the function expects two.

It knows a far thing by name: a member or variable declared with __far or
uaddr_t in the file, the members listed below that headers declare far,
and mapin().  So it can be wrong both ways, and what it prints is a list
to read, not a verdict.
"""
import re
import sys

# Declared far in headers.
MEMBERS = ("iov_base ni_dirp b_kaddr ss_base ss_sp pr_base msg_bufc "
           "dk_label disklab sf_scp").split()
# Functions with prototypes that take far pointers (machine/seg.h and
# others): passing far things to these is checked by the compiler.
PROTO = set("""copyin copyout copyinstr copyoutstr copystr fubyte fuword
    subyte suword fuibyte fuiword suibyte suiword bcopy bzero bcmp uiomove
    rdwri fuptr suptr fartophys dkcksum sysctl_int sysctl_rdint sysctl_long
    sysctl_rdlong sysctl_string sysctl_rdstring sysctl_struct
    sysctl_rdstruct""".split())
NOTCALLS = set("""if while for return sizeof switch NDINIT UOFF USEL UADDR
    MIN MAX LSLOT ISLSEL""".split())
NEARCAST = (r"\((?:caddr_t|int|u_int|unsigned|short|u_short|long|"
            r"(?:struct\s+)?\w+\s*\*+)\)")


def lint(path):
    s = open(path).read()
    s = re.sub(r"/\*.*?\*/", lambda m: re.sub(r"[^\n]", " ", m.group(0)),
               s, flags=re.S)
    names = set()
    for m in re.finditer(r"__far\s*\*\s*(?:__far\s*\*\s*)?(\w+)", s):
        names.add(m.group(1))
    for m in re.finditer(r"\buaddr_t\s+([\w\s,]+);", s):
        names.update(n.strip() for n in m.group(1).split(","))
    names.discard("")
    far = r"(?:(?:\w+(?:->|\.))*(?:%s)\b(?!\s*(?:\(|->|\.|\[))|mapin\s*\()" % (
        "|".join(sorted(names | set(MEMBERS))))
    for n, line in enumerate(s.split("\n"), 1):
        if re.search(NEARCAST + r"\s*\(?\s*" + far, line):
            print("%s:%d: cast: %s" % (path, n, line.strip()))
            continue
        for m in re.finditer(r"(\w+)\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\)",
                             line):
            fn, args = m.group(1), m.group(2)
            if fn in PROTO or fn in NOTCALLS:
                continue
            if re.search(r"(?<![\w>.])" + far, args) and \
                    not re.match(r"\s*(?:register\s+)?(?:struct\s+)?\w+\s+"
                                 r"(?:__far\s*)?\*?\s*$", line[:m.start()]):
                print("%s:%d: call %s: %s" % (path, n, fn, line.strip()))


for path in sys.argv[1:]:
    lint(path)
