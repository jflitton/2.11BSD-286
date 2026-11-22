#!/bin/sh
# Test that the system makes itself: boot it from a hard disk with its
# sources on a file system of their own, /usr/src, and have its make,
# compiler, assembler and linker make there, from the Makefiles that
# tools/srcmk.py writes for them: /boot and the boot blocks; the kernel;
# the C library of each model, which is then installed; the other
# libraries, which are installed too; and the programs, linked with
# those.  Each object, and each program, the kernel and /boot, is to be
# to the byte what the same sources make where the system is built,
# with the system's own tools as they are compiled for that machine
# (obj/nstand, obj/nPC, obj/nlib, obj/nbin), which the disk has on
# another file system to compare with.  A library is compared by its
# members: the archive has the day it was made in it.
#
# The kernel is then made once more, to say of itself that it was made
# there, and installed with /boot (make install); the machine is
# started again from the same disk, and must come up on that kernel,
# with the one it was built on beside it as /unix.old.
#
# PARTS is what is made, any of "stand kernel libc libs progs", and by
# default all of it: half a day of the machine's time.  The programs
# whose sources are themselves made, by yacc or lex or a program of
# their own, are not among them yet (tools/srcmk.py says which).
#
# TOOLS, TOOLS_KERNEL, TOOLS_PROGRAMS and TOOLS_BOOT are as in
# tests/system (tests/harness/system.sh) and say whose work the system
# that does the making is; what it makes is compared with the work of
# the system's own tools whichever.  With ROOTFS set, that root file
# system is booted in place of the one made here.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/self needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
src=$top/usr/src
. "$top/tests/harness/system.sh"
name=self$case
out=$top/obj/tests/$name
mkdir -p "$out"

if [ -z "$ROOTFS" ]; then
    buildsys
    ROOTFS=$top/obj/root/root.fs
else
    buildbase
fi
# What is to be made, as it is made here.
mk -C "$src/sys/PC" TOOLS=native
mk -C "$src/sys/i286stand" TOOLS=native
mk -C "$src/i286" syslibs
mk -C "$src/i286" TOOLS=native all
npc=$top/obj/nPC
nstand=$top/obj/nstand

# What of it is made, of: stand, /boot and the boot blocks; kernel; libc,
# the C library of each model; libs, the other libraries; progs, the
# programs.
PARTS=${PARTS:-"stand kernel libc libs progs"}
for p in $PARTS; do
    case $p in
    stand|kernel|libc|libs|progs) ;;
    *)  echo "PARTS is of stand, kernel, libc, libs and progs, not $p" >&2
        exit 2 ;;
    esac
done
has() {
    case " $PARTS " in *" $1 "*) return 0 ;; esac
    return 1
}

