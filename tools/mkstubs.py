#!/usr/bin/env python3
"""Write system call stubs, in NASM, from a table.

usage: mkstubs.py [--sysent FILE] TABLE SYSCALL.H > stubs.s
       mkstubs.py [--sysent FILE] -d DIR TABLE SYSCALL.H

For the small memory model, which is all there is yet: C calls a stub
near with its arguments on the stack and near data pointers.  The stub
puts the call's number in AX and its arguments, a word each, in BX, CX,
DX, SI, DI and BP, and executes INT 30h (docs/processes.md).  To the
kernel a pointer is always two words, offset and then selector: the stub
passes DS, or 0 with a null pointer.  A call with more than six words of
arguments passes instead the far address of them all, which the stub
builds on its stack.

The kernel returns with carry clear and the result in AX, or DX:AX; or
with carry set and the error number in AX, which goes to errno, the C
function returning -1.

The first form writes every stub to standard output, as one file, with
errno and the error path.  The second writes each stub to a file of its
own in DIR, NAME.s, and the error path and errno to cerror.s, so that a
library made of them gives a program only the calls it makes.

Each line of TABLE is

    NAME[=CALL] ARGUMENTS [RESULT]

NAME is the C function's.  CALL is the system call's name in SYSCALL.H
(<syscall.h>), whose SYS_CALL is its number; it is NAME if not given.
ARGUMENTS is a letter for each argument of the C function, or - for
none: i an int, l a long, p a data pointer, f a pointer to a function.
RESULT says what to do with what the kernel returns: nothing (the value
in AX, or DX:AX, as it is), or

    fork    AX is 0 in the child, which the kernel marks with DX = 1
    vfork   the same, for a child that shares its parent's memory
    pair    the first argument points to two ints, for AX and DX; the
            kernel is passed nothing
    zero    the function returns 0
    errno0  errno is cleared first, because -1 can be a good result

With --sysent, the kernel's table of calls (sys/sys/init_sysent.c) is
read too, and each stub must pass as many words as the kernel takes for
its call.

Blank lines and lines beginning with # are ignored.
"""
import argparse
import os
import re
import sys

REGS = ["bx", "cx", "dx", "si", "di", "bp"]
HEAD = ["        bits    16", "        cpu     286", "",
        "        segment _TEXT public align=2 class=CODE"]


def words_of(name, args):
    """The words the kernel is passed, each ("arg", offset), ("sel",
    offset) for the selector of a data pointer at offset, or ("csel",
    offset) for that of a function pointer; offsets from the first
    argument."""
    words = []
    off = 0
    for a in args:
        if a == "i":
            words.append(("arg", off))
            off += 2
        elif a == "l":
            words += [("arg", off), ("arg", off + 2)]
            off += 4
        elif a == "p":
            words += [("arg", off), ("sel", off)]
            off += 2
        elif a == "f":
            words += [("arg", off), ("csel", off)]
            off += 2
        else:
            sys.exit("%s: argument type %r" % (name, a))
    return words


def load(reg, w, base, n):
    """Instructions that put word W in REG, the arguments being at BASE."""
    if w[0] == "arg":
        return ["        mov     %s, [%s+%d]" % (reg, base, w[1])]
    seg = "ds" if w[0] == "sel" else "cs"
    tmp = "ax" if reg == "bp" else reg
    out = ["        mov     %s, %s" % (tmp, seg),
           "        cmp     word [%s+%d], 0" % (base, w[1]),
           "        jne     .p%d" % n,
           "        xor     %s, %s" % (tmp, tmp),
           ".p%d:" % n]
    if tmp != reg:
        out.append("        mov     %s, %s" % (reg, tmp))
    return out


