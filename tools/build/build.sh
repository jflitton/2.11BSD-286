#!/bin/sh
# Make the system where it is built as it makes itself
# (docs/self-hosting.md): with upstream's Makefiles, run by this
# machine's make with the system's commands as this machine has them
# (commands.sh), installing in a root of its own as `make install`
# installs in the system's.  usr/src/GNUmakefile runs this: make -C
# usr/src, and make -C usr/src root for the root file system,
# obj/root/root.fs, which the tests boot.
#
#	sh tools/build/build.sh [root]
#
# From the environment, as the GNUmakefile gives them:
#
#	TOOLS		native, the system's own compiler, assembler and
#			linker as built for this machine (tools/cc -ccom),
#			in obj/nbuild; or cross, the cross compiler, in
#			obj/build (docs/development.md)
#	KERNEL, STAND	the kernel that is /unix, and the directory of
#			/boot and the boot blocks: by default those of the
#			build this is, obj/nPC and obj/nstand, or obj/PC
#			and obj/stand, which their own GNUmakefiles make,
#			as upstream makes them apart
#	ROOTDISK	the disk the root is on, for /etc/fstab: wd0, the
#			first on the AT disk controller, unless it is sd0,
#			the first SCSI disk, or another
#	ROOTSIZE	the root file system's size in blocks of 1K,
#			24000, with room for what a system that is used
#			writes there
#	STRIP		no, for the root's programs to keep their symbols
#
# In OUT, obj/nbuild or obj/build:
#
#  1. What it runs is made: the system's compiler, assembler and linker
#     for this machine (usr/src/lib/ccom/i286), and its yacc, lex, nroff,
#     eqn and zic (host.mk), in obj/host; the C library as those make it
#     (usr/src/lib/libc/i286), in obj/nlib; and OUT/bin, its commands.
#     With the cross compiler, the libraries its programs are linked
#     with are made first, of its own objects, the C library and the
#     others, by their Makefiles, in a root of their own, OUT/xlib/dest:
#     steps 2 and 4 in OUT/xlib, of lib/libc and usr.lib alone, with
#     obj/lib's C library (usr/src/lib/libc/i286 TOOLS=cross) first.
#  2. usr/src is copied to OUT/root/usr/src, where what was made before
#     is kept (stage.py), and each directory whose Makefile or sources
#     have changed is cleaned.  When what every program is made with has
#     changed, the compiler, its tools or the headers (usr/include's .h
#     files, not the kernel's sources its links reach), all is made anew.
#  3. The root is made anew, OUT/dest, by etc's distribution, which makes
#     its directories, /etc and /dev (usr/src/etc/Makefile), and with what
#     it has of 2.11BSD's distribution that /usr/src makes nothing of
#     (tape.py).
#  4. The C library and the compiler are made and installed there, and
#     the other libraries, as make build in usr/src does, once.  A system
#     that makes itself has a C library before it makes another, which
#     its Makefile has a program linked with before it installs the new
#     one (lib/libc/i286/sys's mkstubs): the root is given obj/nlib's
#     first, which make build's own replaces.  make build does it all
#     twice, to have the compiler it has just made make the library and
#     itself, which here is this machine's build of it either way.
#     When a library is not what it was, the programs are made again,
#     and the compiler when the C library is not.
#  5. The rest is made, make in each of usr/src's SRCDIR, and make
#     installsrc installs it, after the scripts the manual's Makefiles
#     run, which a system has before it makes anything.  A command that
#     fails fails the build, which says where (OUT/log).  What upstream
#     has and does not make, which is ported, is made too and not
#     installed (EXTRA), for the tests to install where they test it.
#  6. The kernel and the boot programs are installed, as their Makefiles
#     install them, and with root, the root file system is made of OUT/dest
#     (proto.py) and /usr/include (tools/ccproto.py).
#
# Steps 3 to 5 are done only when something they are made of has
# changed since they last ended, each file by its size and time
# (OUT/built-with): the sources but the kernel's, the headers, the tools
# and the libraries they start with.  Otherwise OUT/dest is kept as they
# left it, and step 6 done to it again: a build of what has not changed
# takes seconds.
#
# The programs are kept with their symbols in OUT/syms, by the names
# they are installed by (install.py).
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tools/build
TOOLS=${TOOLS:-native}
case $TOOLS in
native) out=$top/obj/nbuild k=nPC s=nstand ;;
cross)  out=$top/obj/build k=PC s=stand ;;
*)      echo "build: TOOLS is native or cross" >&2; exit 2 ;;
esac
kernel=${KERNEL:-$top/obj/$k/unix}
stand=${STAND:-$top/obj/$s}
ROOTDISK=${ROOTDISK:-wd0}
ROOTSIZE=${ROOTSIZE:-24000}
export STRIP
# What the GNUmakefile's make passes on is not for the Makefiles here.
unset MAKEFLAGS MFLAGS MAKELEVEL MAKEOVERRIDES
log=$out/log
mkdir -p "$log" "$top/obj/root"

