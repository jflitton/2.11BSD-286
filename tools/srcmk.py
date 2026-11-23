#!/usr/bin/env python3
"""Write what the system needs to make itself: its /usr/src.

usage: srcmk.py OUT

The system is built here by GNU make, from Makefiles that say what each
part is made of (usr/src/sys/PC, usr/src/sys/i286stand).  The system
has a make of its own, which reads none of that: no pattern rules, no
functions, no conditionals.  This writes for it, from the same tables,
Makefiles that it does read, with a rule for every file, and a proto
for mkfs.py of the file system that is to be /usr/src there: the
sources, in the directories they have here, and each Makefile where the
one it was written from is.

    OUT/sys/PC/Makefile         the kernel: make, in /usr/src/sys/PC
    OUT/sys/i286stand/Makefile  /boot and the boot blocks
    OUT/lib/libc/i286/Makefile  the C library, which is made for each
                                model in s, m and l there, each with a
                                Makefile of its own
    OUT/i286/Makefile           the other libraries, and the programs
    OUT/proto                   the file system, to be mounted on
                                /usr/src
    OUT/programs, OUT/libraries the names of what the last makes
    OUT/installed               each program it installs, and where
    OUT/data                    each file that is not a program that it
                                makes (make data), after the name of
                                what it is of (make data-NAME)
    OUT/datainstalled           each of those it installs, and where;
                                under "progs", what the programs'
                                install does

What make makes there is left where it is run, as upstream's Makefiles
leave it: unix in /usr/src/sys/PC, as in a directory of /sys made by
config.  Each is to be, to the byte, what is made of the same sources
here (tests/self).

Each Makefile has "install", which puts what was made where the system
has it: the kernel as /unix, the one that was there as /unix.old, which
/boot loads if it is named to it; the libraries where cc looks for
them; the programs where the root file system has them
(usr/src/i286/root.proto), with its modes and owners.
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mkman

TOP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(TOP, "usr", "src")

HEAD = """\
# %s
#
# This is for the system's own make, and was written where the system
# is built (tools/srcmk.py) from the Makefile of the same directory
# there, which is GNU make's: what is to be changed is changed there.
"""


def lines(directory, *args):
    """What `make table` says in a directory of usr/src."""
    r = subprocess.run(["make", "-s", "-C", os.path.join(SRC, directory),
                        "TOOLS=native", "table"] + list(args),
                       capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("srcmk: make table in %s:\n%s" % (directory, r.stderr))
    return r.stdout.splitlines()


def table(directory, *args):
    """The same, where each line is a name and its words."""
    t = {}
    for line in lines(directory, *args):
        words = line.split()
        if words:
            t[words[0]] = words[1:]
    return t


def fold(name, words, width=72):
    """NAME= words, on as many lines as they take."""
    lines = []
    line = name + "=\t"
    for w in words:
        if len(line.expandtabs()) + len(w) + 2 > width and \
                not line.endswith("\t"):
            lines.append(line.rstrip() + " \\")
            line = "\t"
        line += w + " "
    lines.append(line.rstrip())
    return "\n".join(lines) + "\n"


def stamp(hdrs):
    """The macros and the rule of "hdrs", a file that is newer than
    every header: what is to be made again when any of them changes
    depends on it.  The names are on several lines, each of a length
    make has room for."""
    macros = rule = ""
    for i in range(0, len(hdrs), 40):
        n = "HDRS%d" % (i // 40 + 1)
        macros += fold(n, hdrs[i:i + 40])
        rule += "hdrs:\t$(%s)\n" % n
    return macros, rule + "\ttouch hdrs\n"


def archive(lib, objs, per=100):
    """The commands that make the library of the objects, in their
    order: make gives a command 254 arguments at most, and the kernel
    5120 characters of them."""
    cmds = "\trm -f %s\n" % lib
    for i in range(0, len(objs), per):
        cmds += fold("\tar %s %s" % ("r" if i else "cr", lib),
                     objs[i:i + per]).replace("=\t", " ", 1).replace(
                         "\n\t", "\n\t    ")
    return cmds + "\tranlib %s\n" % lib


def groups(name, words, per=40):
    """The macros NAME1, NAME2 and so on, of PER words each, and their
    names: a line that make reads, with what its macros stand for put
    in, has room for some 7000 characters."""
    text, names = "", []
    for i in range(0, len(words), per):
        names.append("%s%d" % (name, i // per + 1))
        text += fold(names[-1], words[i:i + per])
    return text, names


def placed(path, where, own_mode):
    """The command that puts PATH at WHERE, as install-progs puts a
    program: a copy beside it is made what it is to be and then takes
    its name."""
    mode, _, own = own_mode.partition(":")
    new = where + ".new"
    cmd = "cp %s '%s'" % (path, new)
    if own:
        uid, _, gid = own.rpartition(".")
        if uid:
            cmd += " && chown %s '%s'" % (uid, new)
        cmd += " && chgrp %s '%s'" % (gid, new)
    return "\t%s && chmod %s '%s' && \\\n\t    mv -f '%s' '%s'\n" % (
        cmd, mode, new, new, where)


def kernel():
    t = table("sys/PC")
    dirs = t["VPATH"]
    net = set(t["NET"])
    rules = []
    for o in t["OBJS"]:
        base = o[:-2]
        if base == "vers":
            continue
        found = [(d, e) for d in dirs for e in (".c", ".s")
                 if os.path.exists(os.path.join(SRC, d, base + e))]
        if len(found) != 1:
            sys.exit("srcmk: %s: %d sources" % (o, len(found)))
        d, e = found[0]
        s = "$S/%s/%s%s" % (d, base, e)
        if e == ".c":
            rules.append("%s: %s hdrs\n\tcc -c $(CFLAGS) %s%s\n" %
                         (o, s, "$(NFLAGS) " if o in net else "", s))
        else:
            rules.append("%s: %s $S/sys/i286/mch_defs.inc\n"
                         "\t$(ASM) -o %s %s\n" % (o, s, o, s))
    macros, hdrs = stamp(["$S/" + h for h in t["HDRS"]])
    return (HEAD % "The kernel for a PC/AT-class 286 machine: make, and unix "
            "is made here." + """\
