#!/usr/bin/env python3
"""Look a program over before it is built for the 286.

usage: preflight.py [-v] [-n] [-j N] [-Dname[=val]] [-Uname] [-Idir]
                    [-lname] PROGRAM ...

PROGRAM is a C file, for a program of one file; or several with commas
between them; or a directory, for the sources its Makefile names (SRCS,
or OBJS), or else every .c, .y and .l in it.  A grammar is run through
the tree's own yacc or lex first (obj/host, which usr/src/i286's
Makefile builds).  The flags are the compiler's, given to every file,
besides the -D, -U and -I that the directory's Makefile has.  A
directory in which several files have a main is several programs, each
linked with the files that have none.

For each program this says what is known to go wrong with the tree's old
C here (docs/userland.md), found four ways.

  The compiler.  Every file is compiled as tools/cc compiles it, and
  what the compiler says is counted by kind.  Its errors are what must
  change before the program builds.

  Across files.  The compiler holds a call to an old-style definition's
  number and types of arguments only when both are in one file.  Between
  files, and between a program and the library, nothing does: the
  PDP-11 had lint for that.  Here the compiler is made to do it.  It is
  first asked what each function a file defines takes and returns, by a
  call that is wrong in every argument, appended to the file: its
  complaint names each type, with typedefs resolved.  Then each file is
  compiled again with a definition appended for every function it calls
  and another file defines, an empty one with the same arguments, and
  the compiler checks the file's calls against those.  The same is done
  for the C library, libutil, libtermcap, libcurses and libm, and for
  the system calls, whose arguments sys/syscalls.tab gives.  A function
  that a prototype in scope declares is left to the compiler, which
  converts its arguments.

  What is found is one of:

    wrong size   an int where a long is taken, or the like: the callee
                 reads arguments that are not there.  A bug on the PDP-11
                 too.  The same for a function returning a long or a
                 double that the caller has not declared, or that a
                 header declares as an int, where it uses the result.
    count        a different number of arguments.  Harmless where the
                 function does not look at the ones left out, since the
                 caller removes them; to be read.  A function that walks
                 its arguments from the address of the first, as an old
                 printf of a program's own does, shows up here and
                 under wrong size, and is neither.
    medium       an int where a pointer to a function is taken: the same
                 size in the small model, not in the medium.
    same size    an int where a data pointer is taken, usually a 0.
                 Counted, and listed with -v.

  Traps.  Lines that match a pattern for something that has gone wrong
  before: a conditional on the machine with nothing for the 286, a
  program that reads the kernel's memory or an executable's header,
  varargs.h, the heap taken to begin at end, a null pointer given to
  ioctl, a year printed after "19", an allocation of a bare count
  (malloc(0) is null here).  A list to read, wrong both ways.

  A trial build.  If every file compiles the program is linked, with
  whichever of the libraries it turns out to need, in the small model
  and, if it does not fit, the medium.  This gives the names the
  libraries do not have, the program's size, and the functions with
  more than 1K of local variables: the stack is 8K unless the program
  is linked with more.

    -v      list the same-size and medium findings, and every line of
            each kind of error
    -n      leave out programs that obj/bin already has
    -j N    work on N programs at once (default: the number of CPUs)

With more than one program a table follows, one line each.  What the
library's functions take is kept in obj/preflight, and worked out again
when a source or a header is newer.  Needs the toolchain
(tools/install-watcom.sh) and the libraries built.
"""
import concurrent.futures
import importlib.machinery
import importlib.util
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
TOOLS = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(TOOLS)
SRC = os.path.join(TOP, "usr", "src")
LIBC = os.path.join(SRC, "lib", "libc")
CACHE = os.path.join(TOP, "obj", "preflight")
HOST = os.path.join(TOP, "obj", "host")
WATCOM = os.path.join(TOP, "toolchain", "openwatcom")
ENV = dict(os.environ, WATCOM=WATCOM,
           PATH=os.path.join(WATCOM, "bin") + os.pathsep + os.environ["PATH"])


def run(cmd, **kw):
    """subprocess.run, without the terminal or a list of programs on
    standard input for the compiler to read."""
    return subprocess.run(cmd, stdin=subprocess.DEVNULL, **kw)


def load_cc():
    """tools/cc, for the flags it gives the compiler."""
    loader = importlib.machinery.SourceFileLoader("cc_driver",
                                                  os.path.join(TOOLS, "cc"))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    mod = importlib.util.module_from_spec(spec)
    loader.exec_module(mod)
    return mod


CC = load_cc()

# The directories the C library is made from (lib/libc/i286/Makefile);
# where two define a name, the first here is the one that is built.
LIBDIRS = ["lib/libc/i286/sys", "lib/libc/i286/gen", "lib/libc/gen",
           "lib/libc/stdio", "lib/libc/stdlib", "lib/libc/string",
           "lib/libc/compat-4.1", "lib/libc/compat-43",
           "lib/libc/compat-sys5", "lib/libc/inet", "lib/libc/net",
           "lib/libc/net/named", "lib/libc/ns", "usr.lib/libutil",
           "usr.lib/libtermlib", "usr.lib/libom", "usr.lib/libcurses"]
