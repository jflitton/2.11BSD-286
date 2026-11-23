#!/bin/sh
# Test that the system makes its libraries, its programs, what they read
# and its manual from the Makefiles of /usr/src, upstream's in each
# directory, and installs them: boot it with its sources, and in each
# directory have it
#
#	make
#	make install
#
# with its own make, compiler, assembler and linker.  The root file
# system is then to be as it was before, which is what the other build
# makes (usr/src/i286's Makefile): each file that was installed with
# the contents, mode, owner and group the root had there, a library with
# its members, and all else as it was, nothing gone and nothing new
# (tests/self/progs.py); and each of the root's programs whose sources
# are in one of the directories is to have been installed, and each of
# its libraries, if the directory is theirs, and of its pages of the
# manual those whose sources are in one.  tests/make runs the
# same Makefiles where the system is built, and booting nothing: this
# says the system's make and shell read them as that machine's do, and
# that the tools do on the 286 what they do there.
#
#	sh tests/self/progs.sh [DIR ...]
#
# DIR is a directory of usr/src with a Makefile: lib/libc, bin,
# usr.bin/yacc, share, man.  Without one it is the C library, the other
# libraries (usr.lib), share, every directory of programs and man, in
# that order, as the programs are linked with the libraries and upstream's
# Makefile in /usr/src has the others; which is more than ten hours of
# the 286's time.  The machine has the faster processor unless CPU=286
# is in the environment, as tests/self/changed.sh has it.  With ROOTFS
# set, that root file system is the one booted, and none is made; with
# SHARE, the directory in obj/tests has that after its name
# (tests/self/split.sh).
#
# A page of the manual is compared without the day it was formatted,
# which it says; the contents of the files of tests/make/differs, which
# the two builds do not make the same way, are not compared, nor is what
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
dirs=${*:-"lib/libc usr.lib share bin sbin libexec ucb usr.bin usr.sbin games
    new man"}
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
# a file for each directory, and the end of it shown if it fails.  The
# files that are newer than the first of them are the ones installed.
cat > fs/make.sh <<EOF
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin
export PATH
date
date > /usr/src/made
sleep 2
for d in $dirs
do
	log=/usr/src/log.\`echo \$d | tr / .\`
	if (cd /usr/src/\$d && make && make install) > \$log 2>&1
	then
		echo "self: made \$d"
	else
		tail -20 \$log
		echo "self: FAILED \$d"
	fi
done
find /bin /sbin /lib /etc /usr/bin /usr/ucb /usr/sbin /usr/libexec \\
    /usr/games /usr/new /usr/share /usr/lib /usr/man /var/db \\
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
python3 -I "$top/tools/fsck.py" -l root.fs > before.list
python3 -I "$top/tools/fsck.py" -p a -l self.img > after.list
python3 -I "$top/tools/fsck.py" -p a -x /newer.list self.img > newer.list
python3 -I "$top/tests/self/progs.py" "$top" "$out" $name $dirs
