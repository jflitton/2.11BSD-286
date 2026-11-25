#!/usr/bin/env python3
"""The shell that make runs a Makefile's commands with, for
tools/build/build.sh.

usage: sh.py HOSTBIN OUT -c COMMAND
       sh.py HOSTBIN OUT FILE [ARG...]

A command that the Makefile names by where the system has it,
/usr/ucb/xstr or /lib/cpp, is the one of that name here (commands.sh):
at the start of COMMAND, or after ; | & ( or ` outside single quotes,
the directory is taken off.  Any other name of the system's root that COMMAND has after a
space, < > or (, such as the place `make install` puts a file, a
directory it goes into, one a redirection writes or a pattern of names,
or that a shell variable is given,
is that name in OUT/dest, the root here, as commands.sh's own commands
have it; but /usr/include and /usr/src, which are OUT/root's
(stage.py).  A PATH that a command sets has OUT/bin before what it
names, as make's own has, so that a script finds commands.sh's commands
as the system's: MAKEDEV's mknod.  Then COMMAND is the shell's, in the
C locale, as the system has no other.  A FILE, a script, is read and its
commands are seen to so, and it is run with its ARGs.

A program a Makefile makes and then runs, ./proc in bin/awk, is the
286's, and is run here as this machine has it, wherever the command
names it: compiled for this machine from the sources in the directory
it was made of, as they are when it is run (HOST, below), in
HOSTBIN/DIR/NAME; or, in HOSTBIN/DIR/NAME already, the one commands.sh
has there, a script for a program that writes what the 286 lays out
(strfile, initdeck, setfiles), or one built with what it needs (zic).
A COMMAND that is no more than one of the 286's that is not so, or one
the Makefile installed that is not one of commands.sh's,
/usr/bin/mkerrlst, does not run here: it is said so, and make goes on
with what is after it, as it would when the program had done what it
does on the system, where the files that it writes are left to the
286.
"""
import fnmatch
import os
import re
import subprocess
import sys

CMD = re.compile(r"(^\s*|[;|&(`]\s*)/(?:bin|sbin|usr/bin|usr/ucb|usr/sbin|"
                 r"lib|usr/lib|usr/man|usr/libexec|etc)/"
                 r"([A-Za-z0-9_.+-]+)(?=[\s;|&)`]|$)", re.M)
ALONE = re.compile(r"\s*(\.?/[A-Za-z0-9_./+-]+)\s*$")
# A program a Makefile makes and runs.
MADE = re.compile(r"(^\s*|[;|&(`]\s*)\./([A-Za-z0-9_.+-]+)(?=[\s;|&)<>`]|$)",
                  re.M)
# What each is compiled of here, in the directory of usr/src it is made
# in, with the flags it needs where this machine's int and pointer are
# not the 286's, or its headers declare what the program does (jove's
# getline): the C the Makefile compiles it of, as it is when the
# program is run (awk's token.c, which the Makefile writes first).  STEM
# is what * matched.
HOST = {
    "bin/awk/proc": "proc.c token.c",
    "bin/tcsh/mkprstr": "mkprstr.c",
    "games/battlestar/mkstr": "mkstr.c",
    "games/boggle/comp": "comp.c",
    "games/hack/crypt": "-Di286 crypt.c",
    "games/hack/makedefs": "makedefs.c",
    "games/warp/sm": "sm.c",
    "lib/libc/i286/sys/mkstubs": "mkstubs.c",
    "new/jove/setmaps": "-D_POSIX_C_SOURCE=200112L setmaps.c",
    "usr.bin/roff/nroff_term/*.tab.tmp": "-I../common_source mktab.c "
                                         "{stem}.c",
}
HOSTCC = ["/usr/bin/cc", "-std=gnu89", "-w", "-fno-strict-aliasing",
          "-Wno-error=return-mismatch", "-Wno-error=int-conversion",
          "-Wno-error=implicit-function-declaration",
          "-Wno-error=implicit-int"]
# A name, and whether it is one of the root's; and one a shell variable
# is given, dict=/usr/dict/words, but not a flag's, -DLOG=/var/log.
NAME = re.compile(r"(?<=[\s<>(])/[^\s;|&()`'\"<>]*")
ASSIGN = re.compile(r"(?<![^\s;(])([A-Za-z_]\w*=)(/[^\s;|&()`'\"<>]*)")
PATH = re.compile(r"(?<![^\s;(])PATH=")
ROOT = re.compile(r"/(?:(?:bin|sbin|etc|lib|var|mdec|usr)(?:/|$)|"
                  r"(?:unix|unix\.old|boot)$)")
