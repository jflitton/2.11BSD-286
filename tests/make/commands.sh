# commands.sh TOP OUT: write OUT/bin, the system's commands as this
# machine has them, for the Makefiles tests/make/run.sh runs:
#
#	sh		sh.py, for make to run its commands with, which
#			runs mkstubs, which libc's Makefile makes and runs,
#			as it is compiled for this machine
#	cc		cc.py
#	as, ld, cpp	the system's, as compiled for this machine
#	yacc, lex	the same
#	xstr, mkstr, tsort  the tree's (ucb/xstr.c, ucb/mkstr, usr.bin/tsort.c),
#			compiled here for this machine
#	ar, ranlib	ar.py, which makes a library whole each time with
#			tools/ar.py, with ranlib's table
#	lorder		tools/lorder.py, which prints what the system's
#			does
#	nroff, manroff	the system's nroff, as compiled for this machine
#	strip		tools/strip.py
#	install		install.py
#	expr		this machine's, and the system's substr
#	size		nothing
#	ln, rm, mv, cp, mkdir	this machine's, on OUT/dest where
#			the name is one in the system's root
#	chmod		chmod.py, the same, which says in OUT/dest.meta what
#			mode it was to be: this machine may not set setuid
#	chown, chgrp	chown.py, which says in OUT/dest.meta whose a file
#			was to be: only the system gives one away
#	make		this machine's, to which each of the root's
#			directories is there and old, as on the system,
#			where a rule that makes one does not run (-o, which
#			make does not pass on to the makes it runs)
top=$1 out=$2
hostmake=$(command -v make)
b=$out/bin
h=$out/hostbin
rm -rf "$b"
mkdir -p "$b" "$h"
hostcc="cc -std=gnu89 -w -fno-strict-aliasing -Wno-error=return-mismatch
    -Wno-error=int-conversion -Wno-error=implicit-function-declaration
    -Wno-error=implicit-int"
s=$top/usr/src
[ "$h/xstr" -nt "$s/ucb/xstr.c" ] || $hostcc -o "$h/xstr" "$s/ucb/xstr.c"
[ "$h/mkstr" -nt "$s/ucb/mkstr/mkstr.c" ] ||
    $hostcc -o "$h/mkstr" "$s/ucb/mkstr/mkstr.c"
[ "$h/tsort" -nt "$s/usr.bin/tsort.c" ] ||
    $hostcc -o "$h/tsort" "$s/usr.bin/tsort.c"
m=$s/lib/libc/i286/sys/mkstubs.c
[ "$h/mkstubs" -nt "$m" ] || $hostcc -o "$h/mkstubs" "$m"

run() {		# run NAME COMMAND...: NAME runs COMMAND with its arguments
    n=$1
    shift
    { echo '#!/bin/sh'; echo "exec $* \"\$@\""; } > "$b/$n"
}
run sh python3 -I "$top/tests/make/sh.py" "$h"
run cc python3 -I "$top/tests/make/cc.py" "$top" "$out"
run install python3 -I "$top/tests/make/install.py" "$out"
# expr substr, which the system's expr has and this machine's may not
cat > "$b/expr" <<'END'
#!/bin/sh
if [ "$1" = substr ]; then
    printf '%s\n' "$2" | cut -c"$3"-"$(($3 + $4 - 1))"
else
    exec /bin/expr "$@"
fi
END
for t in as ld cpp; do run $t "$top/obj/host/ccom/$t"; done
for t in yacc lex; do run $t "$top/obj/host/$t"; done
for t in xstr mkstr tsort; do run $t "$h/$t"; done
run manroff nroff -man -h
for t in size ranlib; do
    printf '#!/bin/sh\nexit 0\n' > "$b/$t"
done
for t in chown chgrp; do run $t python3 -I "$top/tests/make/chown.py" "$out" $t; done
run chmod python3 -I "$top/tests/make/chmod.py" "$out"
cat > "$b/make" <<END
#!/bin/sh
exec $hostmake \$(sed 's/^/-o /' "$out/dirs") "\$@"
END
run ar python3 -I "$top/tests/make/ar.py" "$top"
run lorder python3 -I "$top/tools/lorder.py"
# nroff finds its macros and tables beside it
cat > "$b/nroff" <<END
#!/bin/sh
n=\$#
for a; do
    case \$a in -*|/*) ;; *) [ -f "\$a" ] && a=\$PWD/\$a ;; esac
    set -- "\$@" "\$a"
done
shift \$n
cd "$top/obj/host" && exec ./nroff "\$@"
END
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
    /bin|/bin/*|/sbin|/sbin/*|/etc|/etc/*|/lib|/lib/*|/usr|/usr/*|/var|/var/*)
        a=$out/dest\$a ;;
    esac
    set -- "\$@" "\$a"
done
shift \$n
exec /bin/$t "\$@"
END
done
chmod +x "$b"/*