step() {	# step NAME COMMAND...: run COMMAND, its output in OUT/log/NAME
    n=$1
    shift
    if ! "$@" > "$log/$n" 2>&1; then
        tail -20 "$log/$n" >&2
        echo "build: $n failed ($log/$n)" >&2
        exit 1
    fi
}

# mk DIR TARGET...: make each TARGET, "" for the first, in DIR of
# O/root/usr/src, as the system's make would there; O is OUT, or
# OUT/xlib.  make is given mkvar too, NAME=VALUE, when it is set.
mk() {
    d=$1
    shift
    for t; do
        (cd "$o/root/usr/src/$d" && PATH="$o/bin:$PATH" &&
            MAKEFILES=$here/rules.mk && export PATH MAKEFILES &&
            make -r -k SHELL="$o/bin/sh" ${mkvar:+"$mkvar"} $t)
    done
}
mkvar=
o=$out

# failed LOG...: what make said failed, which fails the build.
failed() {
    f=$(cat "$@" | grep -E '^(g?make|make\[[0-9]+\]): \*\*\* ' |
        grep -v '(ignored)$' || true)
    if [ -n "$f" ]; then
        echo "$f" | head -20 >&2
        echo "build: what failed is in $*" >&2
        exit 1
    fi
}

# 1.
step host sh -c "make -s -C '$top/usr/src/lib/ccom/i286' host &&
    make -s -f '$here/host.mk' &&
    make -s -C '$top/usr/src/lib/libc/i286' TOOLS=native &&
    if [ '$TOOLS' = cross ]; then
        make -s -C '$top/usr/src/lib/libc/i286' TOOLS=cross; fi"
xout=$out/xlib
if [ "$TOOLS" = cross ]; then
    step commands sh "$here/commands.sh" "$top" "$out" cross "$xout/dest"
else
    step commands sh "$here/commands.sh" "$top" "$out"
fi

# 2.  What every program is made with: if it has changed, all is made
# anew.  The cross compiler's own tools are what a cross build's
# programs are made with, not a native build's.
sum() {
    python3 -I - "$top" "$TOOLS" <<'END'
import hashlib, os, sys
top, tools = sys.argv[1:3]
h = hashlib.sha256()
names = [os.path.join(top, "obj/host/ccom", n)
         for n in sorted(os.listdir(os.path.join(top, "obj/host/ccom")))]
names += [os.path.join(top, n) for n in ("tools/cc", "tools/ncc",
          "tools/build/cc.py", "tools/build/ld.py", "tools/build/sh.py",
          "tools/build/rules.mk")]
if tools == "cross":
    names += [os.path.join(top, n) for n in ("tools/cc286",
              "tools/cc286.h", "tools/commonize.py", "tools/build/as.py")]
# The headers alone: usr/include's links into the kernel's directories
# (i286, i286at, net, ...) find its sources there too, which no program
# includes.
for dp, dn, fn in os.walk(os.path.join(top, "usr/include"), followlinks=True):
    dn.sort()
    names += [os.path.join(dp, n) for n in sorted(fn) if n.endswith(".h")]
for n in names:
    if os.path.isfile(n):
        h.update(n.encode() + b"\0" + open(n, "rb").read())
print(h.hexdigest())
END
}
new=$(sum)
if [ "$new" != "$(cat "$out/made-with" 2>/dev/null)" ]; then
    rm -rf "$out/root" "$out/staged" "$out/libraries" "$out/sources" "$xout"