OWN = re.compile(r"/usr/(?:include|src)(?:/|$)")
A_MAGIC = b"\x86\x02"   # the 286's executable (tools/strip.py)


def theirs(path):
    """Whether PATH is a program of the 286's."""
    try:
        with open(path, "rb") as f:
            return f.read(2) == A_MAGIC
    except OSError:
        return False


hostbin = sys.argv[1]
out = sys.argv[2]
dest = out + "/dest"
args = sys.argv[3:]


def rooted(p):
    """The name P in OUT/dest, if it is one of the root's."""
    return dest + p if ROOT.match(p) and not OWN.match(p) else p


def root(m):
    return rooted(m.group(0))


SRC = os.path.join(out, "root", "usr", "src")
TOP = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))
DIR = os.path.relpath(os.getcwd(), SRC)


def host(name):
    """The program NAME of this directory as this machine has it, or
    None.  One of HOST is that, whether or not the 286's is there yet,
    as nroff_term's is not until the command that runs it has made it."""
    p = os.path.join(hostbin, DIR, name)
    for pattern, how in HOST.items():
        d, n = os.path.split(pattern)
        if d != DIR or not fnmatch.fnmatchcase(name, n):
            continue
        words = how.format(stem=name.split(".")[0]).split()
        srcs = [w for w in words if not w.startswith("-")]
        if not os.path.isfile(p) or any(
                os.path.getmtime(s) > os.path.getmtime(p) for s in srcs):
            os.makedirs(os.path.dirname(p), exist_ok=True)
            subprocess.check_call(HOSTCC + ["-o", p] + words)
        return p
    return p if os.path.isfile(p) else None


def quoted(text, at):
    """Whether AT in TEXT is between single quotes, where ; and the
    rest are not the shell's: jove's sed -e 's;SHELL;/bin/csh;'.  A
    command there is a shell's that is given it, which is this one."""
    q = False
    for c in text[:at]:
        if c == "'":
            q = not q
        elif c == "\n":
            q = False
    return q


def command(text):
    """TEXT with the directory taken off each command of the system's."""
    return CMD.sub(lambda m: m.group(0) if quoted(text, m.start())
                   else m.group(1) + m.group(2), text)


def here(m):
    """./NAME, or the one of this machine."""
    if theirs("./" + m.group(2)) or not os.path.exists("./" + m.group(2)):
        p = host(m.group(2))
        if p:
            return m.group(1) + p
    return m.group(0)


# refer's runinv, which its install runs in /usr/dict/papers, is its mkey
# and inv, which are the 286's, and is tools/runinv.py here.
if args and os.path.basename(args[0]) == "runinv" and \
        os.path.realpath(os.getcwd()) == os.path.realpath(
            dest + "/usr/dict/papers"):
    papers = sorted(f for f in os.listdir(".") if f.startswith("R"))
    os.execvp("python3", ["python3", "-I", TOP + "/tools/runinv.py",
                          dest + "/usr/share/misc/eign", "."] + papers)
if args and not args[0].startswith("-") and os.path.isfile(args[0]):
    with open(args[0], errors="surrogateescape") as f:
        text = f.read()
    text = command(text)
    text = PATH.sub("PATH=" + out + "/bin:", NAME.sub(root, text))
    args = ["-c", text, args[0]] + args[1:]
elif len(args) >= 2 and args[0] == "-c":
    args[1] = MADE.sub(here, args[1])
    alone = ALONE.match(args[1])
    if alone and (theirs(alone.group(1)) if alone.group(1).startswith(".")
                  else theirs(dest + alone.group(1)) and not os.path.exists(
                      os.path.join(out, "bin",
                                   os.path.basename(alone.group(1))))):
        print("sh: %s is the 286's, and is not run here" % alone.group(1),
              file=sys.stderr)
        sys.exit(0)
    args[1] = command(args[1])
    args[1] = NAME.sub(root, args[1])
    args[1] = PATH.sub("PATH=" + out + "/bin:", ASSIGN.sub(
        lambda m: m.group(1) + rooted(m.group(2)), args[1]))
os.execve("/bin/sh", ["sh"] + args, dict(os.environ, LC_ALL="C"))
