# commands.sh TOP OUT MODE LIBROOT: write OUT/bin, the system's commands
# as this machine has them, for the Makefiles tools/build/build.sh runs.
# MODE is native, cross or xlib, whose compiler it is (cc.py), and
# LIBROOT the root whose libraries cc links with:
#
#	sh		sh.py, for make to run its commands with, which
#			runs a program a Makefile makes and runs, as this
#			machine has it, compiled for it of the same C,
#			or, of OUT/hostbin/DIR, zic, built as obj/host's
#			is, and three scripts: strfile, initdeck and
#			setfiles, which write what the 286 lays out, by
#			tools/strfile.py, initdeck.py and setfiles.py
#	cc		cc.py
#	as		the system's, as compiled for this machine; with
#			the cross compiler, NASM (as.py)
#	cpp		the system's, as compiled for this machine
#	ld		the same, finding the root's libraries (ld.py);
#			with the cross compiler, cc.py's linking
#	yacc, lex	the same
#	xstr, mkstr, tsort, what  the tree's (ucb/xstr.c, ucb/mkstr,
#			usr.bin/tsort.c, ucb/what.c), compiled here for
#			this machine
#	ar, ranlib	ar.py, which makes a library whole each time with
#			tools/ar.py, with ranlib's table, or of the cross
#			compiler's objects, without
#	lorder		tools/lorder.py, which prints what the system's
#			does, or names a cross compiler's object alone
#	nroff, manroff	the system's nroff, as compiled for this machine
#			(nroff.py, which puts each .so in its place)
#	eqn		the same (host.mk makes both)
#	ex		ex.py, for share/termcap's Makefile, which has it
#			run reorder on termcap.src, and bin/awk's, which
#			has it run tokenscript
#	spellin, tbl	the tree's, compiled here for this machine:
#			spell's Makefile has the one the system has make
#			its lists, and phantasia's has tbl
#	mkerrlst	tools/mkerrlst.py, for the one mkerrlst's Makefile
#			installs and runs, which writes /etc/syserrlst
#	makewhatis	the system's script, which sh.py reads
#	mkpasswd	tools/mkpasswd.py, for -p, which etc's
#			distribution runs: FILE.orig, the password file,
#			and FILE.dir and FILE.pag, its database
#	newaliases	tools/mkaliases.py, for distribution's
#			newaliases -oAFILE: FILE.dir and FILE.pag
#	uuname		-l alone, uucp's name for the system, which uucp's
#			Makefile names two of its spool's directories by:
#			uucp.h's MYNAME, the name it has until rc.conf
#			gives it one, and not this machine's
#	mknod		mknod.py, which says in OUT/devices which files
#			are devices, for MAKEDEV
#	tar		this machine's, which says in OUT/meta that what
#			tar xp writes is root's and wheel's, as the files
#			of the system's /usr/src are, which it keeps
#	strip		tools/strip.py
#	install		install.py
#	expr		this machine's, and the system's substr
#	size		nothing
#	ln, rm, mv, cp, mkdir	this machine's, on OUT/dest where
#			the name is one in the system's root
#	chmod		chmod.py, the same, which says in OUT/meta what
#			mode it was to be: this machine may not set setuid
#	chown, chgrp	chown.py, which says in OUT/meta whose a file
#			was to be: only the system gives one away
#	make		this machine's, to which each of the root's
#			directories is there and old, as on the system,
#			where a rule that makes one does not run (-o, which
#			make does not pass on to the makes it runs)
top=$1 out=$2 mode=${3:-native} libroot=${4:-$2/dest}
hostmake=$(command -v make)
b=$out/bin
h=$out/hostbin
here=$top/tools/build
rm -rf "$b"
mkdir -p "$b" "$h"
hostcc="cc -std=gnu89 -w -fno-strict-aliasing -Wno-error=return-mismatch
    -Wno-error=int-conversion -Wno-error=implicit-function-declaration
    -Wno-error=implicit-int"
s=$top/usr/src
fresh() {	# fresh FILE SOURCE...: whether FILE is newer than each SOURCE
    f=$1
    shift
    for x; do [ "$f" -nt "$x" ] || return 1; done
}
[ "$h/xstr" -nt "$s/ucb/xstr.c" ] || $hostcc -o "$h/xstr" "$s/ucb/xstr.c"
[ "$h/mkstr" -nt "$s/ucb/mkstr/mkstr.c" ] ||
    $hostcc -o "$h/mkstr" "$s/ucb/mkstr/mkstr.c"
[ "$h/tsort" -nt "$s/usr.bin/tsort.c" ] ||
    $hostcc -o "$h/tsort" "$s/usr.bin/tsort.c"
[ "$h/what" -nt "$s/ucb/what.c" ] || $hostcc -o "$h/what" "$s/ucb/what.c"
mkdir -p "$h/share/zoneinfo" "$h/games/fortune" "$h/games/monop" \
    "$h/games/phantasia" "$h/include/sys"
ln -sf "$top/obj/host/zic" "$h/share/zoneinfo/zic"
cp "$s/sys/h/localopts.h" "$h/include/sys"
[ "$h/spellin" -nt "$s/usr.bin/spell/spellin.c" ] ||
    $hostcc -Dunix -I"$h/include" -o "$h/spellin" "$s/usr.bin/spell/spellin.c"
