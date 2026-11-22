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
                                Makefile of its own and the system
                                calls' stubs (tools/mkstubs.py)
    OUT/i286/Makefile           the other libraries, and the programs
    OUT/proto                   the file system, to be mounted on
                                /usr/src
    OUT/programs, OUT/libraries the names of what the last makes

What make makes there is left where it is run, as upstream's Makefiles
leave it: unix in /usr/src/sys/PC, as in a directory of /sys made by
config.  Each is to be, to the byte, what is made of the same sources
here (tests/self).

Each Makefile has "install", which puts what was made where the system
boots it from: the kernel as /unix, the one that was there as
/unix.old, which /boot loads if it is named to it.
"""
import os
import re
import subprocess
import sys

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
#			partition table; fdboot.bin, a diskette's
#	make install	/boot and /mdec/wduboot are those
#	make clean	nothing made is left
#
# as -b writes the bytes alone, and ld -b /boot's code and data as one
# segment and nothing else.

S=	/usr/src
CRT=	$S/lib/libc/i286/crt
""" + macros + """
all:	fdboot.bin mbr.bin hdboot.bin boot

fdboot.bin: fdboot.s
	as -b -o fdboot.bin fdboot.s

mbr.bin: mbr.s
	as -b -o mbr.bin mbr.s

hdboot.bin: hdboot.s
	as -b -o hdboot.bin hdboot.s

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

install: boot hdboot.bin
	cp boot /boot
	cp hdboot.bin /mdec/wduboot

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


def libc(model, out):
    """The Makefile of one model's C library, and its system call
    stubs, which are written here into OUT (tools/mkstubs.py) for the
    system to assemble: the names of the files that were."""
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
    d = os.path.join(out, MD, model, "sys")
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        os.unlink(os.path.join(d, f))
    r = subprocess.run(
        ["python3", "-I", os.path.join(TOP, "tools", "mkstubs.py"),
         "--model", model, "--sysent",
         os.path.join(SRC, "sys/sys/init_sysent.c"), "-d", d,
         os.path.join(SRC, MD, "sys/syscalls.tab"),
         os.path.join(TOP, "usr/include/syscall.h")])
    if r.returncode != 0:
        sys.exit("srcmk: mkstubs.py failed")
    # In the order the library has them where the system is built,
    # which is the shell's there.
    stubs = subprocess.run("echo *.s", shell=True, cwd=d, text=True,
                           capture_output=True).stdout.split()
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
# The system calls' stubs, in sys, were written where the system is
# built, from i286/sys/syscalls.tab (tools/mkstubs.py).  Every object
# is made again when any header changes.

S=	/usr/src
CRT=	$S/lib/libc/i286/crt
CFLAGS=	-O -m%s -I/usr/include
ASM=	as -m%s -I$(CRT)%s
""" % (LIBDIR[model], LIBDIR[model], model, model, model, FAR[model]) +
            macros +
            fold("OBJS", [b + ".o" for b in t["OBJS"]]) +
            fold("STUBS", ["sys/" + f[:-2] + ".o" for f in stubs]) + """
all:	crt0.o libc.a

libc.a:	$(OBJS) sys/made
	rm -f libc.a
	ar cr libc.a $(OBJS) $(STUBS)
	ranlib libc.a

crt0.o:	$S/lib/libc/i286/csu/crt0.s $(CRT)/DEFS.inc
	$(ASM) -o crt0.o $S/lib/libc/i286/csu/crt0.s

sys/made: $S/lib/libc/i286/sys/syscalls.tab
	cd sys; for f in *.s; do as -m%s -o `basename $$f .s`.o $$f || exit 1; done
	touch sys/made

install: all
	cp crt0.o %s/crt0.o
	cp libc.a %s/libc.a
	ranlib %s/libc.a

clean:
	rm -f *.o sys/*.o sys/made libc.a hdrs

""" % (model, LIBDIR[model], LIBDIR[model], LIBDIR[model]) + hdrs + "\n" +
            "\n".join(rules))
    return text, ["sys/" + f for f in stubs], used


def programs():
    """The Makefile of the programs and of the libraries that are not
    the C library; the directories of usr/src they are made from; the
    names of the programs it makes, and of those it does not."""
    progs, libs = [], []
    for line in lines("i286"):
        kind, _, rest = line.partition(" ")
        if kind == "PROG":
            head, flags, srcs, ls = rest.split("|")
            name, model = head.split()
            progs.append((name, model, flags.strip(), srcs.split(),
                          ls.split()))
        elif kind == "LIB":
            name, flags, srcs = rest.split("|")
            libs.append((name.strip(), flags.strip(), srcs.split()))
    dirs = set()
    rules = []
    made, left = [], []

    def flagged(flags):
        """Flags as the system is to be given them: a directory of
        usr/src is named from /usr/src."""
        out = []
        for f in re.findall(r"'[^']*'|\S+", flags):
            if f.startswith("-I") and not f.startswith("-I/"):
                dirs.add(f[2:])
                f = "-I$S/" + f[2:]
            out.append(f)
        return "".join(" " + f for f in out)

    def sources(srcs):
        bases = [os.path.basename(f) for f in srcs]
        if len(set(bases)) != len(bases):
            sys.exit("srcmk: two sources of one name among %s" % srcs)
        for f in srcs:
            dirs.add(os.path.dirname(f))
        return ["$S/" + f for f in srcs]

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
    for name, model, flags, srcs, ls in progs:
        if any(TOP in x for x in [flags] + srcs):
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
#	make bin/NAME	one program
#	make clean	nothing made is left
#
# A program is one command of cc, which compiles every file of it, in
# the directory o, and leaves nothing there.  Those whose sources are
# themselves made, by yacc or lex or a program of their own, are not
# made here yet:
#
""" + "".join("#\t%s\n" % " ".join(left[i:i + 8])
              for i in range(0, len(left), 8)) + """
S=	/usr/src
""" + macros + fold("LIBS", libnames) +
            fold("PROGS", ["bin/" + n for n in made]) + """
all:	libs progs

libs:	$(LIBS)

progs:	$(PROGS)

install-libs: libs
""" + install + """
clean:
	rm -f hdrs bin/* lib/*/*.a lib/*/*/*.o o/*.o

""" + stamprule + "\n" + "\n".join(rules))
    return text, dirs, [l[0] for l in libs], made, left


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


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    out = sys.argv[1]
    made = {"sys/PC/Makefile": kernel(), "sys/i286stand/Makefile": stand()}
    dirs = set(DIRS)
    empty = ["i286/bin", "i286/o"]
    for m in "sml":
        text, stubs, used = libc(m, out)
        made["%s/%s/Makefile" % (MD, m)] = text
        dirs |= used
        for f in stubs:
            made["%s/%s/%s" % (MD, m, f)] = None
    text, used, libs, progs, left = programs()
    made["i286/Makefile"] = text
    dirs |= used
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
            if rel not in made and os.path.isfile(p) and \
                    not os.path.islink(p):
                proto.append("f /%s 644 %s" % (rel, p))
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
    if left:
        sys.stderr.write("srcmk: not made on the system yet: %s\n" %
                         " ".join(left))


if __name__ == "__main__":
    main()
