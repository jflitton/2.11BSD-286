#!/usr/bin/env python3
"""Look for what the large model breaks in C written for near pointers.

usage: lmlint.py [-Dname[=val]] [-Idir] [-k FILE.c] ... FILE.c ...

In the small and medium models a pointer and an int are both 16 bits,
and the tree's older C counts on it where no compiler can see: a 0
passed for a pointer to a function that has no prototype, which is
pushed as two bytes where the function takes four; a function that
returns a pointer and is called without being declared, so that half
of what it returns is dropped; a pointer kept in an int.  A program is
moved to the large model after these are found and put right
(docs/memory-model.md), and this finds them, or most.

Each FILE is given to the cross compiler as tools/cc -ml would give
it, with every warning on and nothing written.  The compiler says where
a function is called that has no prototype in scope, and where one is
called that was never declared.  Those calls are then read, and their
arguments compared with what the function is defined to take: its
definition is looked for in the FILEs, in those named with -k, which
are read for their definitions and not checked, in the C library's
sources and its table of system calls.  Reported:

    0 for a pointer     a constant where the function takes a pointer
    pointer for an int  a string, an address or NULL where it takes an
                        int: NULL is 0L in the large model (tools/cc286.h)
    count               more or fewer arguments than it is defined with
    not declared        the function returns a pointer, or a long, and
                        was not declared where it is called
    not known           a function called with no prototype whose
                        definition was not found: for a look
    conversion          the compiler's own complaint that a pointer and
                        an integer were mixed without a cast

A cast is taken to be meant, and the compiler says nothing of one; so
last, by pattern alone: a cast to an integer type of a name that the
file declares as a pointer.

What it prints is a list to read, not a verdict: an argument that is an
expression is not checked, and a function with a variable number of
arguments is known only by its first.  Needs `. tools/env.sh`.
"""
import glob
import os
import re
import subprocess
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(TOOLS)
LIBC = os.path.join(TOP, "usr", "src", "lib", "libc")
MACROS = ["interrupt", "far", "near", "huge", "cdecl", "pascal", "fortran",
          "_leave", "_try", "_except", "_finally"]
# Types that are pointers without a star.
POINTERS = set("caddr_t jmp_buf va_list sig_t sigjmp_buf".split())
WIDE = set("long off_t time_t daddr_t u_long double float".split())
KEYWORDS = set("if while for switch return sizeof else do case".split())
VARARGS = set("""printf fprintf sprintf scanf fscanf sscanf execl execle
    execlp syslog err errx warn warnx snprintf open fcntl ioctl
    wprintw mvprintw mvwprintw printw""".split())


