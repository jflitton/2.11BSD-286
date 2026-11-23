# commands.sh TOP OUT: write OUT/bin, the system's commands as this
# machine has them, for the Makefiles tests/make/run.sh runs:
#
#	sh		sh.py, for make to run its commands with
#	cc		cc.py
#	as, ld, cpp	the system's, as compiled for this machine
#	yacc, lex	the same
#	xstr, mkstr, tsort  the tree's (ucb/xstr.c, ucb/mkstr, usr.bin/tsort.c),
#			compiled here for this machine
#	ar, ranlib	tools/ar.py, which makes a library whole of the
#			files ar is given
#	nroff, manroff	the system's nroff, as compiled for this machine
#	strip		tools/strip.py
#	install		install.py
#	expr		this machine's, and the system's substr
#	size		nothing
#	ln, rm, mv, cp, mkdir, chmod	this machine's, on OUT/dest where
#			the name is one in the system's root
#	chown, chgrp	nothing: only the system gives a file away
top=$1 out=$2
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

run() {		# run NAME COMMAND...: NAME runs COMMAND with its arguments
    n=$1
    shift
    { echo '#!/bin/sh'; echo "exec $* \"\$@\""; } > "$b/$n"
}
run sh python3 -I "$top/tests/make/sh.py"
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
for t in size chown chgrp ranlib; do
    printf '#!/bin/sh\nexit 0\n' > "$b/$t"
done
cat > "$b/ar" <<END
#!/bin/sh
key=\$1 lib=\$2
shift 2
case \$key in
*c*) : > "\$lib.members" ;;
*r*) [ -f "\$lib" ] || : > "\$lib.members" ;;
*)  echo "ar: only c and r here, not \$key" >&2; exit 1 ;;
esac
for f; do echo "\$PWD/\$f" >> "\$lib.members"; done
exec python3 -I "$top/tools/ar.py" "\$lib" \$(cat "\$lib.members")
END
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
cat > "$b/strip" <<END
#!/bin/sh
for f; do
    python3 -I "$top/tools/strip.py" "\$f" "\$f.strip" &&
        mv "\$f.strip" "\$f" || exit 1
done
END
for t in ln rm mv cp mkdir chmod; do
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
