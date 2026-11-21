#!/usr/bin/env python3
"""Compare how the two compilers lay structures out.

usage: layouts.py headers DIR [HEADER ...]
       layouts.py programs DIR [PROGRAM ...]
       layouts.py tools

The system is built by its own compiler or by the cross compiler, and
what one's system writes the other's reads: a structure that is in a
file, on the network, or between the kernel and a program is to be laid
out alike by both.  They do not lay out every structure alike
(docs/unported.md).  The first form finds every structure and union
that the headers define, in usr/include and in the kernel's
(usr/src/sys/h, i286, i286at, i286if, net, netinet), or in those
HEADERs alone: in usr/include as a program sees them, in the small, the
medium and the large model, and in the kernel's as the kernel does, in
the medium.  For each it has each compiler compile the size, each
member's offset, and for a bit field the bits a store of ones to it
sets, and prints what differs, with how many it compared.  DIR has the
files made on the way, and results.json.  The second does the same for
what the programs' own sources define (usr/src/i286/Makefile's PROGS,
or those PROGRAMs), in each program's model and with its flags.

What a file defines is read from clang's syntax tree (clang -Xclang
-ast-dump=json) of what the system's preprocessor makes of it, so clang
must be on PATH, and the cross compiler (tools/env.sh).  A header is
given first the headers the sources that include it include before it,
when it cannot be compiled alone or after <sys/types.h>, <sys/param.h>
or that and <sys/socket.h>; and a member one compiler has and the other
has not is left out.

The third form checks the host tools that write a structure of the
system's in a file, tools/mkfs.py and the rest (TOOLS, below), against
the system's compiler's layout of it, in the model of the program that
reads it, and against the cross compiler's if it is there: each member
where the tool puts it, and the whole its size.  It exits 1 if a tool
differs from either.  Needs only tools/ncc.
"""
import concurrent.futures
import glob
import json
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
TOP = os.path.dirname(TOOLS_DIR)
INC = os.path.join(TOP, "usr", "include")
SYS = os.path.join(TOP, "usr", "src", "sys")

USER_DIRS = ["", "arpa", "protocols", "sys", "machine", "net", "netinet"]
KERN_DIRS = ["h", "i286", "i286at", "i286if", "net", "netinet"]
# Headers that are not for the 286, or have nothing laid out.
SKIP = {"vfont.h", "pcc.h", "mp.h", "struct.h", "frame.h", "float.h",
        "varargs.h", "stdarg.h", "iso646.h"}

KFLAGS = ["-DKERNEL", "-DINET", "-DUCB_METER", "-I" + SYS + "/PC",
          "-I" + SYS + "/h", "-I" + INC]
# What tools/cc gives the cross compiler besides.
XFLAGS = ["-wcd=131", "-wcd=107", "-wcd=121", "-Uinterrupt", "-Ufar",
          "-Unear", "-Uhuge", "-Ucdecl", "-Upascal", "-Ufortran", "-U_leave",
          "-U_try", "-U_except", "-U_finally", "-Dinline=inline_"]


def nflags(kernel):
    """What the system's compiler is given: the kernel's flags, none, or
    a program's (a list)."""
    if isinstance(kernel, list):
        return kernel
    return ["-k"] + KFLAGS if kernel else []


def xflags(kernel):
    if isinstance(kernel, list):
        return ["-I" + INC] + kernel
    return KFLAGS if kernel else ["-I" + INC]


def ncc(src, model, kernel, out, pre=False):
    """The system's compiler on src, in its directory, so that what it
    says names the file as it is."""
    cmd = [os.path.join(TOOLS_DIR, "ncc"), "-E" if pre else "-S",
           "-m" + model] + nflags(kernel)
    return subprocess.run(cmd + ["-o", os.path.abspath(out),
                                 os.path.basename(src)],
                          cwd=os.path.dirname(os.path.abspath(src)),
                          capture_output=True, text=True)


def xcc(src, model, kernel, out):
    cmd = [os.path.join(TOOLS_DIR, "cc286"), "-m" + model] + \
        xflags(kernel) + XFLAGS
    return subprocess.run(cmd + ["-o", os.path.abspath(out),
                                 os.path.basename(src)],
                          cwd=os.path.dirname(os.path.abspath(src)),
                          capture_output=True, text=True)