def stub(name, number, args, result):
    out = ["", "        global  _%s" % name, "_%s:" % name]
    if result == "vfork":
        # The child runs on the parent's stack until it executes another
        # program, and will have written over anything left there by the
        # time the parent returns.  So nothing is left there: the return
        # address waits in CX, which the kernel preserves.
        return out + ["        pop     cx", "        mov     ax, %d" % number,
                      "        int     0x30", "        jc      .err",
                      "        or      dx, dx", "        jz      .parent",
                      "        xor     ax, ax", ".parent:",
                      "        jmp     cx", ".err:   push    cx",
                      "        jmp     cerror"]
    if result == "pair":
        # The kernel is passed nothing; the argument is where the stub
        # puts the two results.
        return out + ["        mov     ax, %d" % number, "        int     0x30",
                      "        jc      cerror", "        mov     bx, sp",
                      "        mov     bx, [bx+2]", "        mov     [bx], ax",
                      "        mov     [bx+2], dx", "        xor     ax, ax",
                      "        ret"]
    words = words_of(name, args)
    if result == "errno0":
        out.append("        mov     word [_errno], 0")
    if len(words) <= 3:
        # Few enough for BX, CX and DX: no frame, and BX, the base,
        # loaded last.  The arguments are above the return address.
        out.append("        mov     bx, sp")
        regs = REGS[:len(words)]
        for k in reversed(range(len(words))):
            w = words[k]
            out += load(regs[k], (w[0], w[1] + 2), "bx", k)
        out += ["        mov     ax, %d" % number, "        int     0x30",
                "        jc      cerror"]
    else:
        out += ["        push    bp", "        mov     bp, sp",
                "        push    si", "        push    di"]
        if len(words) <= len(REGS):
            # BP, the frame pointer, is loaded last.
            for k, (w, reg) in enumerate(zip(words, REGS)):
                if reg != "bp":
                    out += load(reg, (w[0], w[1] + 4), "bp", k)
            if len(words) == 6:
                w = words[5]
                out += load("bp", (w[0], w[1] + 4), "bp", 5)
        else:
            for k in reversed(range(len(words))):
                w = words[k]
                if w[0] == "arg":
                    out.append("        push    word [bp+%d]" % (w[1] + 4))
                else:
                    out += load("ax", (w[0], w[1] + 4), "bp", k)
                    out.append("        push    ax")
            out += ["        mov     bx, sp", "        mov     cx, ss"]
        out += ["        mov     ax, %d" % number, "        int     0x30"]
        if len(words) > len(REGS):
            out.append("        lea     sp, [bp-4]      ; flags as they are")
        out += ["        pop     di", "        pop     si", "        pop     bp",
                "        jc      cerror"]
    if result == "fork":
        out += ["        or      dx, dx", "        jz      .parent",
                "        xor     ax, ax", ".parent:"]
    elif result == "zero":
        out.append("        xor     ax, ax")
    elif result not in ("", "errno0"):
        sys.exit("%s: result %r" % (name, result))
    out.append("        ret")
    return out


CERROR = ["        segment _BSS public align=2 class=BSS",
          "        group   DGROUP _BSS",
          "        global  _errno, cerror", "",
          "        segment _BSS",
          "_errno: resw    1", "",
          "        segment _TEXT",
          "; The kernel said no: the error number is in AX.",
          "cerror: mov     [_errno], ax",
          "        mov     ax, -1",
          "        cwd",
          "        ret"]


def numbers(path):
    """SYS_ names and numbers from <syscall.h>."""
    out = {}
    for line in open(path):
        m = re.match(r"#define\s+SYS_(\w+)\s+(\d+)", line)
        if m:
            out[m.group(1)] = int(m.group(2))
    return out


def sysent(path):
    """The number of argument words of each call in the kernel's table."""
    out = {}
    for line in open(path):
        m = re.match(r"\s*(?:\w+\()?(\d+),\s*\w+\)?,\s*/\*\s*(\d+) =", line)
        if m:
            out[int(m.group(2))] = int(m.group(1))
    return out


def main():
    ap = argparse.ArgumentParser(usage=__doc__)
    ap.add_argument("--sysent")
    ap.add_argument("-d")
    ap.add_argument("table")
    ap.add_argument("syscall_h")
    a = ap.parse_args()
    nums = numbers(a.syscall_h)
    counts = sysent(a.sysent) if a.sysent else None
    stubs = []
    for n, line in enumerate(open(a.table), 1):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        where = "%s:%d" % (a.table, n)
        if len(w) not in (2, 3):
            sys.exit("%s: cannot make sense of this line" % where)
        name, _, call = w[0].partition("=")
        call = call or name
        if call not in nums:
            sys.exit("%s: no SYS_%s in %s" % (where, call, a.syscall_h))
        args = "" if w[1] == "-" else w[1]
        result = w[2] if len(w) == 3 else ""
        if counts is not None:
            have = 0 if result == "pair" else len(words_of(name, args))
            want = counts.get(nums[call])
            if want != have:
                sys.exit("%s: %s passes %d words; the kernel takes %s"
                         % (where, name, have, want))
        stubs.append((name, stub(name, nums[call], args, result)))
    made = "; Made by tools/mkstubs.py from %s: do not edit." % a.table
    if a.d is None:
        out = [made, ""] + HEAD + CERROR
        for name, s in stubs:
            out += s
        print("\n".join(out))
        return
    os.makedirs(a.d, exist_ok=True)
    with open(os.path.join(a.d, "cerror.s"), "w") as f:
        f.write("\n".join([made, ""] + HEAD + CERROR) + "\n")
    for name, s in stubs:
        with open(os.path.join(a.d, name + ".s"), "w") as f:
            f.write("\n".join([made, ""] + HEAD +
                              ["        segment _BSS public align=2 class=BSS",
                               "        group   DGROUP _BSS",
                               "        extern  cerror, _errno",
                               "        segment _TEXT"] + s) + "\n")


if __name__ == "__main__":
    main()