LIBFILES = ["lib/libc/pdp/gen/atof.c", "lib/libc/pdp/gen/ecvt.c"]
SYSTAB = os.path.join(LIBC, "i286", "sys", "syscalls.tab")
# The library's assembly: arguments as in syscalls.tab, d a double, and
# what each returns.
ASM = {"setjmp": ("p", "int"), "_setjmp": ("p", "int"),
       "longjmp": ("pi", "void"), "_longjmp": ("pi", "void"),
       "ldexp": ("di", "dbl"), "modf": ("dp", "dbl"),
       "htons": ("i", "int"), "ntohs": ("i", "int"),
       "htonl": ("l", "long"), "ntohl": ("l", "long")}
# Called with a varying number of arguments by design, though defined
# with a fixed one.
OPTIONAL = {"open", "fcntl", "execl", "execle", "execlp"}
EXTRALIBS = ["m", "curses", "termcap", "util"]

SIZE = {"int": 2, "ptr": 2, "fn": 2, "long": 4, "far": 4, "dbl": 8}
STUB = {"int": "int %s", "long": "long %s", "ptr": "void *%s",
        "fn": "int (*%s)()", "dbl": "double %s"}
LETTER = {"i": "int", "l": "long", "p": "ptr", "f": "fn", "d": "dbl"}
NAMES = {"int": "int", "long": "long", "ptr": "pointer",
         "fn": "pointer to a function", "dbl": "double"}

KEYWORDS = set("""auto break case char const continue default do double
    else enum extern float for goto if int long register return short
    signed sizeof static struct switch typedef union unsigned void
    volatile while inline_ __far __near __huge __cdecl __pascal
    __interrupt __inline _Packed""".split())

# What the compiler's errors usually want, by number.
ADVICE = {
    "E1151": "a call with a different number of arguments than the "
             "definition in the same file: make it variadic, or pass them",
    "E1071": "a call whose argument's type is not the definition's: a "
             "cast, or a bug if the sizes differ",
    "E1176": "a pointer of another type than the definition takes: a cast",
    "E1055": "a header the system does not have",
    "E1096": "a function that returns a value in one place and none in "
             "another",
    "E1062": "a function used before a declaration that gives it another "
             "type: declare it first",
    "E1129": "a definition that disagrees with its prototype",
    "E1118": "more than 64K in one file's segment",
    "E1031": "PCC let a member's name be used with any structure: a "
             "union of them, or casts",
    "E1033": "-> on something that is not a pointer to a structure, "
             "which PCC allowed",
    "E1032": ". on something that is not a structure, which PCC allowed",
    "E1011": "a name not declared: often one a header was to give",
}

TRAPS = [
    ("machine", "conditionals on the machine, to read for what the 286 "
     "takes",
     r"^\s*#\s*(?:if|ifdef|ifndef|elif)\b.*\b(?:pdp11|PDP11|pdp|vax|VAX|"
     r"tahoe|sun|mc68000|mc68k|ns32000|interdata|u370|u3b|BIT16|BSD2_10|"
     r"NONSEPARATE|MENLO_OVLY)\b"),
    ("kernel", "reads the kernel's memory or name list (docs/userland.md, "
     "\"Programs that read the kernel\")",
     r"\bnlist\s*\(|/dev/k?mem\b|_PATH_KMEM|_PATH_MEM\b|/vmunix|_PATH_UNIX"),
    ("a.out", "reads an executable's or an archive's format, which is "
     "not the PDP-11's here",
     r"<a\.out\.h>|\bstruct\s+exec\b|\bN_BADMAG\b|<ar\.h>|<ranlib\.h>"),
    ("varargs", "varargs.h, which this compiler refuses: stdarg.h",
     r"<varargs\.h>|\bva_alist\b|\bva_dcl\b"),
    ("heap", "the break, or end: the heap does not begin at end here",
     r"\bs?brk\s*\(|\bextern\b[^;]*\b(?:end|etext|edata)\b\s*[;,\[]|"
     r"&\s*end\b"),
    ("ioctl", "a null pointer for ioctl's argument, which the PDP-11 "
     r"could read through",
     r"\bioctl\s*\([^;]*,\s*(?:\([^()]*\*\s*\)\s*)?(?:0|NULL)\s*\)"),
    ("year", "a year, which may be printed after \"19\" or taken to have "
     "two digits",
     r"\"[^\"\n]*19%|\btm_year\b(?!\s*\+\s*1900)"),
    ("alloc", "an allocation of a bare count: malloc(0) is null here",
     r"\b(?:malloc|calloc|realloc)\s*\((?:[^();,]*,\s*)?"
     r"(?:\(\s*unsigned(?:\s+int)?\s*\)\s*)?[A-Za-z_][\w.>\-]*\s*\)"),
    ("asm", "assembly in the C", r"\basm\s*\(|^\s*#\s*asm\b"),
]


class Msg:
    def __init__(self, file, line, kind, code, text):
        self.file, self.line, self.kind = file, line, kind
        self.code, self.text, self.notes = code, text, []

    def note(self, code):
        """The type a note of this number gives."""
        for c, t in self.notes:
            if c == code:
                m = re.search(r"type is '(.*)'$", t)
                if m:
                    return m.group(1)
        return None


MSG = re.compile(r"^(.*)\((\d+)\): (Error|Warning|Note)! ([EWN]\d+): (.*)$")


def messages(out):
    res = []
    for line in out.splitlines():
        m = MSG.match(line)
        if not m:
            if "Error!" in line:
                res.append(Msg(None, 0, "Error", "E?", line.strip()))
            continue
        f, n, kind, code, text = m.groups()
        if kind == "Note":
            if res:
                res[-1].notes.append((code, text))
        else:
            res.append(Msg(f, int(n), kind, code, text))
    return res