# Two file systems.  One is /usr/src: the sources and their Makefiles
# (tools/srcmk.py), and what is made is left there.  The other has
# each thing as it is made here, in the directory it is made in there,
# with check.sh, which compares a directory's, and self.sh, what the
# system is to do.  The kernel says the day it was made, and is told
# there to say this one's.
cd "$out"
cp "$ROOTFS" root.fs
rm -rf fs
mkdir -p fs/src
python3 -I "$top/tools/srcmk.py" fs/src
date=$(sed -n 's/.*(\(.*\)).*/\1/p' "$npc/vers.c")
nlib=$top/obj/nlib
libs=$(cat fs/src/libraries)
: > want.proto
dirs=" "
# want DIRECTORY FILE...: the files are to be made in that directory
# of /usr/src.
want() {
    d=$1
    shift
    p=
    for part in $(echo "$d" | tr / ' '); do
        p=$p/$part
        case $dirs in
        *" $p "*) ;;
        *)  echo "d $p 755" >> want.proto; dirs="$dirs$p " ;;
        esac
    done
    for f; do
        echo "f /$d/$(basename "$f") 644 $f" >> want.proto
    done
}
if has stand; then
    want sys/i286stand "$nstand"/*.bin "$nstand"/*.o "$nstand/boot"
fi
if has kernel; then
    want sys/PC $(make -s -C "$src/sys/PC" table | sed -n 's/^OBJS //p' |
        tr ' ' '\n' | sed "s|^|$npc/|") "$npc/unix"
fi
for m in s m l; do
    if has libc; then
        want lib/libc/i286/$m $(make -s -C "$src/lib/libc/i286" MODEL=$m \
            table | sed -n 's/^OBJS //p' | tr ' ' '\n' |
            sed "s|^|$nlib/$m/obj/|; s|\$|.o|") "$nlib/$m/crt0.o"
        want lib/libc/i286/$m/sys "$nlib/$m"/obj/sys/*.o
    fi
    if has libs; then
        for l in $libs; do
            want i286/lib/$m/$l "$nlib/$m/$l"/*.o
        done
    fi
done
if has progs; then
    want i286/bin $(sed "s|^|$top/obj/nbin/|" fs/src/programs)
fi
n=$(grep -c "^f " want.proto)
cat > fs/check.sh <<'EOF'
n=0
for f in `ls /mnt/$1`
do
	if test -f /mnt/$1/$f
	then
		if cmp -s /mnt/$1/$f /usr/src/$1/$f
		then
			n=`expr $n + 1`
		else
			echo "DIFFERS $1/$f"
		fi
	fi
done
echo "same $1 $n"
EOF
{
    echo "PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin"
    echo "export PATH"
    echo "DATE=$date"
    echo "export DATE"
    echo "date"
    if has stand; then
        echo "cd /usr/src/sys/i286stand && make -k"
        echo "sh /mnt/check.sh sys/i286stand"
    fi
    if has kernel; then
        echo "cd /usr/src/sys/PC && make -k"
        echo "sh /mnt/check.sh sys/PC"
    fi
    if has libc; then
        echo "for m in s m l"
        echo "do"
        echo "	cd /usr/src/lib/libc/i286/\$m && make -k"
        echo "	sh /mnt/check.sh lib/libc/i286/\$m"
        echo "	sh /mnt/check.sh lib/libc/i286/\$m/sys"
        echo "	make install"
        echo "	date"
        echo "done"
    fi
    if has libs; then
        echo "cd /usr/src/i286 && make -k libs"
        echo "for m in s m l"
        echo "do"
        echo "	for l in" $libs
        echo "	do"
        echo "		sh /mnt/check.sh i286/lib/\$m/\$l"
        echo "	done"
        echo "done"
        echo "make install-libs"
        echo "date"
    fi
    if has progs; then
        echo "cd /usr/src/i286 && make -k progs"
        echo "sh /mnt/check.sh i286/bin"
    fi
    echo "date"
    echo 'echo "self: made"'
} > fs/self.sh
cat > fs/install.sh <<'EOF'
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
DATE="made here"
export DATE
cd /usr/src/sys/i286stand && make install &&
cd /usr/src/sys/PC && make install &&
cmp /unix unix && cmp /boot ../i286stand/boot && echo "self: installed"
EOF
for f in check.sh self.sh install.sh; do
    echo "f /$f 644 fs/$f" >> want.proto
done
# The sources are older than anything the system makes of them,
# whatever its clock says.
python3 -I "$top/tools/mkfs.py" -s 52000 -t $(($(date +%s) - 172800)) \
    fs/src/proto src.fs >/dev/null
python3 -I "$top/tools/mkfs.py" want.proto want.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 1000,16,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" self.img a=root.fs \
    b=swap:8000 d=src.fs e=want.fs | tail -1)
install='sh /mnt/install.sh; '
has kernel && has stand || install=
TIMEOUT=${TIMEOUT:-60000} python3 -I "$top/tests/harness/runtest.py" \
    --hd self.img,$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'mkdir /usr/src; mount /dev/wd0d /usr/src; mount /dev/wd0e /mnt; sh /mnt/self.sh' \
    --type "self: made" "${install}umount /mnt; umount /usr/src" \
    --type "# " 'echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name

serial=$out/serial.txt
if grep -q "^DIFFERS " "$serial"; then
    grep "^DIFFERS " "$serial" >&2
    echo "$name: the system did not make those as they are made here" >&2
    exit 1
fi
got=$(tr -d '\r' < "$serial" | awk '/^same / { n += $3 } END { print n + 0 }')
if [ "$got" != "$n" ]; then
    echo "$name: $got of $n files made as they are made here" >&2
    exit 1
fi
if ! grep -q "halting" "$serial"; then
    echo "$name: the system did not come down" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" self.img
python3 -I "$top/tools/fsck.py" -p d self.img
if [ -z "$install" ]; then
    echo "$name: the system made $n files of itself ($PARTS), each" \
        "what is made of it here"
    exit 0
fi
if ! grep -q "^self: installed" "$serial"; then
    echo "$name: the kernel and /boot were not installed" >&2
    exit 1
fi

# Again, on what it made.
TIMEOUT=${TIMEOUT:-600} python3 -I "$top/tests/harness/runtest.py" \
    --hd "$out/self.img",$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'ls -l /unix /unix.old; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name-2
if ! grep -q "^2.11 BSD UNIX for the 286 (made here)" \
    "$top/obj/tests/$name-2/serial.txt"; then
    echo "$name: the system did not start on the kernel it made" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" self.img
echo "$name: the system made $n files of itself ($PARTS), each what is" \
    "made of it here, installed its kernel and /boot and started on them"
