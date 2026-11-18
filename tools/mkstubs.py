#!/usr/bin/env python3
"""Write a program's system call stubs, in NASM, from a table.

usage: mkstubs.py TABLE > stubs.s

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

Each line of TABLE is

    NAME NUMBER ARGUMENTS [RESULT]

ARGUMENTS is a letter for each argument of the C function, or - for
none: i an int, l a long, p a data pointer, f a pointer to a function.
RESULT says what to do with what the kernel returns: nothing (the value
in AX, or DX:AX, as it is), or

    fork    AX is 0 in the child, which the kernel marks with DX = 1
    vfork   the same, for a child that shares its parent's memory
    pair    the first argument points to two ints, for AX and DX
    zero    the function returns 0

Blank lines and lines beginning with # are ignored.
"""
import sys

REGS = ["bx", "cx", "dx", "si", "di", "bp"]


def stub(name, number, args, result):
    if result == "vfork":
        # The child runs on the parent's stack until it executes another
        # program, and will have written over anything left there by the
        # time the parent returns.  So nothing is left there: the return
        # address waits in CX, which the kernel preserves.
        return ["", "        global  _%s" % name, "_%s:" % name,
                "        pop     cx", "        mov     ax, %d" % number,
                "        int     0x30", "        jc      .err",
                "        or      dx, dx", "        jz      .parent",
                "        xor     ax, ax", ".parent:",
                "        jmp     cx", ".err:   push    cx",
                "        jmp     cerror"]
    out = ["", "        global  _%s" % name, "_%s:" % name,
           "        push    bp", "        mov     bp, sp",
           "        push    si", "        push    di"]
    # The words to pass, as (how to load it into a register,).
    words = []          # ("arg", offset) | ("sel", offset) | ("cs",)
    off = 4
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
    if len(words) <= len(REGS):
        # BP, the frame pointer, is loaded last.
        for (w, reg) in zip(words, REGS):
            if w[0] == "arg":
                out.append("        mov     %s, [bp+%d]" % (reg, w[1]))
            else:
                seg = "ds" if w[0] == "sel" else "cs"
                if reg == "bp":
                    out += ["        mov     ax, %s" % seg,
                            "        cmp     word [bp+%d], 0" % w[1],
                            "        jne     .p%d" % w[1],
                            "        xor     ax, ax",
                            ".p%d:   mov     bp, ax" % w[1]]
                else:
                    out += ["        mov     %s, %s" % (reg, seg),
                            "        cmp     word [bp+%d], 0" % w[1],
                            "        jne     .p%d" % w[1],
                            "        xor     %s, %s" % (reg, reg),
                            ".p%d:" % w[1]]
    else:
        for w in reversed(words):
            if w[0] == "arg":
                out.append("        push    word [bp+%d]" % w[1])
            else:
                seg = "ds" if w[0] == "sel" else "cs"
                out += ["        mov     ax, %s" % seg,
                        "        cmp     word [bp+%d], 0" % w[1],
                        "        jne     .p%d" % w[1],
                        "        xor     ax, ax",
                        ".p%d:   push    ax" % w[1]]
        out += ["        mov     bx, sp", "        mov     cx, ss"]
    out += ["        mov     ax, %d" % number, "        int     0x30"]
    if len(words) > len(REGS):
        out.append("        lea     sp, [bp-4]      ; flags as they are")
    if result == "pair":
        # The first argument is above DI, SI, BP and the return address.
        # XOR leaves carry clear.
        out += ["        jc      .err", "        mov     bx, sp",
                "        mov     bx, [bx+8]", "        mov     [bx], ax",
                "        mov     [bx+2], dx", "        xor     ax, ax",
                ".err:"]
    out += ["        pop     di", "        pop     si", "        pop     bp",
            "        jc      cerror"]
    if result == "fork":
        out += ["        or      dx, dx", "        jz      .parent",
                "        xor     ax, ax", ".parent:"]
    elif result == "zero":
        out.append("        xor     ax, ax")
    elif result not in ("", "pair"):
        sys.exit("%s: result %r" % (name, result))
    out.append("        ret")
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = ["; Made by tools/mkstubs.py from %s: do not edit." % sys.argv[1],
           "", "        bits    16", "        cpu     286", "",
           "        segment _TEXT public align=2 class=CODE",
           "        segment _BSS public align=2 class=BSS",
           "        group   DGROUP _BSS",
           "        global  _errno", "", "        segment _BSS",
           "_errno: resw    1", "", "        segment _TEXT",
           "cerror: mov     [_errno], ax", "        mov     ax, -1",
           "        cwd", "        ret"]
    for n, line in enumerate(open(sys.argv[1]), 1):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        if len(w) not in (3, 4):
            sys.exit("%s:%d: cannot make sense of this line" % (sys.argv[1], n))
        args = "" if w[2] == "-" else w[2]
        out += stub(w[0], int(w[1]), args, w[3] if len(w) == 4 else "")
    print("\n".join(out))


if __name__ == "__main__":
    main()