def wcc(src, out, cflags, model, cwd, extra=()):
    """Run the compiler as tools/cc does; its messages."""
    cmd = [os.path.join(TOOLS, "cc286"), "-m" + model] + \
        CC.compiler_flags(cflags) + list(extra) + ["-o", out, src]
    r = run(cmd, capture_output=True, text=True, cwd=cwd,
                       env=ENV, errors="replace")
    return messages(r.stdout + r.stderr)


def check(src, cflags, model, cwd):
    return wcc(src, os.devnull, cflags, model, cwd, ["-zs", "-e=5000"])


def tclass(t):
    """The kind of thing a type is, as the compiler prints it, for how
    it is passed: int, long, ptr, fn, dbl, far, void, or agg for a
    structure and None for what is not known."""
    if t is None:
        return None
    t = t.strip()
    if t == "void":
        return "void"
    if "__far" in t or "__huge" in t:
        return "far"
    if re.search(r"\(\s*\*+\s*\)\s*\(", t):
        return "fn"
    if "*" in t or t.endswith("]"):
        return "ptr"
    if t.startswith(("struct ", "union ")):
        return "agg"
    if "double" in t or "float" in t:
        return "dbl"
    if "__int64" in t or "long long" in t:
        return "agg"
    words = set(t.replace("enum ", "int ").split()) - {
        "const", "volatile", "unsigned", "signed", "register"}
    if "long" in words and words <= {"long", "int"}:
        return "long"
    if words <= {"char", "short", "int"}:
        return "int"
    return None


TOK = re.compile(r"[A-Za-z_]\w*|\d[\w.]*|\"(?:\\.|[^\"\\])*\"|"
                 r"'(?:\\.|[^'\\])*'|\S")
LINE = re.compile(r"#\s*(?:line)?\s*(\d+)\s*\"(.*)\"")


def tokens(text):
    """The tokens of preprocessed C, each with its file and line."""
    f, n = "", 0
    for line in text.splitlines():
        if line.startswith("#"):
            m = LINE.match(line)
            if m:
                f, n = m.group(2), int(m.group(1)) - 1
            continue
        n += 1
        for t in TOK.findall(line):
            yield t, f, n


def isname(t):
    return (t[0].isalpha() or t[0] == "_") and t not in KEYWORDS


def fundef(decl):
    """If the tokens before a { at the outer level begin a function,
    its name's index, whether it is static, its number of parameters
    (None if they vary), and whether the old way's declarations of them
    have begun: those end in semicolons that do not end the function."""
    names = [d[0] for d in decl]
    if "(" not in names or "typedef" in names:
        return None
    depth = 0
    for t in names:
        depth += t == "("
        depth -= t == ")"
        if t == "=" and depth == 0:
            return None
    for j in range(len(names) - 1):
        if names[j + 1] == "(" and isname(names[j]) and \
                names[j + 2:j + 3] != ["*"]:
            break
    else:
        return None
    depth, k = 0, j + 1
    while k < len(names):
        depth += names[k] == "("
        depth -= names[k] == ")"
        if depth == 0:
            break
        k += 1
    params = names[j + 2:k]
    if not params or params == ["void"]:
        n = 0
    elif "." in params:
        n = None
    elif all(p == "," or isname(p) for p in params):
        n = sum(p != "," for p in params)
    else:
        depth, n = 0, 1
        for p in params:
            depth += p in "(["
            depth -= p in ")]"
            n += p == "," and depth == 0
    old = bool(params) and all(p == "," or isname(p) for p in params) and \
        k + 1 < len(names) and names[k + 1][0].isalpha()
    return j, "static" in names[:j], n, old


def scan(text):
    """The functions preprocessed C defines, {name: def}, and the calls
    it makes, {name: [(file, line, whether the result is used)]}."""
    toks = list(tokens(text))
    defs, refs = {}, {}
    depth, decl, body = 0, [], False
    for i, (t, f, n) in enumerate(toks):
        if depth == 0:
            if t == ";":
                d = fundef(decl)
                if d and d[3]:
                    decl.append((t, f, n))
                else:
                    decl = []
            elif t == "{":
                d = fundef(decl)
                body = d is not None
                if d and not decl[d[0]][1].endswith(".h"):
                    name, df, dn = decl[d[0]]
                    defs.setdefault(name, {
                        "n": d[2], "static": d[1], "file": df, "line": dn,
                        "params": None, "types": None, "ret": None,
                        "rtype": None})
                depth = 1
            else:
                decl.append((t, f, n))
        elif t == "{":
            depth += 1
        elif t == "}":
            depth -= 1
            if depth == 0 and body:
                decl = []
        if isname(t) and i + 1 < len(toks) and toks[i + 1][0] == "(" \
                and not f.endswith(".h"):
            p = toks[i - 1][0] if i else ";"
            used = p not in (";", "{", "}", "else", "do", ":")
            if p == ")":
                # after if (...) and the like the result is dropped; after
                # a cast it is used
                d, k = 0, i - 1
                while k >= 0:
                    d += toks[k][0] == ")"
                    d -= toks[k][0] == "("
                    if d == 0:
                        break
                    k -= 1
                used = k < 1 or toks[k - 1][0] not in ("if", "while", "for",
                                                       "switch")
                if used and toks[k + 1][0] == "void" and k + 2 == i - 1:
                    used = False
            refs.setdefault(t, []).append((f, n, used))
    return defs, refs