fi
step stage python3 -I "$here/stage.py" "$top" "$out"
echo "$new" > "$out/made-with"
for d in $(cat "$out/remade"); do
    mk "$d" clean || true
done > "$log/clean" 2>&1
# What was compiled is kept with what it was compiled to, but for the
# directories cleaned and those in them, which their clean cleans.
touch "$out/sources"
awk 'FILENAME == ARGV[1] { r[$1]; next }
    { for (d = $1; ; sub("/[^/]*$", "", d)) {
          if (d in r || "." in r) next
          if (d !~ "/") break
      }
      print }' "$out/remade" "$out/sources" > "$out/sources.new"
mv "$out/sources.new" "$out/sources"
awk -v d="$out/dest" '{ print $1; print d $1 }' "$top/usr/src/etc/dirs" \
    > "$out/dirs"

# libsum DIR...: a sum of the libraries and objects in DIRs of OUT/dest,
# a library by its members, which is what a program is linked with.  An
# object may be installed by a library's name: libI77's libI66.a.  The
# time mkvers puts in a Fortran library's Version is not counted, which
# would have every program made again each time.
libsum() {
    python3 -I - "$top" "$out/dest" "$@" <<'END'
import hashlib, os, re, sys
sys.path.insert(0, os.path.join(sys.argv[1], "tools"))
import ar
MKVERS = re.compile(rb"(@\(#\)lib[FIU]77\.a\t)[A-Z][a-z]{2} [ 0-9][0-9] "
                    rb"[0-9]{2}:[0-9]{2}:[0-9]{2} [0-9]{4}")
h = hashlib.sha256()
for d in sys.argv[3:]:
    for dp, dn, fn in os.walk(os.path.join(sys.argv[2], d)):
        dn.sort()
        for n in sorted(fn):
            p = os.path.join(dp, n)
            if not n.endswith((".a", ".o")):
                continue
            data = open(p, "rb").read()
            if n.endswith(".a") and data[:8] == b"!<arch>\n":
                for name, data in ar.read(p):
                    if name != "__.SYMDEF":
                        h.update(name.encode() + b"\0" +
                                 MKVERS.sub(rb"\1", data))
            else:
                h.update(data)
print(h.hexdigest())
END
}

# inputs: what steps 3 to 5 are made of, each file by its name, size
# and time: usr/src, but for the kernel's sources, which no program is
# made of, apart from the table of system calls that libc's stubs are;
# usr/include; tools; obj/host; the C libraries of obj/nlib and obj/lib;
# the files tape.py copies; and TOOLS, ROOTDISK and STRIP.
inputs() {
    python3 -I - "$top" "$TOOLS $ROOTDISK $STRIP" <<'END'
import hashlib, os, sys
top, env = sys.argv[1:3]
h = hashlib.sha256(env.encode() + b"\n")
def add(p):
    st = os.stat(p)
    h.update(b"%s %d %d\n" % (os.path.relpath(p, top).encode(), st.st_size,
                              st.st_mtime_ns))
def walk(d, keep=lambda p: True):
    for dp, dn, fn in os.walk(os.path.join(top, d)):
        dn[:] = sorted(n for n in dn if n != "__pycache__")
        for n in sorted(fn):
            p = os.path.join(dp, n)
            if keep(p) and os.path.isfile(p):
                add(p)
sys_ = os.path.join(top, "usr/src/sys") + os.sep
sysent = sys_ + "sys/init_sysent.c"
walk("usr/src", lambda p: not p.startswith(sys_) or p == sysent or
     not p.endswith((".c", ".s")))
for d in ("usr/include", "tools", "obj/host"):
    walk(d)
for lib in ("obj/nlib", "obj/lib"):
    for m in "sml":
        d = os.path.join(top, lib, m)
        if os.path.isdir(d):
            for n in sorted(os.listdir(d)):
                if os.path.isfile(os.path.join(d, n)):
                    add(os.path.join(d, n))
for line in open(os.path.join(top, "tools/build/tape.proto")):
    w = line.split()
    if len(w) >= 4 and w[0] == "f":
        add(os.path.join(top, w[3]))
print(h.hexdigest())
END
}

