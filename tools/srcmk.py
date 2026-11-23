#!/usr/bin/env python3
"""Write what the system needs to make itself: its /usr/src.

usage: srcmk.py OUT

The system is built here by GNU make, from Makefiles that say what each
part is made of (usr/src/i286).  The system has a make of its own,
which reads none of that: no pattern rules, no functions, no
conditionals.  This writes for it, from the same tables, a Makefile
that it does read, with a rule for every file, and a proto for mkfs.py
of the file system that is to be /usr/src there: the sources, in the
directories they have here, and the Makefile where the one it was
written from is.

    OUT/i286/Makefile           what the system has that is not a
                                program and is made, and the programs
    OUT/proto                   the file system, to be mounted on
                                /usr/src
    OUT/programs                the names of the programs the last makes
    OUT/installed               each program it installs, and where
    OUT/data                    each file that is not a program that it
                                makes (make data), after the name of
                                what it is of (make data-NAME)
    OUT/datainstalled           each of those it installs, and where;
                                under "progs", what the programs'
                                install does

What make makes there is left where it is run, as upstream's Makefiles
leave it.  Each is to be, to the byte, what is made of the same sources
here (tests/self).  The Makefile has "install", which puts what was
made where the root file system has it (usr/src/i286/root.proto), with
its modes and owners.

The rest is made from the Makefiles of its own directories, which are
given as they are: the kernel from the Makefile config wrote in
sys/PC (sys/conf/config), /boot and the boot blocks from
sys/i286stand's, and, from upstream's, the libraries, the C library
and those of usr.lib, and the programs of the directories of PROGDIRS,
with the directories each model's objects are put in, m and l, where
upstream's have profiled.
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


# Where a model's library and its start-up code are on the system.
LIBDIR = {"s": "/lib", "m": "/lib/m", "l": "/lib/l"}
USRLIB = {"s": "/usr/lib", "m": "/usr/lib/m", "l": "/usr/lib/l"}


def programs():
    """The Makefile of the programs; the directories of usr/src they are
    made from, and those of /usr/src/i286 that what yacc and lex make is
    put in; the names of the programs it makes, and of those it does
    not."""
    progs = []
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
    text = (HEAD % "The system's programs: make, and they are made "
            "here." + """\
#
#	make		the programs, bin/NAME
#	make progs	the same, with the libraries that are in /lib
#			and /usr/lib
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
""" + macros +
            fold("PROGS", ["bin/" + n for n in made]) +
            "".join(fold(n, w) for n, w in sysmacros) + datatext +
            """
all:	progs

progs:	$(PROGS)

install-progs:
	@for f in $(PROGS); do test -f $$f || { echo "$$f is not made"; exit 1; }; done
""" + cmds + """
data:	""" + " ".join("data-" + n for n, _ in data) + """

""" + targets + """install-data: data
""" + datacmds + """
clean:
	rm -f hdrs bin/* o/*.o mkstr bmkstr hcrypt
	rm -f mkerrlst strfile initdeck bcomp setfiles sm mktab mktab.o
	rm -rf """ + " ".join(d for d in datadirs if not any(
        d.startswith(e + "/") for e in datadirs)) + """
	mkdir """ + " ".join(datadirs) + """
	rm -f games/fortunes """ + " ".join(
        f for f in every if os.path.dirname(f) == "games") + """
""" + "".join("\trm -f %s/*%s\n" % (d, ".[ch]" if any(
        g.startswith(d + "/") for g in gendirs) else "") for d in gendirs) + """\

""" + stamprule + "\n" + "\n".join(rules))
    return (text, dirs, gendirs, made, left, installed,
            files, datadirs, data, datainstalled)


# What the system is given of usr/src besides what its programs and
# libraries are made from: each directory with the files in it, and not
# what is in directories below.
DIRS = ["sys/h", "sys/i286", "sys/i286at", "sys/sys", "sys/net",
        "sys/netinet", "sys/i286if", "sys/conf", "sys/PC", "sys/i286stand"]
# Directories that it is given the headers of and nothing else: the
# kernel's sources name some of the PDP-11's.
HEADERS = ["sys/pdp", "sys/pdpif", "sys/pdpuba"]
# Names in those for other directories.
LINKS = ["sys/machine"]


# The directories of programs, which are made from the Makefiles there,
# upstream's; and of libraries, the same.  The C library's is given
# whole: its Makefiles name their directories by macros, and the 286's
# name the PDP-11's C (lib/libc/i286/gen).
PROGDIRS = ["bin", "sbin", "libexec", "ucb", "usr.bin", "usr.sbin", "games",
            "new"]
LIBDIRS = ["usr.lib"]
WHOLE = ["lib/libc"]
# A Makefile that puts the medium model's objects in m and the large's
# in l, as upstream's put the profiled ones in profiled, which its
# directory has.
MODELS = re.compile(r"^\t.*(?:\bcd [ml];|[\s(][ml]/)", re.M)


def models(d):
    """The directories D's Makefile puts each model's objects in."""
    p = os.path.join(SRC, d, "Makefile")
    if os.path.isfile(p) and MODELS.search(open(p, encoding="latin-1").read()):
        return [d + "/m", d + "/l"]
    return []


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
    made = {}
    dirs = set(DIRS)
    empty = ["i286/bin", "i286/o"]
    text, used, gendirs, progs, left, installed, files, datadirs, \
        data, datainstalled = programs()
    made["i286/Makefile"] = text
    dirs |= used
    for d in PROGDIRS + LIBDIRS:
        dirs.update(walked(d))
    for d in WHOLE:
        dirs.update(os.path.relpath(dp, SRC)
                    for dp, _, _ in os.walk(os.path.join(SRC, d)))
    for d in sorted(dirs):
        empty += models(d)
    empty += ["i286/" + d for d in gendirs + datadirs]
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