def probe(src, wrapper, defs, cflags, model, cwd):
    """Compile src with a call of each function it defines that is wrong
    in every argument, and fill in from the complaints what each takes
    and returns.  The messages about src itself."""
    lines = ['#include "%s"' % src, "struct __pf { char x[3]; };"]
    at = {}
    for name, d in defs.items():
        if d["static"] or d["n"] is None or name == "main":
            continue
        lines.append("#undef %s" % name)
        lines.append("static void __pf%d() { struct __pf v; v = %s(%s); }"
                     % (len(lines), name, ", ".join(["v"] * d["n"])))
        at[len(lines)] = d
    with open(wrapper, "w") as f:
        f.write("\n".join(lines) + "\n")
    own = []
    bad = set()
    for m in check(wrapper, cflags, model, cwd):
        if m.file is None or os.path.basename(m.file) != \
                os.path.basename(wrapper):
            own.append(m)
            continue
        d = at.get(m.line)
        if d is None:
            continue
        if d["types"] is None:
            d["types"] = [None] * d["n"]
        k = re.match(r"Type of parameter (\d+) ", m.text)
        if m.code == "E1071" and k and int(k.group(1)) <= d["n"]:
            d["types"][int(k.group(1)) - 1] = m.note("N2004")
        elif m.code == "E1010":
            d["rtype"] = m.note("N2003")
        elif m.code == "E1052":
            d["rtype"] = "void"
        elif m.code == "E1151":
            bad.add(m.line)
    for line, d in at.items():
        if line in bad or d["types"] is None and d["n"]:
            d["types"] = None
            continue
        d["params"] = [tclass(t) for t in d["types"] or []]
        d["ret"] = tclass(d["rtype"])
    return own


def stubbable(d):
    return d and d.get("params") is not None and \
        all(p in STUB for p in d["params"])


def show(path):
    """A file's name as the report gives it."""
    path = os.path.abspath(path)
    for base in (SRC, TOP):
        if path.startswith(base + os.sep):
            return os.path.relpath(path, base)
    return os.path.basename(path)


def library(jobs):
    """What the libraries' functions take: {name: def}."""
    files = []
    for d in LIBDIRS:
        d = os.path.join(SRC, d)
        files += sorted(os.path.join(d, f) for f in os.listdir(d)
                        if f.endswith(".c"))
    files += [os.path.join(SRC, f) for f in LIBFILES]
    newest = max(os.path.getmtime(f) for f in
                 files + [SYSTAB, os.path.abspath(__file__)])
    for d in ("usr/include", "usr/include/sys", "usr/include/machine"):
        d = os.path.join(TOP, d)
        if os.path.isdir(d):
            newest = max([newest] + [os.path.getmtime(os.path.join(d, f))
                                     for f in os.listdir(d)])
    cache = os.path.join(CACHE, "library.json")
    if os.path.exists(cache) and os.path.getmtime(cache) > newest:
        with open(cache) as f:
            return json.load(f)
    tmp = tempfile.mkdtemp(prefix="preflight.")

    def one(arg):
        i, src = arg
        flags = ["-I" + os.path.dirname(src),
                 "-I" + os.path.join(LIBC, "stdio")]
        pre = os.path.join(tmp, "%d.i" % i)
        wcc(src, pre, flags, "s", tmp, ["-pl"])
        if not os.path.exists(pre):
            return {}
        with open(pre, errors="replace") as f:
            defs, _ = scan(f.read())
        probe(src, os.path.join(tmp, "__pf%d.c" % i), defs, flags, "s", tmp)
        return {n: dict(d, where=show(src)) for n, d in defs.items()
                if not d["static"] and d["params"] is not None}
    db = {}
    try:
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            for defs in pool.map(one, enumerate(files)):
                for n, d in defs.items():
                    db.setdefault(n, d)
        # What the headers say the system calls return.
        names = []
        with open(SYSTAB) as f:
            for line in f:
                w = line.split("#")[0].split()
                if len(w) >= 2:
                    names.append((w[0].split("=")[0], w[1]))
        hdr = os.path.join(tmp, "__pfsys.c")
        with open(hdr, "w") as f:
            f.write("#include <sys/types.h>\n#include <unistd.h>\n"
                    "struct __pf { char x[3]; };\nstatic void __pfs() "
                    "{ struct __pf v;\n")
            for n, _ in names:
                f.write("v = %s;\n" % n)
            f.write("}\n")
        rets = {}
        for m in check(hdr, [], "s", tmp):
            if m.code == "E1010" and 5 <= m.line < 5 + len(names):
                t = m.note("N2003") or ""
                k = re.match(r"(.*)\(\s*\*\s*\)\s*\(.*\)$", t)
                if k:
                    rets[names[m.line - 5][0]] = k.group(1).strip()
        for n, args in names:
            if n in db or n in OPTIONAL:
                continue
            params = [] if args == "-" else [LETTER[c] for c in args]
            db[n] = {"n": len(params), "params": params, "types": params,
                     "ret": tclass(rets.get(n)), "rtype": rets.get(n),
                     "where": "a system call"}
        for n, (args, ret) in ASM.items():
            params = [LETTER[c] for c in args]
            db.setdefault(n, {"n": len(params), "params": params,
                              "types": params, "ret": ret, "rtype": ret,
                              "where": "the library's assembly"})
        for n in OPTIONAL:
            db.pop(n, None)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(CACHE, exist_ok=True)
    with open(cache, "w") as f:
        json.dump(db, f)
    return db


