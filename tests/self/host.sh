# tests/self with HOST set: read by run.sh once it has written what is
# to be made (fs/src) and what each file is to be (want.proto).  The
# Makefiles are run here, in $out/root/usr/src, by this machine's make,
# with these for the system's commands:
#
#	cc	tools/cc -ccom, which links as the system's does; with -c
#		the compiler alone, a file at a time (tools/ncc)
#	as, ld	the system's, as compiled for this machine
#	ar, ranlib  tools/ar.py, which makes the library whole: ar says
#		which files, ranlib makes it
#	yacc, lex   the system's, as compiled for this machine
#
# A program the Makefiles make and then run is the 286's and does not
# run here: what depends on one is not made, and is listed.
root=$out/root
rm -rf "$root"
python3 -I "$top/tests/self/hostroot.py" "$top" "$out/fs/src" "$root"
mkdir "$root/bin"
cat > "$root/bin/cc" <<END
#!/bin/sh
# A source is named as the tree has it, for what the compiler says of
# its name (__FILE__) to be what it is where the system is built.
n=\$#
c=
for a; do
    [ "\$a" = -c ] && c=1
    case \$a in
    "$root"/usr/src/*)
        b="$top/usr/src/\${a#$root/usr/src/}"
        if [ -L "\$a" ] && [ -f "\$b" ]; then a=\$b; fi ;;
    esac
    set -- "\$@" "\$a"
done
shift \$n
[ -n "\$c" ] || exec "$top/tools/cc" -ccom "\$@"
flags= files=
for a; do
    case \$a in
    -c) ;;
    *.c) files="\$files \$a" ;;
    *) flags="\$flags \$a" ;;
    esac
done
for f in \$files; do
    "$top/tools/ncc" \$flags "\$f" || exit 1
done
END
for t in as ld; do
    printf '#!/bin/sh\nexec "%s" "$@"\n' "$top/obj/host/ccom/$t" > "$root/bin/$t"
done
for t in yacc lex; do
    printf '#!/bin/sh\nexec "%s" "$@"\n' "$top/obj/host/$t" > "$root/bin/$t"
done
cat > "$root/bin/ar" <<'END'
#!/bin/sh
key=$1 lib=$2
shift 2
case $key in
cr) : > "$lib.members" ;;
r)  ;;
*)  echo "ar: only cr and r here, not $key" >&2; exit 1 ;;
esac
for f; do echo "$PWD/$f" >> "$lib.members"; done
END
cat > "$root/bin/ranlib" <<END
#!/bin/sh
exec python3 -I "$top/tools/ar.py" "\$1" \$(cat "\$1.members")
END
chmod +x "$root/bin"/*

hmake() {
    d=$1
    shift
    (cd "$root/usr/src/$d" && PATH="$root/bin:$PATH" DATE="$date" \
        make -k "$@") >> "$out/make.log" 2>&1 || true
}
: > "$out/make.log"
start=$(date +%s)
if has stand; then
    hmake sys/i286stand
fi
if has kernel; then
    hmake sys/PC
fi
if has libs; then
    hmake i286 libs
    for m in s m l; do
        for l in $libs; do
            a=$root/usr/src/i286/lib/$m/lib$l.a
            [ ! -f "$a" ] || python3 -I "$top/tools/ar.py" -t "$a" |
                sed 's/:.*//' > "$root/usr/src/i286/lib/$m/lib$l.t"
        done
    done
fi
if has progs; then
    if [ -n "$ONLY" ]; then
        hmake i286 $(echo $ONLY | tr ' ' '\n' | sed 's|^|bin/|')
    else
        hmake i286 progs
    fi
fi

# Each file run.sh has the machine compare, compared here.
: > "$out/same"
: > "$out/differs"
: > "$out/unmade"
sed -n 's/^f //p' want.proto | while read path mode from; do
    case $path in
    /rootbin/*|/check.sh|/self.sh|/install.sh|/installed.sh) continue ;;
    esac
    case $from in /*) ;; *) from=$out/$from ;; esac
    if [ ! -f "$root/usr/src$path" ]; then
        echo "$path" >> "$out/unmade"
    elif cmp -s "$from" "$root/usr/src$path"; then
        echo "$path" >> "$out/same"
    else
        echo "$path" >> "$out/differs"
    fi
done
n=$(wc -l < "$out/same" | tr -d ' ')
took=$(($(date +%s) - start))
# What is made with a program the Makefiles make and run, and so is not
# made here: the machine is to make these (tests/self/select.py has
# the programs' names for tests/self/changed.sh).
cat > "$out/left" <<END
/i286/bin/ex
/i286/bin/tcsh
/i286/bin/awk
/i286/bin/battlestar
/i286/bin/hack
/i286/ex/exstrings
/i286/tcsh/tcsh6.00strgs
/i286/games/battlestar/battle_strings
END
if grep -v -x -F -f "$out/left" "$out/unmade" > "$out/failed"; then
    sed 's/^/NOT MADE /' "$out/failed" >&2
    echo "$name: those were not made here ($out/make.log)" >&2
    exit 1
fi
if [ -s "$out/differs" ]; then
    sed 's/^/DIFFERS /' "$out/differs" >&2
    echo "$name: those are not made as they are by the Makefiles of" \
        "usr/src" >&2
    exit 1
fi
echo "$name: this machine's make made $n files of the system ($PARTS)" \
    "from the system's Makefiles in $took s, each what the Makefiles" \
    "of usr/src make; $(wc -l < "$out/unmade" | tr -d ' ') are left to" \
    "the 286"
