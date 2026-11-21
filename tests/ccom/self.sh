#!/bin/sh
# Test that the system makes its own compiler: boot it from a hard
# disk, as tests/programs does, and have it compile, assemble and link
# from their sources cc, /lib/cpp, the compiler's two passes and its
# object code improver, as, ld, ar, ranlib, nm and size (CCPROGS in
# usr/src/i286/Makefile, which says what each is made of).  Each is to
# be, to the byte, what the same tools make of it where the system is
# built (tools/cc -ccom), which the disk has to compare with.
#
# It does so twice.  The first time the tools at work are those of the
# root file system, most of them the cross compiler's work; they are
# then put where those were, and the second time the tools are what the
# system made of itself.
#
# The sources are on a file system of their own, where the objects and
# the programs are made too, laid out as in usr/src, and each program
# is made from the top of that with the names its sources have there:
# a file's name is in what the compiler says of it and nowhere else.
#
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
out=$top/obj/tests/ccom-self
mkdir -p "$out"

quiet() { grep -v '^cc \|^as \|^wlib \|^ar \|Warning!\|Note!\|^unix:' || true; }
make -s -C "$src/lib/ccom/i286" host
make -s -C "$src/sys/PC" | quiet
make -s -C "$src/sys/i286stand" | quiet
make -s -C "$src/lib/libc/i286" | quiet
if [ -z "$ROOTFS" ]; then
    make -s -C "$src/i286" all | quiet
    make -s -C "$src/i286" root | quiet
    ROOTFS=$top/obj/root/root.fs
else
    make -s -C "$src/i286" syslibs | quiet
fi

# The file system: /progs has a line for each program, its name and
# what cc is given to make it; /src the sources those name, and the
# headers beside them; /want each program as it is made here; and
# /self.sh what the system is to do.
cd "$out"
cp "$ROOTFS" root.fs
rm -rf fs
mkdir fs fs/want
make -s -C "$src/i286" ccprogs > fs/progs
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
		cp cpp c0 c1 c2 /lib
		cd /mnt/src
		echo "installed"
	fi
done
rm -f *.o
echo "self: done"
EOF
python3 -I "$top/tools/mkfs.py" -s 2400 fs.proto src.fs >/dev/null

stand=$top/obj/stand
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" self.img a=root.fs b=swap:4000 d=src.fs | tail -1)
TIMEOUT=${TIMEOUT:-3600} python3 -I "$top/tests/harness/runtest.py" \
    --hd self.img,$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /mnt; sh /mnt/self.sh; umount /mnt' \
    --type "self: done" 'echo "@ex""it 00"; shutdown -h now going down' \
    --boot ccom-self

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