fresh "$h/tbl" "$s"/usr.bin/tbl/t* ||
    $hostcc -Dunix -o "$h/tbl" "$s"/usr.bin/tbl/t[0-9a-z].c
# strfile -r FILE writes FILE.dat; initdeck, cards.inp into cards.pck;
# setfiles, phantasia's directory, with umask(077), as it does.
cat > "$h/games/fortune/strfile" <<END
#!/bin/sh
r=
[ "\$1" = -r ] && { r=-r; shift; }
exec python3 -I "$top/tools/strfile.py" \$r "\$1" "\$1.dat"
END
cat > "$h/games/monop/initdeck" <<END
#!/bin/sh
exec python3 -I "$top/tools/initdeck.py" cards.inp cards.pck
END
cat > "$h/games/phantasia/setfiles" <<END
#!/bin/sh
umask 077
exec python3 -I "$top/tools/setfiles.py" "$out/dest/usr/games/lib/phantasia"
END
chmod +x "$h/games/fortune/strfile" "$h/games/monop/initdeck" \
    "$h/games/phantasia/setfiles"

run() {		# run NAME COMMAND...: NAME runs COMMAND with its arguments
    n=$1
    shift
    { echo '#!/bin/sh'; echo "exec $* \"\$@\""; } > "$b/$n"
}
run sh python3 -I "$here/sh.py" "$h" "$out"
run cc python3 -I "$here/cc.py" "$top" "$out" "$mode" "$libroot"
run install python3 -I "$here/install.py" "$out"
# expr substr, which the system's expr has and this machine's may not
cat > "$b/expr" <<'END'
#!/bin/sh
if [ "$1" = substr ]; then
    printf '%s\n' "$2" | cut -c"$3"-"$(($3 + $4 - 1))"
else
    exec /bin/expr "$@"
fi
END
for t in as cpp; do run $t "$top/obj/host/ccom/$t"; done
run ld python3 -I "$here/ld.py" "$top" "$out" "$mode" "$libroot"
[ "$mode" = native ] || run as python3 -I "$here/as.py" "$top" "$out" "$mode"
for t in yacc lex eqn; do run $t "$top/obj/host/$t"; done
run ex python3 -I "$here/ex.py" "$top"
run tar python3 -I "$here/tar.py" "$out"
run makewhatis "$b/sh" "$s/usr.sbin/makewhatis/makewhatis.sh"
for t in xstr mkstr tsort what spellin tbl; do run $t "$h/$t"; done
run mkerrlst python3 -I "$top/tools/mkerrlst.py" \
    "$s/usr.lib/liberrlst/errlst.c" "$out/dest/etc/syserrlst"
run manroff nroff -man -h
run mknod python3 -I "$here/mknod.py" "$out"
cat > "$b/mkpasswd" <<END
#!/bin/sh
[ \$# = 2 ] && [ "\$1" = -p ] || { echo "mkpasswd: only -p file here" >&2; exit 1; }
d=\$(mktemp -d "$out/mkpasswd.XXXXXX") &&
python3 -I "$top/tools/mkpasswd.py" "\$2" "\$d" &&
mv "\$d/passwd" "\$2.orig" && mv "\$d/passwd.dir" "\$2.dir" &&
mv "\$d/passwd.pag" "\$2.pag" && rmdir "\$d"
END
cat > "$b/newaliases" <<END
#!/bin/sh
case \$* in
-oA*) f=\${1#-oA} ;;
*) echo "newaliases: only -oAfile here" >&2; exit 1 ;;
esac
exec python3 -I "$top/tools/mkaliases.py" "\$f" "\$(dirname "\$f")"
END
cat > "$b/uuname" <<END
#!/bin/sh
[ "\$*" = -l ] || { echo "uuname: only -l here" >&2; exit 1; }
sed -n 's/^#define[[:space:]]*MYNAME[[:space:]]*"\(.*\)".*/\1/p' \
    "$s/usr.bin/uucp/uucp.h"
END
for t in size ranlib; do
    printf '#!/bin/sh\nexit 0\n' > "$b/$t"
done
for t in chown chgrp; do run $t python3 -I "$here/chown.py" "$out" $t; done
run chmod python3 -I "$here/chmod.py" "$out"
cat > "$b/make" <<END
#!/bin/sh
exec $hostmake \$(sed 's/^/-o /' "$out/dirs") "\$@"
END
run ar python3 -I "$here/ar.py" "$top"
run lorder python3 -I "$top/tools/lorder.py"
run nroff python3 -I "$here/nroff.py" "$top"
# strip writes the file over, as the system's does, which keeps it the
# file it was, with its mode
cat > "$b/strip" <<END
#!/bin/sh
for f; do
    python3 -I "$top/tools/strip.py" "\$f" "\$f.strip" &&
        cat "\$f.strip" > "\$f" && /bin/rm "\$f.strip" || exit 1
done
END
for t in ln rm mv cp mkdir; do
    cat > "$b/$t" <<END
#!/bin/sh
n=\$#
for a; do
    case \$a in //*) a=\${a#/} ;; esac
    case \$a in
    /bin|/bin/*|/sbin|/sbin/*|/etc|/etc/*|/lib|/lib/*|/usr|/usr/*|/var|/var/*|\\
    /mdec|/mdec/*|/unix|/unix.old|/boot)
        a=$out/dest\$a ;;
    esac
    set -- "\$@" "\$a"
done
shift \$n
exec /bin/$t "\$@"
END
done
chmod +x "$b"/*