def bits(off, size, mask):
    """A store's mask as the bytes, and the bits of each, it sets."""
    return " ".join("%d:%02x" % (off + i, (mask >> 8 * i) & 0xFF)
                    for i in range(size) if (mask >> 8 * i) & 0xFF)


def native_values(path):
    """The values of L<n>: an int's initial value, or what a function's
    store to a bit field sets."""
    v = {}
    lines = open(path).read().splitlines()
    fn = None
    for i, l in enumerate(lines):
        m = re.match(r"^_(L\d+):$", l)
        if m:
            fn = m.group(1)
            m2 = re.match(r"^\s+dw\s+(-?\d+)", lines[i + 1]) \
                if i + 1 < len(lines) else None
            if m2:
                v[fn] = int(m2.group(1)) & 0xFFFF
            continue
        m = re.match(r"^\s+(?:or|mov)\s+(byte|word)\s+\[(?:ss:)?_BV\d+"
                     r"(?:\+(\d+))?\],(-?\d+)$", l)
        if m and fn and fn not in v:
            size = 1 if m.group(1) == "byte" else 2
            v[fn] = bits(int(m.group(2) or 0), size,
                         int(m.group(3)) & (0xFF if size == 1 else 0xFFFF))
    return v


def cross_values(path):
    out = subprocess.run(["wdis", path], capture_output=True,
                         text=True).stdout.splitlines()
    v = {}
    pending = []        # labels at one place, as the compiler merges them
    for l in out:
        m = re.match(r"^[0-9A-F]{4}\s+_(L\d+):$", l)
        if m:
            pending.append(m.group(1))
            continue
        m = re.search(r"\sor\s+(byte|word) ptr (?:ss:)?_BV\d+"
                      r"(?:\+0x([0-9a-f]+))?,0x([0-9a-f]+)$", l)
        if m and pending:
            size = 1 if m.group(1) == "byte" else 2
            for fn in pending:
                v[fn] = bits(int(m.group(2) or "0", 16), size,
                             int(m.group(3), 16))
            pending = []
    for i, l in enumerate(out[:-1]):
        m = re.match(r"^[0-9A-F]{4}\s+_(L\d+):$", l)
        m2 = re.match(r"^[0-9A-F]{4}\s+([0-9A-F]{2}) ([0-9A-F]{2})\s",
                      out[i + 1])
        if m and m2 and m.group(1) not in v:
            v[m.group(1)] = int(m2.group(2) + m2.group(1), 16)
    return v


def include(pre, h, kernel):
    out = ""
    for x in pre + [h]:
        if x.startswith("/"):
            out += '#include "%s"\n' % x
        elif kernel:
            out += '#include "%s/%s"\n' % (SYS, x)
        else:
            out += "#include <%s>\n" % x
    return out


