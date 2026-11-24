#!/bin/sh
# Test that the system makes its libraries, its programs, what they read
# and its manual from the Makefiles of /usr/src, upstream's, and installs
# them: boot it with its sources and have it, with its own make,
# compiler, assembler and linker, do in /usr/src what 2.11BSD's Makefile
# there is for,
#
#	make build
#	make installsrc
#
# which makes the C library and installs it, makes the compiler with
# that library and installs it, makes both again with what it
# installed, makes the other libraries and installs them, and then
# makes the rest, share first and the manual last, and installs it.
# The root file system is then to be as it was before, which is what
# the same Makefiles made where the system is built (make -C usr/src,
# tools/build/build.sh): each file that was installed with the
# contents, mode, owner and group the root had there, a library with
# its members, and all else as it was, nothing gone and nothing new
# (tests/self/progs.py); and each file the Makefiles installed there is
# to have been installed.  Nor is make to have said that anything
# failed, which a Makefile that runs make in its directories may not
# stop for.  This says the system's make and shell read the Makefiles
# as that machine's do, and that the tools do on the 286 what they do
# there.
#
#	sh tests/self/progs.sh [DIR ...]
#
# With a DIR, a directory of usr/src with a Makefile, lib/libc, bin,
# usr.bin/yacc, share, man, it is instead
#
#	make
#	make install
#
# in each, in the order given, and what those directories' Makefiles
# installed where the system is built is to have been installed.
# All of it is more than ten hours of the 286's time.  The machine
# has the faster processor unless CPU=286 is in the environment, as
# tests/self/changed.sh has it.  With ROOTFS set, that root file system
# is the one booted, and none is made; with SHARE, the directory in
# obj/tests has that after its name (tests/self/split.sh).
#
# A page of the manual is compared without the day it was formatted,
# which it says; the contents of the files of tests/self/differs, which
# are not made the same way each time, are not compared, nor is what
# the system changes by running (tests/self/running).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/self needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
export CPU=${CPU:-486}
dirs=$*
name=self$case-progs${SHARE:+-$SHARE}
out=$top/obj/tests/$name
mkdir -p "$out"
if [ -z "$ROOTFS" ]; then
    buildsys
    ROOTFS=$top/obj/root/root.fs
else
    buildbase
fi

cd "$out"
cp "$ROOTFS" root.fs
rm -rf fs
mkdir -p fs/src
python3 -I "$top/tools/srcmk.py" fs/src
# What the system is to do.  What make says is kept with the sources,
# a file for each run of it, and the end of it shown if it fails.  The
# files that are newer than the first of them are the ones installed.
if [ -n "$dirs" ]; then
    runs=$dirs
else
    runs="build installsrc"
fi
cat > fs/make.sh <<EOF
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
date
date > /usr/src/made
sleep 2
for d in $runs
do
	log=/usr/src/log.\`echo \$d | tr / .\`
	case "$dirs" in
	"")	run="cd /usr/src && make \$d" ;;
	*)	run="cd /usr/src/\$d && make && make install" ;;
	esac
	if (eval \$run) > \$log 2>&1
	then
		echo "self: made \$d"
	else
		tail -20 \$log
		echo "self: FAILED \$d"
	fi
done
find /bin /sbin /lib /etc /usr/bin /usr/ucb /usr/sbin /usr/libexec \\
    /usr/games /usr/local /usr/new /usr/share /usr/lib /usr/man /var/db \\
    -newer /usr/src/made -type f -print > /newer.list
sync
date
EOF
echo "f /make.sh 644 fs/make.sh" > want.proto
# The sources are older than anything the system makes of them,
# whatever its clock says.
python3 -I "$top/tools/mkfs.py" -s 64000 -t $(($(date +%s) - 172800)) \
    fs/src/proto src.fs >/dev/null
python3 -I "$top/tools/mkfs.py" want.proto want.fs >/dev/null
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 1000,16,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" self.img a=root.fs \
    b=swap:8000 d=src.fs e=want.fs | tail -1)
TIMEOUT=${TIMEOUT:-90000} python3 -I "$top/tests/harness/runtest.py" \
    --hd self.img,$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /usr/src; mount /dev/wd0e /mnt; sh /mnt/make.sh; echo "self: do""ne"' \
    --type "self: done" 'umount /mnt; umount /usr/src; echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name

serial=$top/obj/tests/$name/serial.txt
if grep -q "^self: FAILED" "$serial"; then
    grep -B20 "^self: FAILED" "$serial" >&2
    echo "$name: make did not make those ($out/self.img, /usr/src/log.*)" >&2
    exit 1
fi
if ! grep -q "^self: done" "$serial"; then
    echo "$name: the system did not get to the end" >&2
    exit 1
fi
# What make said failed, though it went on: a line of its own for a
# command that failed, but not one it was told to pass over, and its
# last for a Makefile it could not go on with.
: > failed
for d in $runs; do
    python3 -I "$top/tools/fsck.py" -p d -x "/log.$(echo $d | tr / .)" \
        self.img | tr -d '\r' | grep -E '^\*\*\* |Stop\.$' |
        grep -v '(ignored)$' | sed "s|^|$d: |" >> failed || true
done
if [ -s failed ]; then
    cat failed >&2
    echo "$name: make said those failed ($out/self.img, /usr/src/log.*)" >&2
    exit 1
fi
python3 -I "$top/tools/fsck.py" -l root.fs > before.list
python3 -I "$top/tools/fsck.py" -p a -l self.img > after.list
python3 -I "$top/tools/fsck.py" -p a -x /newer.list self.img > newer.list
# All of it is every directory usr/src's Makefile names.
python3 -I "$top/tests/self/progs.py" "$top" "$out" $name ${dirs:-$(sed -n \
    's/^[LS][IR][BC]DIR=//p' "$top/usr/src/Makefile")}