def strip(text):
    """TEXT without comments, with strings and character constants made
    harmless, and preprocessor lines blanked; newlines are kept."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", text[i:j]))
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            body = text[i + 1:j]
            out.append(c + re.sub(r"[^\n]", "_", body) + c)
            i = j + 1
        else:
            out.append(c)
            i += 1
    text = "".join(out)
    lines = text.split("\n")
    k = 0
    while k < len(lines):
        if lines[k].lstrip().startswith("#"):
            while lines[k].rstrip().endswith("\\") and k + 1 < len(lines):
                lines[k] = ""
                k += 1
            lines[k] = ""
        k += 1
    return "\n".join(lines)


def split_args(s):
    """The pieces of S between its top-level commas."""
    out, depth, cur = [], 0, []
    for c in s:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(c)
    last = "".join(cur).strip()
    if last or out:
        out.append(last)
    return out


def kind(decl):
    """p for a pointer, l for something of four bytes or more, else i."""
    if "*" in decl or "[" in decl or "(" in decl:
        return "p"
    words = set(re.findall(r"\w+", decl))
    if words & POINTERS:
        return "p"
    if words & WIDE:
        return "l"
    return "i"


TAIL = re.compile(r"(\w+)\s*\(([^()]*)\)\s*((?:[^;{}]*;)*)\s*$", re.S)
TYPEWORDS = set("""int char long short unsigned void double float struct
    union register static extern signed const""".split())


def definitions(path, db):
    """Add to DB the functions PATH defines: name -> (what it returns,
    [what each parameter is]).  A definition is what stands before a
    brace at the top level: a name, its parameters in parentheses, and
    in older C their declarations."""
    try:
        text = strip(open(path, errors="replace").read())
    except OSError:
        return
    depth = 0
    start = 0
    for i, c in enumerate(text):
        if c == "{":
            if depth == 0:
                head(db, text[start:i])
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                start = i + 1


def head(db, seg):
    """SEG is everything since the last definition: the function's name
    is the last in it that, with what follows, reads as one."""
    for m in reversed(list(re.finditer(r"\b\w+\s*\(", seg))):
        t = TAIL.match(seg, m.start())
        if not t or t.group(1) in TYPEWORDS or t.group(1) in KEYWORDS:
            continue
        ret = seg[:m.start()].rsplit(";", 1)[-1]
        if "=" in ret or ")" in ret:
            continue
        add(db, ret, t.group(1), t.group(2), t.group(3))
        return


def add(db, ret, name, params, decls):
    ret = kind(ret) if "*" in ret or set(re.findall(r"\w+", ret)) & \
        (POINTERS | WIDE) else "i"
    names = [p.strip() for p in params.split(",")] if params.strip() else []
    if names == ["void"]:
        names = []
    kinds = []
    if all(re.fullmatch(r"\w+", p) for p in names):
        # K&R: the types follow
        types = {}
        for d in decls.split(";"):
            d = d.strip()
            if not d:
                continue
            mm = re.match(r"((?:register\s+|unsigned\s+|struct\s+\w+\s+|"
                          r"union\s+\w+\s+|const\s+)*\w*)\s*(.*)$", d, re.S)
            base, rest = mm.group(1), mm.group(2)
            if not rest:
                base, rest = "int", base
            for item in split_args(rest):
                nm = re.findall(r"\w+", re.sub(r"\[[^\]]*\]", "[]", item))
                if nm:
                    types[nm[0] if "(" not in item else
                          re.search(r"\(\s*\*+\s*(\w+)", item).group(1)
                          if re.search(r"\(\s*\*+\s*(\w+)", item) else nm[0]] = \
                        kind(base + " " + item)
        kinds = [types.get(p, "i") for p in names]
    else:
        kinds = [kind(p) for p in names if p != "..."]
        if "..." in names:
            VARARGS.add(name)
    db.setdefault(name, (ret, kinds))


def syscalls(db):
    """The system call stubs, which no C defines."""
    tab = os.path.join(LIBC, "i286", "sys", "syscalls.tab")
    for line in open(tab):
        w = line.split()
        if not w or w[0].startswith("#"):
            continue
        name = w[0].partition("=")[0]
        args = "" if w[1] == "-" else w[1]
        ret = "p" if len(w) > 2 and w[2] == "null" else "i"
        db.setdefault(name, (ret, ["p" if a in "pPf" else "l" if a == "l"
                                   else "i" for a in args]))


def call_args(lines, lineno, name):
    """The arguments of the call of NAME on or after line LINENO of the
    stripped text, as a list, or None."""
    text = "\n".join(lines[lineno - 1:lineno + 12])
    for m in re.finditer(r"\b%s\s*\(" % re.escape(name), text):
        depth, j = 1, m.end()
        while j < len(text) and depth:
            depth += text[j] in "([{"
            depth -= text[j] in ")]}"
            j += 1
        if depth == 0:
            return split_args(text[m.end():j - 1])
    return None


CONST = re.compile(r"\(?\s*-?(0[xX][0-9a-fA-F]+|\d+)\s*\)?$|'")
POINTY = re.compile(r"\"|&\s*\w|NULL\s*$")


def check(path, flags, db, out):
    cmd = [os.path.join(TOOLS, "cc286"), "-ml",
           "-I" + os.path.join(TOP, "usr", "include")] + flags + \
        ["-wx", "-wcd=107", "-wcd=121", "-wcd=303", "-wcd=304", "-wcd=1179",
         "-wcd=1178", "-wcd=137", "-wcd=202", "-wcd=400"] + \
        ["-U" + k for k in MACROS] + ["-Dinline=inline_", "-zs", "-o",
                                      os.devnull, path]
    r = subprocess.run(cmd, capture_output=True, text=True)
    said = r.stdout + r.stderr
    raw = open(path, errors="replace").read()
    lines = strip(raw).split("\n")
    seen = set()
    pat = re.compile(r"^(.*)\((\d+)\): (Warning|Error)! ([EW]\d+): (.*)$")
    for line in said.splitlines():
        m = pat.match(line)
        if not m:
            continue
        where, n, _, code, msg = m.groups()
        n = int(n)
        if os.path.abspath(where) != os.path.abspath(path):
            continue
        loc = "%s:%d" % (os.path.relpath(path, TOP), n)
        if code in ("W131", "W308"):
            name = re.search(r"'(\w+)'", msg).group(1)
            if (n, name, code) in seen:
                continue
            seen.add((n, name, code))
            known = db.get(name)
            # what a call that is a statement by itself returns is not
            # looked at
            alone = re.match(r"\s*(\(\s*void\s*\)\s*)?%s\s*\(" % name,
                             lines[n - 1])
            if code == "W131" and known and known[0] != "i" and not alone:
                out.append("%s: not declared: %s returns %s" % (
                    loc, name, "a pointer" if known[0] == "p" else "a long"))
            if code == "W131" and (n, name, "W308") in seen:
                continue
            if code == "W308" and (n, name, "W131") in seen:
                continue
            if not known:
                out.append("%s: not known: %s" % (loc, name))
                continue
            args = call_args(lines, n, name)
            if args is None:
                continue
            want = known[1]
            if len(args) != len(want) and name not in VARARGS:
                out.append("%s: count: %s is given %d, and takes %d" % (
                    loc, name, len(args), len(want)))
            for k, (a, w) in enumerate(zip(args, want), 1):
                if w == "p" and CONST.match(a):
                    out.append("%s: 0 for a pointer: argument %d of %s: %s"
                               % (loc, k, name, a))
                elif w == "i" and POINTY.match(a):
                    out.append("%s: pointer for an int: argument %d of %s: %s"
                               % (loc, k, name, a))
            if name in VARARGS:
                for k, a in enumerate(args[len(want):], len(want) + 1):
                    if re.fullmatch(r"\(?\s*0\s*\)?", a):
                        out.append("%s: 0 for a pointer, perhaps: argument "
                                   "%d of %s" % (loc, k, name))
        elif code == "W102" or code.startswith("E"):
            out.append("%s: %s: %s" % (
                loc, "conversion" if code == "W102" else "error", msg))
    # Casts of what the file declares as a pointer.
    ptrs = set()
    for m in re.finditer(r"\*+\s*(\w+)\s*(?:[,;=\[)]|$)", "\n".join(lines),
                         re.M):
        ptrs.add(m.group(1))
    cast = re.compile(r"\(\s*(?:unsigned\s+|u_)?(?:int|short|unsigned|long)\s*\)"
                      r"\s*(\w+)\b(?!\s*[\(\[\.]|\s*->)")
    for n, l in enumerate(lines, 1):
        for m in cast.finditer(l):
            if m.group(1) in ptrs:
                out.append("%s:%d: cast: (integer)%s, which is a pointer "
                           "somewhere in the file" % (
                               os.path.relpath(path, TOP), n, m.group(1)))


def main():
    flags, files, known = [], [], []
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        a = args[i]
        i += 1
        if a == "-k":
            known.append(args[i])
            i += 1
        elif a[:2] in ("-D", "-I", "-U"):
            flags.append(a)
        elif a.startswith("-"):
            sys.exit(__doc__.split("\n\n")[1])
        else:
            files.append(a)
    if not files:
        sys.exit(__doc__.split("\n\n")[1])
    db = {}
    for f in files + known:
        definitions(f, db)
    for f in sorted(glob.glob(os.path.join(LIBC, "**", "*.c"), recursive=True)):
        if os.sep + "pdp" + os.sep + "sys" in f or "compat-2.9" in f:
            continue
        definitions(f, db)
    syscalls(db)
    out = []
    for f in files:
        check(f, flags, db, out)
    for line in out:
        print(line)
    return 1 if out else 0


if __name__ == "__main__":
    sys.exit(main())