#
#	make		the kernel, unix, and what the linker says of
#			it, unix.map
#	make install	and it is /unix, and the one that was is
#			/unix.old, to be named to /boot if this one
#			does not start
#	make clean	nothing made is left
#
# The network's files have a data segment of their own (cc -nd=NET,
# docs/network.md).  Every object is made again when any header
# changes: crude, and never wrong.  The kernel says when it was made,
# or what DATE is in the environment.  -I$S/sys is for the headers that
# are named from there and are not in /usr/include: pdpif/if_uba.h.

S=	/usr/src
""" + fold("OPTS", t["OPTS"]) + """\
CFLAGS=	-O -mm -DKERNEL $(OPTS) -I$S/sys/PC -I$S/sys/h -I/usr/include -I$S/sys
NFLAGS=	-DSUPERVISOR -nd=NET
ASM=	as -mm -DFAR -I$S/sys/i286/ -I$S/lib/libc/i286/crt/
""" + macros +
            "# mch_start.o must come first: the kernel starts at its "
            "beginning.\n" + fold("OBJS", t["OBJS"]) + """
all:	unix

# The kernel's selectors are the GDT's from slot 16 (docs/boot.md), and
# its stacks are in its data (uslot, param.c).
unix:	$(OBJS)
	ld -mm -gdt 16 -stack 0 -M -o unix $(OBJS) > unix.map

install: unix
	cp /unix /unix.old
	cp unix /unix

clean:
	rm -f $(OBJS) unix unix.map vers.c hdrs

""" + hdrs + """
vers.c:	FRC
	@(set `date`; echo 'char version[] = "2.11 BSD UNIX for the 286 ('"$${DATE-$$2 $$3 $$6}"')";') > vers.c

vers.o:	vers.c
	cc -c -O -mm vers.c

FRC:

""" + "\n".join(rules))


def stand():
    text = open(os.path.join(SRC, "sys/i286stand/Makefile")).read()
    bootmax = int(re.search(r"^BOOTMAX=(\S+)", text, re.M).group(1), 0)
    t = table("sys/i286stand")
    macros, hdrs = stamp(["$S/" + h for h in t["HDRS"]])
    return (HEAD % "/boot and the boot blocks: make, and they are made "
            "here." + """\
#
#	make		boot, which is /boot; hdboot.bin, the boot block
#			of a hard disk, which is /mdec/wduboot;
#			mbr.bin, the first sector of a disk that has a
#			partition table; flboot.bin, the boot block
#			of a diskette, which is /mdec/fluboot;
#			fdboot.bin, the first sector of a diskette
#			with no file system
#	make install	/boot, /mdec/wduboot and /mdec/fluboot are those
#	make clean	nothing made is left
#
# as -b writes the bytes alone, and ld -b /boot's code and data as one
# segment and nothing else.

S=	/usr/src
CRT=	$S/lib/libc/i286/crt
""" + macros + """
all:	fdboot.bin mbr.bin hdboot.bin flboot.bin boot

fdboot.bin: fdboot.s
	as -b -o fdboot.bin fdboot.s

mbr.bin: mbr.s
	as -b -o mbr.bin mbr.s

hdboot.bin: hdboot.s
	as -b -o hdboot.bin hdboot.s

flboot.bin: flboot.s
	as -b -o flboot.bin flboot.s

srt0.o:	srt0.s
	as -ms -I$(CRT)/ -o srt0.o srt0.s

i4m.o:	$(CRT)/i4m.s
	as -ms -I$(CRT)/ -o i4m.o $(CRT)/i4m.s

i4d.o:	$(CRT)/i4d.s
	as -ms -I$(CRT)/ -o i4d.o $(CRT)/i4d.s

boot.o:	boot.c hdrs
	cc -c -O -ms boot.c

""" + hdrs + """
# srt0.o first: the program starts at its first byte.  What /boot may
# be with the memory its file does not hold is %d bytes: it is loaded
# at 4K and its stack is at the top of the first 64K (srt0.s).
boot:	srt0.o boot.o i4m.o i4d.o
	ld -b -M -o boot srt0.o boot.o i4m.o i4d.o > boot.map
	@awk '/^image: / && $$2 > %d { print "boot: " $$2 " bytes in memory; the limit is %d"; exit 1 }' boot.map || { rm -f boot; exit 1; }

