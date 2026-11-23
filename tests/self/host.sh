# tests/self with HOST set: read by run.sh once it has written what is
# to be made (fs/src) and what each file is to be (want.proto).  The
# Makefiles are run here, in $out/root/usr/src, by this machine's make,
# with these for the system's commands:
#
#	cc	tools/cc -ccom, which links as the system's does; with -c
#		the compiler alone, a file at a time (tools/ncc)
#	as, ld	the system's, as compiled for this machine
#	yacc, lex   the system's, as compiled for this machine
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

# Each file run.sh has the machine compare, compared here.
: > "$out/same"
: > "$out/differs"
: > "$out/unmade"
# A GNUmakefile, which config writes for where the system is built, is
# not in the tree here, for this machine's make would read it
# (hostroot.py).
sed -n 's/^f //p' want.proto | while read path mode from; do
    case $path in
    /check.sh|/self.sh|/install.sh) continue ;;
    */GNUmakefile) continue ;;
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
if [ -s "$out/unmade" ]; then
    sed 's/^/NOT MADE /' "$out/unmade" >&2
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
    "of usr/src make"