# Steps 3 to 5 are done again only when what they are made of is not
# what it was when they last ended (OUT/built-with).  Otherwise OUT/dest
# is as they left it, with what step 6 put there taken out, and so is
# OUT/meta (OUT/meta.5).
given=$(inputs)
if [ -d "$out/dest" ] && [ -s "$out/meta.5" ] &&
    [ "$given" = "$(cat "$out/built-with" 2>/dev/null)" ]; then
    remake=
    rm -f "$out/dest/unix" "$out/dest/boot" "$out/dest/mdec/wduboot" \
        "$out/dest/mdec/sduboot" "$out/dest/mdec/fluboot"
    cp "$out/meta.5" "$out/meta"
    echo "build: ${out#$top/}/dest as it was: nothing it is made of has changed"
else
    remake=1
    rm -f "$out/built-with" "$out/meta.5"
fi

# 1, with the cross compiler: its libraries.
if [ "$TOOLS" = cross ] && [ -n "$remake" ]; then
    mkdir -p "$xout/log"
    step xlib sh -c "sh '$here/commands.sh' '$top' '$xout' xlib '$xout/dest' &&
        python3 -I '$here/stage.py' '$top' '$xout'"
    rm -rf "$xout/dest"
    mkdir -p "$xout/dest/lib/m" "$xout/dest/lib/l" "$xout/dest/usr/lib/m" \
        "$xout/dest/usr/lib/l"
    : > "$xout/meta"
    sed "s,$out/dest,$xout/dest," "$out/dirs" > "$xout/dirs"
    # The C library its Makefile has a program linked with before it
    # installs the new one, as in step 4: obj/lib's, a library of the
    # cross compiler's linker, which tools/cc takes as it is.
    for m in s m l; do
        d=$xout/dest/lib/$m
        [ $m = s ] && d=$xout/dest/lib
        cp "$top/obj/lib/$m/crt0.o" "$d"
        cp "$top/obj/lib/$m/libc.lib" "$d/libc.a"
    done
    o=$xout
    for d in $(cat "$xout/remade"); do
        mk "$d" clean || true
    done > "$xout/log/clean" 2>&1
    # Not the Fortran libraries: no program is linked with them here,
    # and the cross compiler stops at their calls, which pass a char *
    # for what is declared otherwise.
    mkvar="SUBDIR=$(sed -n '/^SUBDIR=/,/[^\\]$/p' \
        "$top/usr/src/usr.lib/Makefile" | sed 's/^SUBDIR=//; s/\\$//' |
        tr -s ' \t\n' '\n\n\n' | grep -v -x -e '' -e 'lib[FIU]77' |
        tr '\n' ' ' | sed 's/ $//')"
    {
        mk lib/libc "" install
        mk usr.lib "" install
    } > "$xout/log/lib" 2>&1
    mkvar=
    o=$out
    failed "$xout/log/lib"
fi