install: boot hdboot.bin flboot.bin
	cp boot /boot
	cp hdboot.bin /mdec/wduboot
	cp flboot.bin /mdec/fluboot

clean:
	rm -f *.o *.bin boot boot.map hdrs
""") % (bootmax, bootmax, bootmax)


LIBC = "lib/libc"
MD = LIBC + "/i286"
# Where an object of the C library is looked for, in the order of the
# rules of usr/src/lib/libc/i286/Makefile, and what the compiler is
# told for one found there.
LIBCDIRS = [(LIBC + "/gen", ".c", ""), (MD + "/gen", ".c", ""),
            (LIBC + "/pdp/gen", ".c", ""),
            (LIBC + "/stdio", ".c", " -I$S/" + LIBC + "/stdio"),
            (LIBC + "/stdlib", ".c", ""), (LIBC + "/string", ".c", ""),
            (LIBC + "/compat-4.1", ".c", ""), (LIBC + "/compat-43", ".c", ""),
            (LIBC + "/compat-sys5", ".c", ""), (LIBC + "/inet", ".c", ""),
            (LIBC + "/net", ".c", ""), (LIBC + "/net/named", ".c", ""),
            (LIBC + "/ns", ".c", ""), (MD + "/sys", ".c", ""),
            (MD + "/sys", ".s", ""), (MD + "/gen", ".s", ""),
            (MD + "/net", ".s", ""), (MD + "/crt", ".s", "")]
# Where a model's library and its start-up code are on the system.
LIBDIR = {"s": "/lib", "m": "/lib/m", "l": "/lib/l"}
USRLIB = {"s": "/usr/lib", "m": "/usr/lib/m", "l": "/usr/lib/l"}
FAR = {"s": "", "m": " -DFAR", "l": " -DFAR -DFARDATA"}


def libc(model):
    """The Makefile of one model's C library, and the directories its
    sources are in."""
    t = table(MD, "MODEL=" + model)
    rules = []
    used = set()
    for base in t["OBJS"]:
        found = None
        if model == "l" and os.path.exists(
                os.path.join(SRC, MD, "gen", "l" + base + ".c")):
            found = (MD + "/gen", "l" + base, ".c", "")
        for d, e, flags in LIBCDIRS:
            if not found and os.path.exists(os.path.join(SRC, d, base + e)):
                found = (d, base, e, flags)
        if not found:
            sys.exit("srcmk: libc: no source for %s" % base)
        d, name, e, flags = found
        used.add(d)
        s = "$S/%s/%s%s" % (d, name, e)
        if e == ".c":
            # cc names the object for the source.
            rules.append("%s.o: %s hdrs\n\tcc -c $(CFLAGS)%s %s\n%s" % (
                base, s, flags, s, "" if name == base else
                "\tmv %s.o %s.o\n" % (name, base)))
        else:
            rules.append("%s.o: %s $(CRT)/DEFS.inc\n\t$(ASM) -o %s.o %s\n"
                         % (base, s, base, s))
    stubs = t["STUBS"]
    hdrs = sorted("/usr/include/" + f for f in os.listdir(
        os.path.join(TOP, "usr/include")) if f.endswith(".h")) + \
        sorted("$S/%s/%s" % (h, f) for h in ("sys/h", "sys/i286",
                                             LIBC + "/stdio")
               for f in os.listdir(os.path.join(SRC, h)) if f.endswith(".h"))
    macros, hdrs = stamp(hdrs)
    text = (HEAD % ("The C library of the %s model: make, and libc.a and "
                    "crt0.o are made here." % {
                        "s": "small", "m": "medium", "l": "large"}[model]) +
            """\
#
#	make		the library, libc.a, and the start-up code,
#			crt0.o
#	make install	and they are %s/libc.a and %s/crt0.o, which
#			cc -m%s links a program with
#	make clean	nothing made is left
#
# The system calls' stubs are written in sys, a file each, from
# i286/sys/syscalls.tab, by i286/sys/mkstubs.c, which is compiled
# there, and each is checked against the kernel's table of calls.
# Every object is made again when any header changes.

S=	/usr/src
CRT=	$S/lib/libc/i286/crt
SYS=	$S/lib/libc/i286/sys
CFLAGS=	-O -m%s -I/usr/include
ASM=	as -m%s -I$(CRT)%s
""" % (LIBDIR[model], LIBDIR[model], model, model, model, FAR[model]) +
            macros +
            fold("OBJS", [b + ".o" for b in t["OBJS"]]) + """
all:	crt0.o libc.a

libc.a:	$(OBJS) sys/made
""" + archive("libc.a", [b + ".o" for b in t["OBJS"]] +
              ["sys/" + f + ".o" for f in stubs]) + """\

crt0.o:	$S/lib/libc/i286/csu/crt0.s $(CRT)/DEFS.inc
	$(ASM) -o crt0.o $S/lib/libc/i286/csu/crt0.s

sys/mkstubs: $(SYS)/mkstubs.c
	cd sys && cc -O -o mkstubs $(SYS)/mkstubs.c && rm -f mkstubs.o