class Unit:
    """One source file of a program."""

    def __init__(self, path, src):
        self.path = path        # as the tree has it: a .c, .y or .l
        self.src = src          # the C the compiler is given
        self.defs, self.refs = {}, {}
        self.msgs = []          # the compiler's, about the file itself
        self.found = []         # across files
        self.unchecked = []


def makefile(d):
    """What a directory's Makefile says: sources, flags, libraries, and
    whether the PDP-11's program is overlaid."""
    srcs, flags, libs, overlaid = [], [], [], False
    path = os.path.join(d, "Makefile")
    if not os.path.exists(path):
        return srcs, flags, libs, overlaid
    with open(path, errors="replace") as f:
        text = f.read().replace("\\\n", " ")
    overlaid = bool(re.search(r"\s-Z\s|\bOV\d+\s*=|\bZ\s*=", text))
    for line in text.splitlines():
        m = re.match(r"\s*(\w+)\s*[:+]?=\s*(.*)", line)
        if not m:
            continue
        var, val = m.groups()
        words = val.replace('\\"', '"').split()
        if var in ("SRCS", "SRC", "CFILES", "OBJS", "OBJ", "OBJECTS") \
                and not srcs:
            for w in words:
                base, ext = os.path.splitext(w)
                for e in ((ext,) if ext in (".c", ".y", ".l")
                          else (".c", ".y", ".l") if ext == ".o" else ()):
                    if os.path.exists(os.path.join(d, base + e)):
                        if base + e not in srcs:
                            srcs.append(base + e)
                        break
        for w in words:
            if "$" in w:
                continue
            if re.match(r"-[DU]\w", w) and w not in flags:
                flags.append(w)
            elif w.startswith("-I") and len(w) > 2:
                w = "-I" + os.path.normpath(os.path.join(d, w[2:]))
                if w not in flags:
                    flags.append(w)
            elif re.match(r"-l\w+$", w) and w[2:] not in libs:
                libs.append(w[2:])
    return srcs, flags, libs, overlaid


def strip_comments(text):
    return re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group().count("\n"),
                  text, flags=re.S)


def traps(paths):
    """{kind: [(file, line)]} for the lines that match a trap."""
    found = {}
    for path in paths:
        try:
            with open(path, errors="replace") as f:
                text = strip_comments(f.read())
        except OSError:
            continue
        for n, line in enumerate(text.splitlines(), 1):
            if "i286" in line:
                continue
            for kind, _, pat in TRAPS:
                if re.search(pat, line):
                    found.setdefault(kind, []).append((show(path), n))
    return found


def frames(obj):
    """The functions of an object with 1K or more of local variables."""
    r = run(["wdis", obj], capture_output=True, text=True,
                       env=ENV, errors="replace")
    big, name = [], "?"
    for line in r.stdout.splitlines():
        m = re.match(r"[0-9A-Fa-f]{4}\s+(\w+):\s*$", line)
        if m:
            name = m.group(1).strip("_")
        m = re.search(r"\benter\s+0x([0-9a-f]+),", line)
        if m and int(m.group(1), 16) >= 1024:
            big.append((int(m.group(1), 16), name))
    return big


def sizes(path):
    """Code, initialized data and the rest of the data segment."""
    with open(path, "rb") as f:
        b = f.read()
    hdr, seg = "<HHHHHHHHLL", "<HHLL"
    nseg, stack = struct.unpack_from(hdr, b)[2], struct.unpack_from(hdr, b)[6]
    code = data = bss = 0
    for i in range(nseg):
        flags, _, flen, mem = struct.unpack_from(
            seg, b, struct.calcsize(hdr) + i * struct.calcsize(seg))
        if flags == 1:
            code += mem
        else:
            data, bss = flen, mem - flen
    return code, data, bss, stack


def link(objs, libs, model, out, cwd):
    cmd = [os.path.join(TOOLS, "cc"), "-m" + model, "-o", out] + objs + \
        ["-l" + n for n in libs]
    r = run(cmd, capture_output=True, text=True, cwd=cwd, env=ENV,
                       errors="replace")
    msg = r.stdout + r.stderr
    undef = sorted(set(n.lstrip("_") for n in re.findall(
        r"E2028: (\S+) is an undefined reference", msg)))
    twice = sorted(set(n.lstrip("_") for n in re.findall(
        r"W1027: .*redefinition of (\S+) ignored", msg)))
    rest = [l for l in msg.splitlines() if l.strip() and "E2028" not in l
            and "W1027" not in l and not l.startswith("file ")]
    return r.returncode == 0 and os.path.exists(out), undef, twice, rest