def preambles(h, kernel):
    """What to include before h: nothing, or <sys/types.h>, <sys/param.h>
    or that and <sys/socket.h>, or what a source that includes h includes
    before it."""
    out = [[], ["h/param.h"], ["h/param.h", "h/socket.h"]] if kernel else \
        [[], ["sys/types.h"], ["sys/param.h"], ["sys/param.h", "sys/socket.h"]]
    path = os.path.realpath(os.path.join(SYS if kernel else INC, h))
    if kernel:
        srcs = []
        for d in ("sys", "netinet", "net", "i286", "i286at", "i286if"):
            srcs += glob.glob(os.path.join(SYS, d, "*.c"))
    else:
        r = subprocess.run(["grep", "-rl", "--include=*.c", "include.*" +
                            os.path.basename(h), os.path.join(TOP, "usr",
                                                              "src")],
                           capture_output=True, text=True)
        srcs = [f for f in r.stdout.split() if "/sys/" not in f]
    found = 0
    for f in sorted(srcs):
        pre = []
        for l in open(f, errors="replace"):
            m = re.match(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', l)
            if not m:
                continue
            if m.group(1) == "<":
                p = os.path.join(INC, m.group(2))
            else:
                p = os.path.join(os.path.dirname(f), m.group(2))
                if kernel and not os.path.exists(p):
                    p = os.path.join(SYS, "h", m.group(2))
            p = os.path.realpath(p)
            if p == path:
                if pre and pre not in out:
                    out.append(pre)
                    found += 1
                break
            if os.path.exists(p):
                pre.append(p)
        if found >= 6:
            break
    return out


def records(ifile, keep):
    """What the files keep() takes define, from clang's tree of ifile:
    (name, file, members), a member being (path, whether it is a bit
    field)."""
    r = subprocess.run(
        ["clang", "-x", "c", "-std=gnu89", "-w", "-fsyntax-only",
         "-Xclang", "-ast-dump=json", "-D__far=", "-D__near=", "-D__ss=",
         "-D__based(x)=", "-D__inline=static", "-Drestrict=", "-D__huge=",
         "-D__interrupt=", ifile], capture_output=True, text=True)
    try:
        tree = json.loads(r.stdout)
    except ValueError:
        return None
    where = [None]
    tops, recs = [], []

    def walk(n, top):
        loc = n.get("loc", {})
        f = loc.get("presumedFile") or loc.get("file")
        if "expansionLoc" in loc:
            f = loc["expansionLoc"].get("presumedFile") or f
        if f:
            where[0] = f
        n["_file"] = where[0]
        if top:
            tops.append(n)
        if n.get("kind") == "RecordDecl":
            recs.append(n)      # a structure inside another's is one too
        for c in n.get("inner", []) or []:
            walk(c, False)
    for n in tree.get("inner", []):
        walk(n, True)

    def mine(n):
        f = n.get("_file") or ""
        if not f:
            return False
        for d in (TOP, os.path.dirname(os.path.abspath(ifile))):
            if os.path.exists(os.path.join(d, f)):
                return keep(os.path.realpath(os.path.join(d, f)))
        return keep(os.path.realpath(f))

    def members(rec, prefix):
        out, last = [], None
        for c in rec.get("inner", []) or []:
            if c.get("kind") == "RecordDecl":
                last = c
                continue
            if c.get("kind") != "FieldDecl" or not c.get("name"):
                continue
            path = prefix + c["name"]
            out.append((path, bool(c.get("isBitfield"))))
            qt = c.get("type", {}).get("qualType", "")
            if last is not None and \
                    re.match(r"^(struct|union) \((unnamed|anonymous)", qt):
                out += members(last, path + ".")
        return out

    named = {}
    for n in tops:
        if n.get("kind") == "TypedefDecl" and mine(n):
            for c in n.get("inner", []) or []:
                d = c.get("ownedTagDecl") or {}
                for cc in c.get("inner", []) or []:
                    d = d or cc.get("decl") or {}
                if d.get("id"):
                    named.setdefault(d["id"], n["name"])
    out, seen = [], set()
    for n in recs:
        if not n.get("completeDefinition") or not mine(n):
            continue
        if n.get("name"):
            name = "%s %s" % (n["tagUsed"], n["name"])
        elif n.get("id") in named:
            name = named[n["id"]]
        else:
            continue
        if name not in seen:
            seen.add(name)
            out.append((name, n["_file"], members(n, "")))
    return out


def items_of(recs):
    """What each compiler is to compile of recs: the lines, and what
    each label is (name, file, member)."""
    items, what = [], {}
    for t, (name, f, mems) in enumerate(recs):
        what["L%d" % len(items)] = (name, f, "sizeof")
        items.append("int L%d = sizeof(%s);" % (len(items), name))
        if any(b for _, b in mems):
            items.append("%s BV%d;" % (name, t))
        for path, b in mems:
            k = "L%d" % len(items)
            if b:
                what[k] = (name, f, path + " (bits)")
                items.append("%s() { BV%d.%s = ~0; }" % (k, t, path))
            else:
                what[k] = (name, f, path)
                items.append("int %s = (int)&((%s *)0)->%s;" % (k, name,
                                                                path))
    return items, what


def both(base, head, items, model, kernel):
    """Each compiler's values of items, leaving out any a compiler
    refuses: (values, left out), or None."""
    left = []
    for _ in range(40):
        lines = [l for l in items if l not in left]
        with open(base + ".c", "w") as f:
            f.write(head + "".join(l + "\n" for l in lines))
        first = head.count("\n") + 1
        name = re.escape(os.path.basename(base) + ".c")
        bad = set()
        n = ncc(base + ".c", model, kernel, base + ".s")
        if n.returncode:
            for e in n.stderr.splitlines():
                m = re.match(r"^%s:(\d+): (?!warning)" % name, e)
                if m and 0 <= int(m.group(1)) - first < len(lines):
                    bad.add(lines[int(m.group(1)) - first])
        x = xcc(base + ".c", model, kernel, base + ".o")
        if x.returncode:
            for e in (x.stdout + x.stderr).splitlines():
                m = re.match(r"^%s\((\d+)\): Error!" % name, e)
                if m and 0 <= int(m.group(1)) - first < len(lines):
                    bad.add(lines[int(m.group(1)) - first])
        if not n.returncode and not x.returncode:
            nv, xv = native_values(base + ".s"), cross_values(base + ".o")
            return {k: (nv.get(k), xv.get(k)) for k in nv.keys() | xv.keys()
                    if k.startswith("L")}, left
        if not bad:
            return None
        left += sorted(bad)
    return None


def headers(outdir, only):
    os.makedirs(outdir, exist_ok=True)
    jobs = []
    for d in USER_DIRS:
        for p in sorted(glob.glob(os.path.join(INC, d, "*.h"))):
            if os.path.basename(p) not in SKIP:
                jobs.append((os.path.join(d, os.path.basename(p)), False))
    for d in KERN_DIRS:
        for p in sorted(glob.glob(os.path.join(SYS, d, "*.h"))):
            jobs.append((d + "/" + os.path.basename(p), True))
    results, notes = [], []
    for h, kernel in jobs:
        if only and h not in only:
            continue
        mode = "kernel" if kernel else "user"
        base = os.path.join(outdir, mode[0] + "_" + h.replace("/", "_")[:-2])
        head = None
        for pre in preambles(h, kernel):
            text = include(pre, h, kernel)
            with open(base + ".c", "w") as f:
                f.write(text + "int L0 = 1;\n")
            if ncc(base + ".c", "m", kernel, base + ".s").returncode == 0 \
                    and xcc(base + ".c", "m", kernel,
                            base + ".o").returncode == 0:
                head = text
                break
        if head is None:
            notes.append("%s %s: not compiled by both" % (mode, h))
            continue
        with open(base + ".c", "w") as f:
            f.write(head)
        ncc(base + ".c", "m", kernel, base + ".i", pre=True)
        hpath = os.path.realpath(os.path.join(SYS if kernel else INC, h))
        recs = records(base + ".i", lambda f: f == hpath)
        if recs is None:
            notes.append("%s %s: clang could not read it" % (mode, h))
            continue
        items, what = items_of(recs)
        for model in (["m"] if kernel else ["s", "m", "l"]):
            r = both(base + "-" + model, head, items, model, kernel)
            if r is None:
                notes.append("%s %s, model %s: not compiled" % (mode, h,
                                                                 model))
                continue
            vals, left = r
            for l in left:
                notes.append("%s %s, model %s: left out %s" % (mode, h,
                                                                model, l))
            for k, (nv, xv) in vals.items():
                if k in what:
                    results.append({"mode": mode, "header": h,
                                    "model": model, "type": what[k][0],
                                    "what": what[k][2], "native": nv,
                                    "cross": xv})
    report(outdir, results, notes)


def report(outdir, results, notes):
    json.dump(results, open(os.path.join(outdir, "results.json"), "w"),
              indent=0)
    types = {(r["header"], r["type"]) for r in results}
    diff = [r for r in results if r["native"] != r["cross"]]
    for r in diff:
        print("%-9s %-22s %s %-24s %-28s system's %s, cross %s" % (
            r["mode"], r["header"], r["model"], r["type"], r["what"],
            r["native"], r["cross"]))
    for n in notes:
        print(n)
    print("layouts: %d structures and unions, %d values, %d differ" % (
        len(types), len(results), len(diff)))


# What the programs are made of, by the Makefile: name, model, sources
# and flags, a line each.
PRINT = """printall:
\t@$(foreach p,$(PROGS),echo '$(p)|$(call model,$(p))|$(call srcs,$(p))|' \\
\t    $(foreach a,$($(p)_FLAGS),"'$(a)'");)
"""


def programs(outdir, only):
    """What the programs' own sources define, each once."""
    os.makedirs(outdir, exist_ok=True)
    mk = os.path.join(outdir, "print.mk")
    with open(mk, "w") as f:
        f.write(PRINT)
    out = subprocess.run(["make", "-s", "-f", "Makefile", "-f", mk,
                          "printall"],
                         cwd=os.path.join(TOP, "usr", "src", "i286"),
                         capture_output=True, text=True, check=True).stdout
    system = [os.path.realpath(d) for d in
              (INC, os.path.join(SYS, "h"), os.path.join(SYS, "i286"),
               os.path.join(SYS, "net"), os.path.join(SYS, "netinet"))]

    def theirs(f):
        return not any(f.startswith(d + "/") for d in system)
    jobs, seen = [], set()
    for line in out.splitlines():
        prog, model, srcs, fl = line.split("|", 3)
        if only and prog not in only:
            continue
        fl = [a for a in shlex.split(fl) if a[:2] in ("-D", "-U", "-I")]
        for k, src in enumerate(srcs.split()):
            # hack's are usr/src/games/hack's, encrypted
            src = re.sub(r"/obj/games/n?hack/", "/usr/src/games/hack/", src)
            if src.endswith(".c") and src not in seen:
                seen.add(src)
                jobs.append((prog, model, fl, src,
                             os.path.join(outdir, "%s-%d" % (prog, k))))

    def read(job):
        prog, model, fl, src, base = job
        with open(base + ".c", "w") as f:
            f.write('#include "%s"\n' % src)
        ncc(base + ".c", model, fl, base + ".i", pre=True)
        return records(base + ".i", theirs)

    def compare(job):
        prog, model, fl, src, base, recs = job
        items, what = items_of(recs)
        return what, both(base, '#include "%s"\n' % src, items, model, fl)
    results, notes, done, todo = [], [], set(), []
    with concurrent.futures.ThreadPoolExecutor(8) as pool:
        for job, recs in zip(jobs, pool.map(read, jobs)):
            if recs is None:
                notes.append("%s %s: clang could not read it" % job[0:4:3])
                continue
            recs = [r for r in recs if (r[0], r[1]) not in done]
            done.update((r[0], r[1]) for r in recs)
            if recs:
                todo.append(job + (recs,))
        for job, (what, r) in zip(todo, pool.map(compare, todo)):
            prog, model, src = job[0], job[1], job[3]
            if r is None:
                notes.append("%s %s: not compiled by both" % (prog, src))
                continue
            vals, left = r
            for l in left:
                notes.append("%s %s: left out %s" % (prog, src, l))
            for k, (nv, xv) in vals.items():
                if k in what:
                    results.append({"mode": prog, "header": what[k][1],
                                    "model": model, "type": what[k][0],
                                    "what": what[k][2], "native": nv,
                                    "cross": xv})
    report(outdir, results, notes)


# The host tools' structures: what writes it, the headers, the model of
# what reads it, the type, its size, and each member with the struct
# format the tool gives it, in order.
TOOLS = [
    ("mkfs.py, fsck.py: an inode", ["sys/param.h", "sys/inode.h"], "s",
     "struct dinode", 64,
     [("di_mode", "H"), ("di_nlink", "H"), ("di_uid", "H"), ("di_gid", "H"),
      ("di_size", "l"), ("di_addr", "7L"), ("di_reserved", "5H"),
      ("di_flags", "H"), ("di_atime", "l"), ("di_mtime", "l"),
      ("di_ctime", "l")]),
    ("mkfs.py, fsck.py, tests/system: the super block",
     ["sys/param.h", "sys/fs.h"], "s", "struct fs", None,
     [("fs_isize", "H"), ("fs_fsize", "l"), ("fs_nfree", "h"),
      ("fs_free", "50l"), ("fs_ninode", "h"), ("fs_inode", "100H"),
      ("fs_flock", "b"), ("fs_fmod", "b"), ("fs_ilock", "b"),
      ("fs_ronly", "b"), ("fs_time", "l"), ("fs_tfree", "l"),
      ("fs_tinode", "H"), ("fs_step", "h"), ("fs_cyl", "h"),
      ("fs_fsmnt", "12s"), ("fs_lasti", "H"), ("fs_nbehind", "H"),
      ("fs_flags", "H")]),
    ("mkfs.py, fsck.py: a block of the free list",
     ["sys/param.h", "sys/fs.h"], "s", "struct fblk", 202,
     [("df_nfree", "h"), ("df_free", "50l")]),
    ("mkfs.py, fsck.py: a directory entry", ["sys/param.h", "sys/dir.h"],
     "s", "struct direct", 70,
     [("d_ino", "H"), ("d_reclen", "H"), ("d_namlen", "H"),
      ("d_name", "64s")]),
    ("mkdisk.py, fsck.py: the disk label",
     ["sys/param.h", "sys/disklabel.h"], "s", "struct disklabel", None,
     [("d_magic", "L"), ("d_type", "B"), ("d_subtype", "B"),
      ("d_typename", "16s"), ("d_packname", "16s"), ("d_secsize", "H"),
      ("d_nsectors", "H"), ("d_ntracks", "H"), ("d_ncylinders", "H"),
      ("d_secpercyl", "H"), ("d_secperunit", "L"),
      ("d_sparespertrack", "H"), ("d_sparespercyl", "H"),
      ("d_acylinders", "H"), ("d_rpm", "H"), ("d_interleave", "B"),
      ("d_trackskew", "B"), ("d_cylskew", "B"), ("d_headswitch", "B"),
      ("d_trkseek", "H"), ("d_flags", "H"), ("d_drivedata", "5L"),
      ("d_spare", "5L"), ("d_magic2", "L"), ("d_checksum", "H"),
      ("d_npartitions", "H"), ("d_bbsize", "H"), ("d_sbsize", "H"),
      ("d_partitions", "")]),
    ("mkdisk.py, fsck.py: a partition of the label",
     ["sys/param.h", "sys/disklabel.h"], "s", "struct partition", 12,
     [("p_size", "L"), ("p_offset", "L"), ("p_fsize", "H"),
      ("p_fstype", "B"), ("p_frag", "B")]),
    ("strip.py, ar.py, ne2aout.py, kpatch.py, kprof.py, mkfd.py: the "
     "header", ["a.out.h"], "s", "struct exec", 24,
     [("a_magic", "H"), ("a_model", "H"), ("a_nseg", "H"), ("a_eseg", "H"),
      ("a_entry", "H"), ("a_dseg", "H"), ("a_stack", "H"),
      ("a_unused", "H"), ("a_syms", "L"), ("a_strs", "L")]),
    ("the same: a segment", ["a.out.h"], "s", "struct exseg", 12,
     [("xs_type", "H"), ("xs_nrel", "H"), ("xs_filesz", "L"),
      ("xs_memsz", "L")]),
    ("ar.py, ne2aout.py, kpatch.py, kprof.py, netcheck.py: a symbol",
     ["a.out.h"], "s", "struct nlist", 8,
     [("n_un", "L"), ("n_type", "B"), ("n_ovly", "b"), ("n_value", "H")]),
    ("the same, as a large-model program reads it", ["a.out.h"], "l",
     "struct nlist", 8,
     [("n_un", "L"), ("n_type", "B"), ("n_ovly", "b"), ("n_value", "H")]),
    ("ar.py, netcheck.py: relocation", ["a.out.h"], "s", "struct reloc", 6,
     [("r_addr", "H"), ("r_sym", "H"), ("r_type", "H")]),
    ("ar.py: ranlib's table", ["sys/types.h", "ranlib.h"], "s",
     "struct ranlib", 8, [("ran_un", "L"), ("ran_off", "L")]),
    ("the same, as a large-model program reads it",
     ["sys/types.h", "ranlib.h"], "l", "struct ranlib", 8,
     [("ran_un", "L"), ("ran_off", "L")]),
    ("ar.py: a member's header", ["ar.h"], "s", "struct ar_hdr", 60,
     [("ar_name", "16s"), ("ar_date", "12s"), ("ar_uid", "6s"),
      ("ar_gid", "6s"), ("ar_mode", "8s"), ("ar_size", "10s"),
      ("ar_fmag", "2s")]),
    ("mkerrlst.py: syserrlst's header", ["sys/types.h", "errlst.h"], "s",
     "struct ERRLSTHDR", 16,
     [("magic", "h"), ("maxmsgnum", "h"), ("maxmsglen", "h"),
      ("pad", "5h")]),
    ("mkerrlst.py: a message's place", ["sys/types.h", "errlst.h"], "s",
     "struct ERRLST", 6, [("offmsg", "l"), ("lenmsg", "h")]),
    ("strfile.py: fortune's table",
     [TOP + "/usr/src/games/fortune/strfile.h"], "s", "STRFILE", 20,
     [("str_numstr", "H"), ("str_longlen", "H"), ("str_shortlen", "H"),
      ("str_delims", "3l"), ("str_flags", "h")]),
    ("initdeck.py: monop's deck", [TOP + "/usr/src/games/monop/deck.h"],
     "s", "DECK", 8,
     [("num_cards", "h"), ("last_card", "h"), ("gojf_used", "bx"),
      ("offsets", "H")]),
]


def setfiles():
    """setfiles.py's structures, laid out by its own rule (pack())."""
    sys.path.insert(0, TOOLS_DIR)
    import setfiles as sf
    out = []
    for name, layout in (("struct stats", sf.STATS),
                         ("struct nrgvoid", sf.NRGVOID)):
        mems, at = [], 0
        for m, k in layout:
            if not k.startswith("s") and k != "b" and at % 2:
                at += 1
            mems.append((m, at))
            at += int(k[1:]) if k.startswith("s") else struct.calcsize("<" + k)
        out.append(("setfiles.py: phantasia's " + name,
                    ["stdio.h", TOP + "/usr/src/games/phantasia/phant.h"],
                    "m", name, len(sf.pack(layout, {})), mems))
    return out


def tools():
    cross = shutil.which("wdis") is not None
    checks = []
    for what, incs, model, expr, size, fields in TOOLS:
        mems, fmt = [], "<"
        for m, f in fields:
            mems.append((m, struct.calcsize(fmt)))
            fmt += f
        checks.append((what, incs, model, expr,
                       size if size is not None else None, mems))
    checks += setfiles()
    bad = 0
    tmp = tempfile.mkdtemp(prefix="layouts.")
    try:
        for what, incs, model, expr, size, mems in checks:
            src = os.path.join(tmp, "t.c")
            with open(src, "w") as f:
                f.write(include(incs[:-1], incs[-1], False))
                f.write("int L0 = sizeof(%s);\n" % expr)
                for i, (m, _) in enumerate(mems):
                    f.write("int L%d = (int)&((%s *)0)->%s;\n" % (i + 1, expr,
                                                                   m))
            n = ncc(src, model, False, os.path.join(tmp, "t.s"))
            if n.returncode:
                print("layouts: %s: %s" % (what, n.stderr.strip()))
                bad += 1
                continue
            nv = native_values(os.path.join(tmp, "t.s"))
            wrong = []
            if size is not None and nv["L0"] != size:
                wrong.append("%d bytes where the tool has %d" % (nv["L0"],
                                                                  size))
            for i, (m, at) in enumerate(mems):
                if nv.get("L%d" % (i + 1)) != at:
                    wrong.append("%s at %s where the tool has %d" % (
                        m, nv.get("L%d" % (i + 1)), at))
            agree = ""
            if cross and xcc(src, model, False,
                             os.path.join(tmp, "t.o")).returncode == 0:
                xv = cross_values(os.path.join(tmp, "t.o"))
                if xv == nv:
                    agree = ", as the cross compiler has it"
                else:
                    wrong.append("the cross compiler has it otherwise")
            print("layouts: %s: %s, %d bytes in the %s model%s%s" % (
                what, expr, nv["L0"], {"s": "small", "m": "medium",
                                       "l": "large"}[model],
                agree, ": " + "; ".join(wrong) if wrong else ""))
            bad += bool(wrong)
    finally:
        shutil.rmtree(tmp)
    sys.exit(1 if bad else 0)


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "headers":
        headers(sys.argv[2], sys.argv[3:])
    elif len(sys.argv) >= 3 and sys.argv[1] == "programs":
        programs(sys.argv[2], sys.argv[3:])
    elif sys.argv[1:] == ["tools"]:
        tools()
    else:
        sys.exit(__doc__.split("\n\n")[1])


if __name__ == "__main__":
    main()
