#!/bin/sh
# Test that the system makes its own compiler: boot it from a hard
# disk, as tests/programs does, and have it compile, assemble and link
# from their sources cc, /lib/cpp, the compiler's two passes and its
# object code improver, as, ld, ar, ranlib, nm and size (PROGS, below,
# which says what each is made of, as its Makefile has it).  Each is to
# be, to the byte, what the same tools make of it where the system is
# built (tools/cc -ccom), which the disk has to compare with.
#
# It does so twice.  The first time the tools at work are those of the
# root file system; they are then put where those were, and the second
# time the tools are what the system made of itself.
#
# The sources are on a file system of their own, where the objects and
# the programs are made too, laid out as in usr/src, and each program
# is made from the top of that with the names its sources have there:
# a file's name is in what the compiler says of it and nowhere else.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh): with TOOLS=native the system is the system's
# own tools' work throughout, and so are the tools it makes them with
# the first time.
# With ROOTFS set, that root file system is booted in place of the one
# made here.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/ccom/self.sh needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
src=$top/usr/src
. "$top/tests/harness/system.sh"
out=$top/obj/tests/ccom-self$case
mkdir -p "$out"

mk -C "$src/lib/ccom/i286" host
if [ -z "$ROOTFS" ]; then
    buildsys
    ROOTFS=$top/obj/root/root.fs
else
    buildbase
fi
mk -C "$src/lib/libc/i286" TOOLS=native

# Each program, and what cc is given to make it, its model if not the
# small, the flags its Makefile gives, and its sources, named from
# usr/src.
PROGS="cc bin/cc.c
cpp -DLIBVMF=0 -Ilib/cpp lib/cpp/cpp.c lib/cpp/cpc.c lib/cpp/token.c
c0 lib/ccom/c00.c lib/ccom/c01.c lib/ccom/c02.c lib/ccom/c03.c lib/ccom/c04.c lib/ccom/c05.c lib/ccom/ansi.c
c1 lib/ccom/i286/c10.c lib/ccom/i286/c11.c lib/ccom/i286/c12.c lib/ccom/i286/c13.c lib/ccom/i286/c14.c lib/ccom/i286/c15.c
c2 lib/c2/i286/c20.c lib/c2/i286/c21.c
c2l -ml lib/c2/i286/c20.c lib/c2/i286/c21.c
as bin/as/i286/as0.c bin/as/i286/as1.c bin/as/i286/as2.c bin/as/i286/as3.c
ld bin/ld/i286/ld.c
ar -Ibin/ar bin/ar/append.c bin/ar/ar.c bin/ar/archive.c bin/ar/contents.c bin/ar/delete.c bin/ar/extract.c bin/ar/misc.c bin/ar/move.c bin/ar/print.c bin/ar/replace.c bin/ar/strmode.c
ranlib -Ibin/ar bin/ar/archive.c usr.bin/ranlib/build.c usr.bin/ranlib/misc.c usr.bin/ranlib/ranlib.c usr.bin/ranlib/touch.c
nm -Ibin/ar bin/nm/nm.c
size bin/size.c"

# The file system: /progs has a line for each program, its name and
# what cc is given to make it; /src the sources those name, and the
# headers beside them; /want each program as it is made here; and
# /self.sh what the system is to do.
cd "$out"
cp "$ROOTFS" root.fs
rm -rf fs
mkdir fs fs/want
echo "$PROGS" > fs/progs
n=$(wc -l < fs/progs | tr -d ' ')
{
    echo "d /src 755"; echo "d /want 755"
    echo "f /progs 644 fs/progs"
    echo "f /self.sh 644 fs/self.sh"
} > fs.proto
files=
while read name args; do
    (cd "$src" && "$top/tools/cc" -ccom -O -o "$out/fs/want/$name" $args)
    echo "f /want/$name 755 fs/want/$name" >> fs.proto
    for a in $args; do
        case $a in
        -I*) files="$files $(cd "$src" && ls "${a#-I}"/*.h)" ;;
        -*) ;;
        *)  files="$files $a $(cd "$src" && ls "$(dirname "$a")"/*.h \
                2>/dev/null || true)" ;;
        esac
    done
done < fs/progs
made=" "
for f in $(echo $files | tr ' ' '\n' | sort -u); do
    p=
    for part in $(dirname "$f" | tr / ' '); do
        p=$p/$part
        case $made in
        *" $p "*) ;;
        *)  echo "d /src$p 755" >> fs.proto; made="$made$p " ;;
        esac
    done
    echo "f /src/$f 644 $src/$f" >> fs.proto
done
cat > fs/self.sh <<'EOF'
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
cd /mnt/src
for stage in 1 2
do
	mkdir /mnt/new$stage
	while read name args
	do
		rm -f *.o
		if cc -O -o /mnt/new$stage/$name $args >/tmp/log 2>&1 </dev/null &&
		    cmp /mnt/new$stage/$name /mnt/want/$name >>/tmp/log 2>&1
		then
			echo "same $stage $name"
		else
			echo "DIFFERS $stage $name"; cat /tmp/log; echo "END $name"
		fi
	done < /mnt/progs
	if test $stage = 1
	then
		cd /mnt/new1
		cp cc as ld ar nm size /bin
		cp ranlib /usr/bin
		cp cpp c0 c1 c2 c2l /lib
		cd /mnt/src
		echo "installed"
	fi
done
rm -f *.o
echo "self: done"
EOF
python3 -I "$top/tools/mkfs.py" -s 2400 fs.proto src.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" self.img a=root.fs b=swap:4000 d=src.fs | tail -1)
TIMEOUT=${TIMEOUT:-3600} python3 -I "$top/tests/harness/runtest.py" \
    --hd self.img,$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /mnt; sh /mnt/self.sh; umount /mnt' \
    --type "self: done" 'echo "@ex""it 00"; shutdown -h now going down' \
    --boot ccom-self$case

serial=$out/serial.txt
if grep -q "^DIFFERS " "$serial"; then
    tr -d '\r' < "$serial" | sed -n '/^DIFFERS /,/^END /p' >&2
    echo "ccom-self: the system did not make those as they are made here" >&2
    exit 1
fi
for stage in 1 2; do
    got=$(grep -c "^same $stage " "$serial" || true)
    if [ "$got" != "$n" ]; then
        echo "ccom-self: $got of $n programs made the $stage. time" >&2
        exit 1
    fi
done
for want in "^installed" "^self: done" "halting"; do
    if ! grep -q -- "$want" "$serial"; then
        echo "ccom-self: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" self.img
echo "ccom-self: the system made the $n programs of its compiler from" \
    "their sources, twice, the second time with those it had just made," \
    "and each is what is made of it here"