def build(units, libs, cflags, cwd, tag):
    """Compile and link: a dictionary of what came of it."""
    res = {"libs": [], "frames": []}
    have = {m: [n for n in EXTRALIBS + libs if os.path.exists(
        os.path.join(TOP, "obj", "lib", m, "lib%s.lib" % n))] for m in "sm"}
    res["nolib"] = [n for n in libs if n not in have["s"]]
    for model in "sm":
        objs = []
        for i, u in enumerate(units):
            obj = os.path.join(cwd, "%s%s%d.o" % (tag, model, i))
            r = run(
                [os.path.join(TOOLS, "cc"), "-m" + model, "-c"] + cflags +
                ["-o", obj, u.src], capture_output=True, text=True, cwd=cwd,
                env=ENV, errors="replace")
            if not os.path.exists(obj):
                res["failed"] = (r.stdout + r.stderr).strip()[-300:]
                return res
            objs.append(obj)
        if model == "s":
            for o in objs:
                res["frames"] += frames(o)
        out = os.path.join(cwd, tag + ".out")
        used = []
        ok, undef, twice, rest = link(objs, used, model, out, cwd)
        for n in have[model]:
            if ok or not undef:
                break
            r2 = link(objs, used + [n], model, out, cwd)
            if r2[0] or len(r2[1]) < len(undef):
                used.append(n)
                ok, undef, twice, rest = r2
        res.update(model=model, libs=used, undef=undef, twice=twice)
        if ok:
            res["size"] = sizes(out)
            return res
        if undef or twice or model == "m" or not re.search(
                r"64k|too large|too big|exceeds", " ".join(rest), re.I):
            res["msg"] = [re.sub(r"\S*/", "", l)[:200] for l in rest[:4]]
            return res
    return res


def severity(src, dst):
    if src is None or dst is None:
        return None
    a, b = SIZE.get(src), SIZE.get(dst)
    if a is None or b is None or a != b:
        return "wrong"
    return "medium" if "fn" in (src, dst) and src != dst else "same"


def cross(u, others, lib, cflags, cwd, wrapper):
    """Check a file's calls against the functions other files and the
    libraries define."""
    lines = ['#include "%s"' % u.src]
    at = {}
    for name in sorted(u.refs):
        if name in u.defs or name == "main":
            continue
        d = others.get(name) or lib.get(name)
        if d is None:
            continue
        if not stubbable(d):
            if name in others:
                u.unchecked.append(name)
            continue
        args = ["__pf%d" % k for k in range(d["n"])]
        lines.append("#undef %s" % name)
        lines.append("%s(%s) %s { }" % (name, ", ".join(args), " ".join(
            STUB[c] % a + ";" for c, a in zip(d["params"], args))))
        at[len(lines)] = (name, d)
    if not at:
        return
    with open(wrapper, "w") as f:
        f.write("\n".join(lines) + "\n")
    base = os.path.basename(wrapper)
    declared, skip = {}, set()
    calls = []
    for m in check(wrapper, cflags, "s", cwd):
        if m.file and os.path.basename(m.file) == base:
            if m.line in at:
                if m.code == "E1062":
                    declared[at[m.line][0]] = m.note("N2004")
                else:
                    skip.add(at[m.line][0])
            continue
        for c, t in m.notes:
            k = re.match(r"'(\w+)' defined in: (.*)\((\d+)\)", t)
            if c == "N2002" and k and os.path.basename(k.group(2)) == base \
                    and int(k.group(3)) in at:
                calls.append((m, at[int(k.group(3))]))
    for m, (name, d) in calls:
        if name in skip:
            continue
        where = (show(m.file), m.line)
        k = re.search(r"arameter (\d+)", m.text)
        if m.code == "E1151":
            u.found.append(("count", name, d, "takes %d" % d["n"], where))
        elif m.code == "E1071" and k and int(k.group(1)) <= d["n"]:
            i = int(k.group(1)) - 1
            src = m.note("N2003")
            sev = severity(tclass(src), d["params"][i])
            if sev:
                u.found.append((sev, name, d, "argument %d is '%s', taken "
                                "as '%s'" % (i + 1, src, d["types"][i]),
                                where))
    for name, d in at.values():
        have = declared.get(name, "int")
        if name in skip or "void" in (d["ret"], have) or d["ret"] is None:
            continue
        if name not in declared and d["ret"] == "int":
            continue
        sev = severity(tclass(have), d["ret"])
        if sev is None or tclass(have) == d["ret"]:
            continue
        used = [(show(f), n) for f, n, us in u.refs[name] if us]
        if name in declared:
            text = "declared to return '%s', returns '%s'" % (have,
                                                             d["rtype"])
            used = used or [(show(u.path), 0)]
        else:
            text = "an int to the caller, which has no other declaration: " \
                "returns '%s'" % d["rtype"]
        for where in used:
            u.found.append((sev, name, d, text, where))