if [ -n "$remake" ]; then
    # 3.
    rm -rf "$out/dest" "$out/syms"
    rm -f "$out/meta" "$out/devices"
    mkdir -p "$out/dest"
    : > "$out/meta"
    DISTRIBUTION=1 && export DISTRIBUTION
    step distribution mk etc "distribution DESTDIR=$out/dest"
    unset DISTRIBUTION
    step tape python3 -I "$here/tape.py" "$top" "$out"

    # 4.
    for m in s m l; do
        d=$out/dest/lib/$m
        [ $m = s ] && d=$out/dest/lib
        mkdir -p "$d"
        cp "$top/obj/nlib/$m/crt0.o" "$top/obj/nlib/$m/libc.a" "$d"
    done
    mk lib/libc "" install > "$log/lib" 2>&1
    # The compiler is linked with the C library, and with nothing else that
    # is made here: when that is not what it was, so is the compiler.
    libc=$(libsum lib)
    if [ -s "$out/libc" ] && [ "$libc" != "$(cat "$out/libc")" ]; then
        for d in lib/ccom lib/cpp lib/c2; do
            mk "$d" clean
        done >> "$log/clean" 2>&1 || true
    fi
    echo "$libc" > "$out/libc"
    {
        mk lib "ccom cpp c2"
        mk lib/ccom install
        mk lib/cpp install
        mk lib/c2 install
        mk usr.lib "" install
    } >> "$log/lib" 2>&1
    # What upstream has and does not make (5.): sendmail.MX, a sendmail
    # that finds where to send mail by MX records (tests/mx).
    EXTRA=usr.sbin/sendmail.MX
    libs=$(libsum lib usr/lib)
    if [ -s "$out/libraries" ] && [ "$libs" != "$(cat "$out/libraries")" ]; then
        for d in $(sed -n 's/^SRCDIR=//p' "$top/usr/src/Makefile") $EXTRA; do
            mk "$d" clean
        done > "$log/clean" 2>&1 || true
    fi
    echo "$libs" > "$out/libraries"

    # 5.
    srcdir=$(sed -n 's/^SRCDIR=//p' "$top/usr/src/Makefile")
    {
        (mk man scriptinstall)
        (mk usr.sbin/makewhatis install)
        mk . "$srcdir"
        mk . installsrc
        for d in $EXTRA; do mk "$d" ""; done
    } > "$log/build" 2>&1
    failed "$log/lib" "$log/build"
    cp "$out/meta" "$out/meta.5"
    echo "$given" > "$out/built-with"
fi

# 6.
for f in "$kernel" "$stand/boot" "$stand/hdboot.bin" "$stand/flboot.bin"; do
    if [ ! -f "$f" ]; then
        echo "build: no $f (make -C usr/src/sys/PC and sys/i286stand)" >&2
        exit 1
    fi
done
step kernel sh -c "PATH='$out/bin:$PATH' && cd '$out' &&
    install -c -o root -g kmem -m 744 '$kernel' /unix &&
    install -c -o root -g wheel -m 744 '$stand/boot' /boot &&
    install -c -m 644 '$stand/hdboot.bin' /mdec/wduboot &&
    ln /mdec/wduboot /mdec/sduboot &&
    install -c -m 644 '$stand/flboot.bin' /mdec/fluboot"
if [ "$ROOTDISK" != wd0 ]; then
    sed "s,^/dev/wd0,/dev/$ROOTDISK," "$out/dest/etc/fstab" > "$out/fstab"
    cat "$out/fstab" > "$out/dest/etc/fstab"
    rm "$out/fstab"
fi
if [ "$1" = root ]; then
    python3 -I "$here/proto.py" "$top" "$out" > "$out/root.proto"
    python3 -I "$top/tools/ccproto.py" "$top" >> "$out/root.proto"
    step mkfs python3 -I "$top/tools/mkfs.py" -s "$ROOTSIZE" \
        "$out/root.proto" "$top/obj/root/root.fs"
    echo "build: obj/root/root.fs, the root of $TOOLS's build"
fi