sys/made: sys/mkstubs $(SYS)/syscalls.tab /usr/include/syscall.h \\
		$S/sys/sys/init_sysent.c
	rm -f sys/*.s sys/*.o
	sys/mkstubs -m %s -k $S/sys/sys/init_sysent.c -d sys \\
	    $(SYS)/syscalls.tab /usr/include/syscall.h
	cd sys; for f in *.s; do as -m%s -o `basename $$f .s`.o $$f || exit 1; done
	touch sys/made

install: all
	cp crt0.o %s/crt0.o
	cp libc.a %s/libc.a
	ranlib %s/libc.a

clean:
	rm -f *.o sys/*.o sys/*.s sys/mkstubs sys/made libc.a hdrs

""" % (model, model, LIBDIR[model], LIBDIR[model], LIBDIR[model]) + hdrs +
            "\n" + "\n".join(rules))
    return text, used


def programs():
    """The Makefile of the programs and of the libraries that are not
    the C library; the directories of usr/src they are made from, and
    those of /usr/src/i286 that what yacc and lex make is put in; the
    names of the libraries, of the programs it makes, and of those it
    does not."""
    progs, libs = [], []
    gen = {}            # what yacc and lex make: (which, of what, header)
    sysgen = []         # what the rules written for the system make
    mkstr = []          # (strings, options, sources, what is made first)
    filtered = []       # (directory, program, its source and flags,
                        #  cc -E's flags, sources)
    sysrules = []
    sysmacros = []      # (name, words)
    single = []         # (target, what it depends on, the command)
    data = []           # (what it is of, files)
    site = []           # data that is made and never installed
    progdata = []       # what a program reads that is made with it
    placing = []        # (where, mode and owner, file)
    linking = []        # (name, the file it is another name of)
    given = []          # directories of usr/src to give besides
    inrules = False
    for line in lines("i286"):
        if inrules:
            if line == "END":
                inrules = False
            else:
                sysrules[-1] += line + "\n"
            continue
        kind, _, rest = line.partition(" ")
        if kind == "RULES":
            inrules = True
            sysrules.append("")
        elif kind == "SYSGEN":
            sysgen = rest.split()
        elif kind == "FILTERED":
            d, prog, flags, srcs = rest.split("|")
            filtered.append((d.strip(), prog.split(), flags.split(),
                             srcs.split()))
        elif kind == "MKSTR":
            out, opts, srcs, deps = rest.split("|")
            mkstr.append((out.strip(), opts.strip(), srcs.split(),
                          deps.split()))
            progdata.append(out.strip())
        elif kind == "PROGDATA":
            progdata += rest.split()
        elif kind == "MACRO":
            w = rest.split()
            sysmacros.append((w[0], w[1:]))
        elif kind == "RULE":
            target, deps, cmd = rest.split("|", 2)
            single.append((target.strip(), deps.split(), cmd.strip()))
        elif kind == "DATA":
            name, files = rest.split("|")
            data.append((name.strip(), files.split()))
        elif kind == "SITE":
            site = rest.split()
        elif kind == "DIRS":
            given += rest.split()
        elif kind == "INSTALL":
            placing.append(tuple(rest.split()))
        elif kind == "LINK":
            linking.append(tuple(rest.split()))
        if kind == "PROG":
            head, flags, srcs, ls = rest.split("|")
            name, model = head.split()
            progs.append((name, model, flags.strip(), srcs.split(),
                          ls.split()))
        elif kind == "LIB":
            name, flags, srcs = rest.split("|")
            libs.append((name.strip(), flags.strip(), srcs.split()))
        elif kind in ("YACC", "LEX"):
            w = rest.split()
            gen[w[0]] = (kind, w[1], w[2] if len(w) > 2 else None)
    # What is made in obj where the system is built is made in
    # /usr/src/i286 on the system, in the same directory of it.
    obj = os.path.join(TOP, "obj") + "/"
    # mkstr's copy of a file is beside its messages, under the
    # file's own name.
    for out, opts, srcs, deps in mkstr:
        sysgen += [os.path.dirname(out) + "/" + os.path.basename(f)
                   for f in srcs]
    for d, prog, flags, srcs in filtered:
        sysgen += [d + "/" + os.path.basename(f) for f in srcs]
    gendirs = sorted(set(os.path.dirname(g) for g in list(gen) + sysgen))

    def known(flags, srcs):
        """Whether the system can make every source that is not in
        usr/src, and has every directory the flags name."""
        for f in srcs:
            if f.startswith("/") and not (
                    f.startswith(obj) and (f[len(obj):] in gen or
                                           f[len(obj):] in sysgen)):
                return False
        for f in re.findall(r"-I(/\S+)", flags):
            if not (f.startswith(obj) and f[len(obj):] in gendirs):
                return False
        return TOP not in re.sub(r"-I/\S+", "", flags)
    dirs = set(given)
    rules = []
    made, left = [], []

    def flagged(flags):
        """Flags as the system is to be given them: a directory of
        usr/src is named from /usr/src."""
        out = []
        for f in re.findall(r"'[^']*'|\S+", flags):
            if f.startswith("-I" + obj):
                f = "-I$S/i286/" + f[2 + len(obj):]
            elif f.startswith("-I") and not f.startswith("-I/"):
                dirs.add(f[2:])
                f = "-I$S/" + f[2:]
            out.append(f)
        return "".join(" " + f for f in out)

    def sources(srcs):
        bases = [os.path.basename(f) for f in srcs]
        if len(set(bases)) != len(bases):
            sys.exit("srcmk: two sources of one name among %s" % srcs)
        out = []
        for f in srcs:
            if f.startswith(obj):
                out.append("$S/i286/" + f[len(obj):])
            else:
                dirs.add(os.path.dirname(f))
                out.append("$S/" + f)
        return out

    libnames = []
    for name, flags, srcs in libs:
        for m in "sml":
            a = "lib/%s/lib%s.a" % (m, name)
            libnames.append(a)
            objs = " ".join(os.path.basename(f)[:-2] + ".o" for f in srcs)
            rules.append(
                fold("%s:\thdrs" % a, sources(srcs)).replace("=\t", " ", 1) +
                "\tcd lib/%s/%s && rm -f *.o && cc -c -O -m%s%s \\\n"
                "\t    %s\n"
                "\trm -f %s\n"
                "\tcd lib/%s/%s && ar cr ../lib%s.a \\\n\t    %s\n"
                "\tranlib %s\n" % (
                    m, name, m, flagged(flags),
                    " \\\n\t    ".join(sources(srcs)), a, m, name, name,
                    objs, a))
    for g in sorted(gen):
        which, src, hdr = gen[g]
        dirs.add(os.path.dirname(src))
        d, c = os.path.split(g)
        if which == "YACC":
            cmd = "yacc %s$S/%s && mv y.tab.c %s" % (
                "-d " if hdr else "", src, c)
            if hdr and os.path.basename(hdr) != "y.tab.h":
                cmd += " && mv y.tab.h %s" % os.path.basename(hdr)
        else:
            cmd = "lex $S/%s && mv lex.yy.c %s" % (src, c)
        rules.append("$S/i286/%s: $S/%s\n\tcd %s && %s\n" % (g, src, d, cmd))
    rules += sysrules
    # A file as the preprocessor leaves it, through a program of the
    # game's own: hack's.
    for d, prog, flags, srcs in filtered:
        name, src, pflags = prog[0], prog[1], prog[2:]
        dirs.add(os.path.dirname(src))
        rules.append("%s:\t$S/%s\n\tcd o && rm -f *.o && cc -O%s -o ../%s "
                     "$S/%s && rm -f *.o\n" % (
                         name, src, "".join(" " + f for f in pflags), name,
                         src))
        eflags = "".join(" -I$S/" + f[2:] if f.startswith("-I") else " " + f
                         for f in flags)
        own = sorted(set(
            "$S/%s/%s" % (os.path.dirname(f), h) for f in srcs
            for h in os.listdir(os.path.join(SRC, os.path.dirname(f)))
            if h.endswith(".h")))
        m = name.upper() + "HDRS"
        rules.append(fold(m, own))
        for f in srcs:
            dirs.add(os.path.dirname(f))
            c = d + "/" + os.path.basename(f)
            rules.append(
                "$S/i286/%s: $S/%s %s hdrs $(%s)\n"
                "\tcc -E%s $S/%s > %s.i\n"
                "\t./%s < %s.i > %s\n\trm -f %s.i\n" % (
                    c, f, name, m, eflags, f, c, name, c, c, c))
    if mkstr:
        dirs.add("i286")
        rules.append("mkstr:\t$S/i286/mkstr.c\n\tcd o && rm -f *.o && "
                     "cc -O -o ../mkstr $S/i286/mkstr.c && rm -f *.o\n")
    for out, opts, srcs, deps in mkstr:
        d = os.path.dirname(out)
        for f in srcs:
            dirs.add(os.path.dirname(f))
        rules.append(
            fold("$S/i286/%s:\tmkstr" % out, ["$S/" + f for f in srcs] +
                 ["$S/i286/" + f for f in deps]).replace("=\t", " ", 1) +
            "\t./mkstr%s %s %s \\\n\t    %s\n" % (
                " " + opts if opts else "", out, d,
                " \\\n\t    ".join("$S/" + f for f in srcs)) +
            fold("#", ["$S/i286/%s/%s" % (d, os.path.basename(f))
                       for f in srcs]).replace("#=\t", "", 1).rstrip("\n") +
            ": $S/i286/%s\n" % out)
    for name, model, flags, srcs, ls in progs:
        if not known(flags, srcs):
            left.append(name)
            continue
        made.append(name)
        deps = ["%s/crt0.o" % LIBDIR[model], "%s/libc.a" % LIBDIR[model]] + \
            ["%s/lib%s.a" % (USRLIB[model], l[2:]) for l in ls]
        rules.append(
            fold("bin/%s:\thdrs" % name, sources(srcs) + deps)
            .replace("=\t", " ", 1) +
            "\tcd o && rm -f *.o && cc -O -m%s%s -o ../bin/%s \\\n"
            "\t    %s%s && rm -f *.o\n" % (
                model, flagged(flags), name,
                " \\\n\t    ".join(sources(srcs)),
                "".join(" " + l for l in ls)))
    hdrs = sorted("/usr/include/" + f for f in os.listdir(
        os.path.join(TOP, "usr/include")) if f.endswith(".h")) + \
        sorted("$S/%s/%s" % (h, f) for h in ("sys/h", "sys/i286")
               for f in os.listdir(os.path.join(SRC, h)) if f.endswith(".h"))
    macros, stamprule = stamp(hdrs)

    # What is not a program.  The rules of one command each, and the
    # manual: a page is formatted where its source is, as upstream's
    # Makefiles format it, so that a file it reads (.so) is found.
    for target, deps, cmd in single:
        rules.append(fold(target + ":", deps).replace(":=\t", ":\t", 1) +
                     "\t" + cmd + "\n")
    # A page of a directory beside /usr/man, /usr/new/man, is its
    # program's Makefile's to make, upstream's, and is not in whatis.
    wanted = [w for w in mkman.pages(SRC, os.path.join(SRC, "i286",
                                                       "man.list"))
              if not w[0].startswith("../")]
    names = mkman.others(wanted)
    files = set()       # sources that are given without their directory
    pages = []
    for d, name, path in sorted(wanted):
        rel = os.path.relpath(path, SRC)
        files.add(rel)
        for line in open(path, "rb"):
            m = mkman.SO.match(line)
            if m and not m.group(1).startswith(b"/"):
                files.add(os.path.join(os.path.dirname(rel),
                                       m.group(1).decode()))
        pages.append("man/%s/%s" % (d, name))
        rules.append("%s: $S/%s\n\tcd $S/%s && nroff %s %s > "
                     "$S/i286/%s\n" % (
                         pages[-1], rel, os.path.dirname(rel),
                         " ".join(["-man"] + mkman.tabs(path)),
                         os.path.basename(rel), pages[-1]))
    mandirs = sorted(set(d for d, _, _ in wanted))
    mantext, mannames = groups("MAN", pages)
    targets = "".join("data-man: $(%s)\n" % n for n in mannames) + \
        "data-man: man/whatis\n\n"
    rules.append(
        "".join("man/whatis: $(%s)\n" % n for n in mannames) +
        "\tfor d in %s; do for f in man/$$d/*.0; do \\\n"
        "\t    sed -n -f $S/man/makewhatis.sed $$f; done; done | \\\n"
        "\t    sort -u > man/whatis\n" % " ".join(mandirs))
    files.add("man/makewhatis.sed")
    # What the rules as they are written name of usr/src: a file, or a
    # directory, which is then given whole.
    for text in sysrules + [" ".join(w) for _, w in sysmacros] + \
            [" ".join(d) + " " + c for _, d, c in single]:
        for f in re.findall(r"\$S/([\w./+-]+)", text):
            f = f.rstrip("/")
            if os.path.isdir(os.path.join(SRC, f)):
                dirs.add(f)
            elif os.path.isfile(os.path.join(SRC, f)):
                files.add(f)
    datatext, every = mantext, []
    for name, made_ in data:
        text, ns = groups(name.upper(), made_)
        datatext += text
        targets += "".join("data-%s: $(%s)\n" % (name, n) for n in ns) + "\n"
        every += made_
    data.append(("man", pages + ["man/whatis"]))
    every += data[-1][1]
    # The directories they are made in, but zic's own, and games, which
    # has more than these.
    datadirs = sorted(set(os.path.dirname(f) for f in every) - {"games"})
    datadirs = [d for d in datadirs if not d.startswith("zoneinfo/")]
    # Where each program is on the system, its mode and whose it is, as
    # the root file system has it (root.proto), and the other names it
    # has there.
    where = {}
    installed = []
    datainstalled = []  # (what it is of, file, where)
    cmds = datacmds = ""
    for line in open(os.path.join(SRC, "i286", "root.proto")):
        w = line.split()
        made_ = w[3][len("../../../obj/"):] if len(w) == 4 and \
            w[0] == "f" and w[3].startswith("../../../obj/") else None
        if made_ in progdata:
            cmds += placed(made_, w[1], w[2])
            datainstalled.append(("progs", made_, w[1]))
        elif made_ in every and made_ not in site:
            placing.append((w[1], w[2], made_))
        elif len(w) == 3 and w[0] == "h" and w[2] in [p[0] for p in placing]:
            linking.append((w[1], w[2]))
        if len(w) == 4 and w[0] == "f" and "/obj/root/bin/" in w[3] and \
                os.path.basename(w[3]) in made:
            name = os.path.basename(w[3])
            mode, _, own = w[2].partition(":")
            where[w[1]] = name
            installed.append((name, w[1]))
            # A copy beside the one that is there is made what it is
            # to be, and then takes its name: the other is whole until
            # then, and is the one still if anything fails; mv -f,
            # since it asks before it replaces one that is running.  install
            # -s takes the old one away first, and the new one too if
            # strip does not run, as it does not when it is strip that
            # is being installed.
            new = w[1] + ".new"
            cmd = "cp bin/%s %s && strip %s" % (name, new, new)
            if own:
                uid, _, gid = own.rpartition(".")
                if uid:
                    cmd += " && chown %s %s" % (uid, new)
                cmd += " && chgrp %s %s" % (gid, new)
            cmds += "\t%s && chmod %s %s && \\\n\t    mv -f %s %s\n" % (
                cmd, mode, new, new, w[1])
        elif len(w) == 3 and w[0] == "h" and w[2] in where:
            cmds += "\trm -f '%s'\n\tln %s '%s'\n" % (w[1], w[2], w[1])
    of = {f: name for name, made_ in data for f in made_}
    at = {}
    for dest, mode, f in placing:
        datacmds += placed(f, dest, mode)
        datainstalled.append((of[f], f, dest))
        at[dest] = f
    # The pages, a directory of them at a time, and then their other
    # names, each in place of the one that was.
    datacmds += (
        "\tcd man && for d in %s; do for f in $$d/*.0; do \\\n"
        "\t    cp $$f /usr/man/$$f.new && chmod 444 /usr/man/$$f.new && \\\n"
        "\t    mv -f /usr/man/$$f.new /usr/man/$$f || exit 1; done; done\n"
        % " ".join(mandirs) + placed("man/whatis", "/usr/man/whatis", "444"))
    datainstalled += [("man", p, "/usr/" + p) for p in pages]
    datainstalled.append(("man", "man/whatis", "/usr/man/whatis"))
    for dest, target in linking:
        datacmds += "\trm -f '%s'\n\tln '%s' '%s'\n" % (dest, target, dest)
        datainstalled.append((of[at[target]], at[target], dest))
    for (d, name), (pd, page) in sorted(names.items()):
        datacmds += "\trm -f /usr/man/%s/%s\n\tln /usr/man/%s/%s " \
            "/usr/man/%s/%s\n" % (d, name, pd, page, d, name)
        datainstalled.append(("man", "man/%s/%s" % (pd, page),
                              "/usr/man/%s/%s" % (d, name)))
    install = "".join(
        "\tcp %s %s/%s\n\tranlib %s/%s\n" % (
            a, USRLIB[a.split("/")[1]], os.path.basename(a),
            USRLIB[a.split("/")[1]], os.path.basename(a))
        for a in libnames)
    text = (HEAD % "The system's programs, and its libraries but the C "
            "library: make, and they are made here." + """\
#
#	make		the libraries, lib/MODEL/libNAME.a for each of
#			the three models, and the programs, bin/NAME
#	make libs	the libraries alone
#	make install-libs  and they are in /usr/lib, /usr/lib/m and
#			/usr/lib/l, where cc -l finds them
#	make progs	the programs alone, with the libraries that are
#			in /lib and /usr/lib
#	make install-progs  and each is where the system has it, without
#			its symbols, in place of the one that was, which
#			is there until the new one is whole: all of
#			them, if all are made, or none
#	make bin/NAME	one program
#	make data	what the system has that is not a program and
#			is made: the databases and tables of /etc and
#			/usr/share, spell's lists, the time zones, the
#			games' files, the manual as it is read
#	make data-NAME	some of that: """ + ", ".join(
        n for n, _ in data).replace(", man", ",\n#\t\t\tman") + """
#	make install-data  and each is where the system has it, but those
#			that are a new system's and not this one's: the
#			users, the mail aliases, the table of disks
#	make clean	nothing made is left
#
# A program is one command of cc, which compiles every file of it, in
# the directory o, and leaves nothing there.  What yacc and lex make
# of a grammar is in gen and games, as it is in obj where the system
# is built, in the directory it is made in there; so is what mkstr
# makes of ex's files and tcsh's, in ex and tcsh, and what the games'
# own programs make of theirs.
""" + ("# Not made here, their sources being made by something the system "
       "does not\n# have:\n#\n" + "".join(
           "#\t%s\n" % " ".join(left[i:i + 8])
           for i in range(0, len(left), 8)) if left else "") + """
S=	/usr/src
""" + macros + fold("LIBS", libnames) +
            fold("PROGS", ["bin/" + n for n in made]) +
            "".join(fold(n, w) for n, w in sysmacros) + datatext +
            """
all:	libs progs

libs:	$(LIBS)

progs:	$(PROGS)

install-libs: libs
""" + install + """
install-progs:
	@for f in $(PROGS); do test -f $$f || { echo "$$f is not made"; exit 1; }; done
""" + cmds + """
data:	""" + " ".join("data-" + n for n, _ in data) + """

""" + targets + """install-data: data
""" + datacmds + """
clean:
	rm -f hdrs bin/* lib/*/*.a lib/*/*/*.o o/*.o mkstr bmkstr hcrypt
	rm -f mkerrlst strfile initdeck bcomp setfiles sm mktab mktab.o
	rm -rf """ + " ".join(d for d in datadirs if not any(
        d.startswith(e + "/") for e in datadirs)) + """
	mkdir """ + " ".join(datadirs) + """
	rm -f games/fortunes """ + " ".join(
        f for f in every if os.path.dirname(f) == "games") + """
""" + "".join("\trm -f %s/*%s\n" % (d, ".[ch]" if any(
        g.startswith(d + "/") for g in gendirs) else "") for d in gendirs) + """\

""" + stamprule + "\n" + "\n".join(rules))
    return (text, dirs, gendirs, [l[0] for l in libs], made, left, installed,
            files, datadirs, data, datainstalled)


# What the system is given of usr/src besides what its programs and
# libraries are made from: each directory with the files in it, and not
# what is in directories below.
DIRS = ["sys/h", "sys/i286", "sys/i286at", "sys/sys", "sys/net",
        "sys/netinet", "sys/i286if", "sys/PC", "sys/i286stand",
        MD + "/crt", MD + "/csu", MD + "/sys"]
# Directories that it is given the headers of and nothing else: the
# kernel's sources name some of the PDP-11's.
HEADERS = ["sys/pdp", "sys/pdpif", "sys/pdpuba"]
# Names in those for other directories.
LINKS = ["sys/machine"]


# The directories of programs, which are made from the Makefiles there,
# upstream's.
PROGDIRS = ["bin", "sbin", "libexec", "ucb", "usr.bin", "usr.sbin", "games",
            "new"]


def walked(d):
    """The directories make is in when it is run in D, D among them:
    those D's Makefile names in SUBDIR or goes into to run make, and
    so on from theirs; and what is under any of them with no Makefile
    of its own, which is its files."""
    out = [d]
    text = open(os.path.join(SRC, d, "Makefile"),
                encoding="latin-1").read().replace("\\\n", " ")
    names = set()
    for m in re.finditer(r"^SUBDIRS?\s*=(.*)$", text, re.M):
        names.update(m.group(1).split())
    names.update(re.findall(r"^\tcd ([\w.+-]+)\s*[;&].*\bmake\b", text, re.M))
    for name in sorted(os.listdir(os.path.join(SRC, d))):
        sub = d + "/" + name
        if not os.path.isdir(os.path.join(SRC, sub)):
            continue
        if os.path.isfile(os.path.join(SRC, sub, "Makefile")):
            if name in names:
                out += walked(sub)
        elif not os.path.isfile(os.path.join(SRC, sub, "makefile")):
            out += [os.path.relpath(dp, SRC)
                    for dp, _, _ in os.walk(os.path.join(SRC, sub))]
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    made = {"sys/PC/Makefile": kernel(), "sys/i286stand/Makefile": stand()}
    dirs = set(DIRS)
    empty = ["i286/bin", "i286/o"]
    for m in "sml":
        text, used = libc(m)
        made["%s/%s/Makefile" % (MD, m)] = text
        dirs |= used
        empty.append("%s/%s/sys" % (MD, m))
    text, used, gendirs, libs, progs, left, installed, files, datadirs, \
        data, datainstalled = programs()
    made["i286/Makefile"] = text
    dirs |= used
    for d in PROGDIRS:
        dirs.update(walked(d))
    empty += ["i286/" + d for d in gendirs + datadirs]
    empty += ["i286/lib/%s/%s" % (m, l) for m in "sml" for l in libs]
    made[MD + "/Makefile"] = HEAD % (
        "The C library, of each model: make, and each is made in its "
        "directory.") + """
all install clean:
	cd s; make $@
	cd m; make $@
	cd l; make $@
"""
    for name, text in made.items():
        if text is not None:
            p = os.path.join(out, name)
            os.makedirs(os.path.dirname(p), exist_ok=True)
            with open(p, "w") as f:
                f.write(text)
    proto = []
    seen = set()

    def directory(d):
        if d and d not in seen:
            directory(os.path.dirname(d))
            seen.add(d)
            proto.append("d /%s 755" % d)

    for d in sorted(dirs):
        directory(d)
        for name in sorted(os.listdir(os.path.join(SRC, d))):
            rel = d + "/" + name
            p = os.path.join(SRC, rel)
            # A name for another file is given what that file has in
            # it, which may be where the system has nothing.  What the
            # cross compiler says of a file is left beside it.
            # One that is run, a script a Makefile has, is to be run
            # there.
            if rel not in made and os.path.isfile(p) and \
                    not name.endswith(".err"):
                proto.append("f /%s %s %s" % (
                    rel, "755" if os.access(p, os.X_OK) else "644", p))
    for rel in sorted(files):
        if os.path.dirname(rel) not in dirs:
            directory(os.path.dirname(rel))
            proto.append("f /%s 644 %s" % (rel, os.path.join(SRC, rel)))
    for d in HEADERS:
        directory(d)
        for name in sorted(os.listdir(os.path.join(SRC, d))):
            if name.endswith(".h"):
                proto.append("f /%s/%s 644 %s" % (
                    d, name, os.path.join(SRC, d, name)))
    for rel in sorted(made):
        directory(os.path.dirname(rel))
        proto.append("f /%s 644 %s" % (rel, os.path.abspath(
            os.path.join(out, rel))))
    for d in empty:
        directory(d)
    for l in LINKS:
        directory(os.path.dirname(l))
        proto.append("l /%s %s" % (l, os.readlink(os.path.join(SRC, l))))
    with open(os.path.join(out, "proto"), "w") as f:
        f.write("\n".join(proto) + "\n")
    with open(os.path.join(out, "programs"), "w") as f:
        f.write("\n".join(progs) + "\n")
    with open(os.path.join(out, "libraries"), "w") as f:
        f.write("\n".join(libs) + "\n")
    with open(os.path.join(out, "installed"), "w") as f:
        f.write("".join("%s %s\n" % i for i in installed))
    with open(os.path.join(out, "data"), "w") as f:
        f.write("".join("%s %s\n" % (name, p) for name, ps in data
                        for p in ps))
    with open(os.path.join(out, "datainstalled"), "w") as f:
        f.write("".join("%s %s %s\n" % i for i in datainstalled))
    if left:
        sys.stderr.write("srcmk: not made on the system yet: %s\n" %
                         " ".join(left))


if __name__ == "__main__":
    main()