def program(arg, opts, lib):
    """Everything about one program, as a dictionary for the report."""
    several = [os.path.abspath(a) for a in arg.split(",") if a]
    arg = several[0]
    rep = {"name": show(arg) + (",..." if several[1:] else ""), "notes": [],
           "builds": []}
    if several[1:]:
        d, paths, headers, flags, libs = os.path.dirname(arg), several, [], \
            [], []
    elif os.path.isdir(arg):
        d = arg
        srcs, flags, libs, overlaid = makefile(d)
        if not srcs:
            srcs = sorted(f for f in os.listdir(d)
                          if f.endswith((".c", ".y", ".l")))
        paths = [os.path.join(d, f) for f in srcs]
        headers = [os.path.join(d, f) for f in sorted(os.listdir(d))
                   if f.endswith(".h")]
        if overlaid:
            rep["notes"].append("overlaid on the PDP-11: likely to need "
                                "the medium model")
    else:
        d, paths, headers, flags, libs = os.path.dirname(arg), [arg], [], \
            [], []
    rep["files"] = len(paths)
    if not paths:
        rep["notes"].append("no C here")
        return rep
    tmp = tempfile.mkdtemp(prefix="preflight.")
    try:
        cflags = ["-I" + d, "-I" + tmp] + flags + opts.cflags
        libs = [n for n in libs + opts.libs]
        libs = ["termcap" if n in ("termlib", "termcap") else n
                for n in libs]
        units = []
        for i, path in enumerate(paths):
            base, ext = os.path.splitext(os.path.basename(path))
            src = path
            if ext in (".y", ".l"):
                tool = os.path.join(HOST, "yacc" if ext == ".y" else "lex")
                if not os.path.exists(tool):
                    rep["notes"].append("%s: no %s to run it through" % (
                        show(path), show(tool)))
                    continue
                sub = os.path.join(tmp, "g%d" % i)
                os.mkdir(sub)
                run([tool] + (["-d"] if ext == ".y" else []) +
                               [path], cwd=sub, capture_output=True)
                made = os.path.join(sub, "y.tab.c" if ext == ".y"
                                    else "lex.yy.c")
                if not os.path.exists(made):
                    rep["notes"].append("%s: %s made nothing of it" % (
                        show(path), os.path.basename(tool)))
                    continue
                src = os.path.join(tmp, "g%d_%s.c" % (i, base))
                os.rename(made, src)
                if os.path.exists(os.path.join(sub, "y.tab.h")):
                    os.rename(os.path.join(sub, "y.tab.h"),
                              os.path.join(tmp, "y.tab.h"))
            units.append(Unit(path, src))
        rep["lines"] = 0
        for i, u in enumerate(units):
            with open(u.path, errors="replace") as f:
                rep["lines"] += f.read().count("\n")
            pre = os.path.join(tmp, "%d.i" % i)
            wcc(u.src, pre, cflags, "s", tmp, ["-pl"])
            if os.path.exists(pre):
                with open(pre, errors="replace") as f:
                    u.defs, u.refs = scan(f.read())
            u.msgs = probe(u.src, os.path.join(tmp, "__pfa%d.c" % i), u.defs,
                           cflags, "s", tmp)
        # The functions one file defines for the others.  A name several
        # define, as each program in a directory has its own, is none.
        count = {}
        for u in units:
            for n, df in u.defs.items():
                if not df["static"]:
                    count[n] = count.get(n, 0) + 1
        ext = {}
        for u in units:
            for n, df in u.defs.items():
                if not df["static"] and count[n] == 1:
                    ext[n] = dict(df, where=show(u.path))
        for i, u in enumerate(units):
            cross(u, ext, lib, cflags, tmp,
                  os.path.join(tmp, "__pfb%d.c" % i))
        rep["units"] = units
        rep["traps"] = traps(paths + headers)
        rep["errors"] = sum(m.kind == "Error" for u in units for m in u.msgs)
        if rep["errors"] == 0 and units:
            mains = [u for u in units if "main" in u.defs]
            rest = [u for u in units if "main" not in u.defs]
            if len(mains) <= 1:
                rep["builds"].append((None, build(units, libs, cflags, tmp,
                                                  "b")))
            else:
                for k, u in enumerate(mains):
                    rep["builds"].append((
                        os.path.splitext(os.path.basename(u.path))[0],
                        build([u] + rest, libs, cflags, tmp, "b%d" % k)))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    return rep


def places(where, limit=8):
    """file:line, line, ... for a list of (file, line)."""
    by = {}
    for f, n in where:
        by.setdefault(f, [])
        if n and n not in by[f]:
            by[f].append(n)
    out = []
    for f, ns in by.items():
        ns.sort()
        more = ", ..." if len(ns) > limit else ""
        out.append(f + (":" + ", ".join(map(str, ns[:limit])) + more
                        if ns else ""))
    return "; ".join(out)


def k(n):
    return "%.1fK" % (n / 1024.0)


def report(rep, verbose, out):
    """Write a program's findings; its line of the table."""
    w = out.append
    units = rep.get("units", [])
    w("%s: %d file%s, %d lines" % (rep["name"], rep["files"],
                                   "" if rep["files"] == 1 else "s",
                                   rep.get("lines", 0)))
    for n in rep["notes"]:
        w("  note: " + n)
    row = {"name": rep["name"], "files": rep["files"],
           "lines": rep.get("lines", 0), "errors": 0, "wrong": 0,
           "count": 0, "other": 0, "traps": 0, "build": "-"}
    # the compiler
    by, seen = {}, set()
    for u in units:
        for m in u.msgs:
            if (m.file, m.line, m.code) not in seen:
                seen.add((m.file, m.line, m.code))
                by.setdefault((m.kind, m.code), []).append(m)
    errs = sorted(c for c in by if c[0] == "Error")
    row["errors"] = sum(len(by[c]) for c in errs)
    if not errs:
        w("  compiles")
    for c in errs:
        ms = by[c]
        w("  error %s, %d: %s" % (c[1], len(ms), ms[0].text))
        if c[1] in ADVICE:
            w("      " + ADVICE[c[1]])
        if c[1] == "E1055" or c[1] == "E?":
            for t in sorted(set(m.text for m in ms)):
                w("      " + t)
        w("      " + places([(show(m.file) if m.file else "?", m.line)
                             for m in ms], 200 if verbose else 8))
    warns = sorted(c for c in by if c[0] == "Warning")
    if warns:
        w("  warnings: " + ", ".join("%s x%d (%s)" % (
            c[1], len(by[c]), by[c][0].text[:50]) for c in warns))
    # across files
    groups = {}
    for u in units:
        for sev, name, d, text, where in u.found:
            groups.setdefault((sev, name, d["where"], text), []).append(where)
    for sev in ("wrong", "count", "medium", "same"):
        g = sorted(x for x in groups if x[0] == sev)
        n = sum(len(groups[x]) for x in g)
        if sev in ("wrong", "count"):
            row[sev] = n
        else:
            row["other"] += n
        if not g:
            continue
        title = {"wrong": "wrong size", "count": "count",
                 "medium": "wrong size in the medium model",
                 "same": "same size"}[sev]
        if sev in ("medium", "same") and not verbose:
            w("  %s: %d (-v lists them)" % (title, n))
            continue
        w("  %s: %d" % (title, n))
        for x in g:
            w("      %s (%s): %s" % (x[1], x[2], x[3]))
            w("          " + places(groups[x], 200 if verbose else 8))
    unchecked = sorted(set(n for u in units for n in u.unchecked))
    if unchecked and verbose:
        w("  not checked across files (a structure by value, or a varying "
          "list): " + ", ".join(unchecked))
    # traps
    for kind, text, _ in TRAPS:
        hits = rep.get("traps", {}).get(kind)
        if hits:
            row["traps"] += len(hits)
            w("  to read, %d: %s" % (len(hits), text))
            w("      " + places(hits, 200 if verbose else 8))
    # the trial build
    verdicts = []
    for name, b in rep["builds"]:
        pre = "  build%s: " % (" of " + name if name else "")
        if "failed" in b:
            w(pre + "does not compile as tools/cc runs it: " + b["failed"])
            verdicts.append("fails")
            continue
        libs = "".join(" -l" + n for n in b["libs"])
        if "size" in b:
            code, data, bss, stack = b["size"]
            w(pre + "links, %s model%s: code %s, data %s and %s more, "
              "stack %s" % ({"s": "small", "m": "medium"}[b["model"]],
                            libs and "," + libs, k(code), k(data), k(bss),
                            k(stack)))
            verdicts.append("links -m%s %s+%s" % (
                b["model"], k(code), k(data + bss)))
        else:
            if b.get("undef"):
                w(pre + "the libraries%s do not have: %s" % (
                    libs and " (with" + libs + ")", ", ".join(b["undef"])))
                verdicts.append("%d undefined" % len(b["undef"]))
            if b.get("twice"):
                w(pre + "defined twice with a value, in two files or in "
                  "one and a library: " + ", ".join(b["twice"]))
                verdicts.append("%d defined twice" % len(b["twice"]))
            if not b.get("undef") and not b.get("twice"):
                verdicts.append("does not link")
            for line in b.get("msg", []):
                w(pre + "the linker says: " + line)
        if b.get("nolib"):
            w("      its Makefile asks for libraries not built: " +
              ", ".join("-l" + n for n in b["nolib"]))
        if b["frames"]:
            w("      large frames: " + ", ".join(
                "%s %d" % (n, s) for s, n in sorted(b["frames"],
                                                    reverse=True)[:8]))
    if verdicts:
        row["build"] = verdicts[0] if len(set(verdicts)) == 1 and \
            len(rep["builds"]) == 1 else "%d programs: %d link" % (
                len(rep["builds"]), sum(v.startswith("links")
                                        for v in verdicts))
    elif row["errors"]:
        row["build"] = "errors"
    w("")
    return row


def main():
    class Opts:
        cflags, libs, verbose, new, jobs = [], [], False, False, \
            os.cpu_count() or 2
    opts, args = Opts, []
    argv = sys.argv[1:]
    while argv:
        a = argv.pop(0)
        if a == "-v":
            opts.verbose = True
        elif a == "-n":
            opts.new = True
        elif a == "-j" and argv:
            opts.jobs = int(argv.pop(0))
        elif a[:2] in ("-D", "-U", "-I") and len(a) > 2:
            opts.cflags.append(a)
        elif a[:2] == "-l" and len(a) > 2:
            opts.libs.append(a[2:])
        elif a.startswith("-"):
            sys.exit(__doc__)
        else:
            args.append(a)
    if not args:
        sys.exit(__doc__)
    if not os.path.exists(os.path.join(WATCOM, "bin", "wcc")):
        sys.exit("preflight: no toolchain; run tools/install-watcom.sh")
    todo = []
    for a in args:
        a = a.strip(",")
        name = os.path.splitext(os.path.basename(os.path.normpath(a)))[0]
        if os.path.isdir(a):
            if not any(f.endswith((".c", ".y", ".l")) for f in os.listdir(a)):
                continue
        elif not a.endswith((".c", ".y", ".l")):
            continue
        if opts.new and os.path.exists(os.path.join(TOP, "obj", "bin", name)):
            continue
        todo.append(a)
    lib = library(opts.jobs)
    rows = []
    with concurrent.futures.ThreadPoolExecutor(opts.jobs) as pool:
        for rep in pool.map(lambda a: program(a, opts, lib), todo):
            out = []
            rows.append(report(rep, opts.verbose, out))
            print("\n".join(out))
    if len(rows) > 1:
        wd = max(len(r["name"]) for r in rows)
        print("%-*s %5s %6s %6s %5s %5s %5s %5s  %s" % (
            wd, "program", "files", "lines", "errors", "wrong", "count",
            "other", "traps", "build"))
        for r in sorted(rows, key=lambda r: (r["errors"] > 0, r["errors"] +
                                             r["wrong"], r["name"])):
            print("%-*s %5d %6d %6d %5d %5d %5d %5d  %s" % (
                wd, r["name"], r["files"], r["lines"], r["errors"],
                r["wrong"], r["count"], r["other"], r["traps"], r["build"]))


if __name__ == "__main__":
    main()
