#!/bin/sh
# Test that the system makes itself: boot it from a hard disk with its
# sources on a file system of their own, /usr/src, and have its make,
# compiler, assembler and linker make there: /boot and the boot blocks,
# from sys/i286stand's Makefile; the kernel, from the Makefile that its
# own config writes in sys/PC from sys/conf/PC, with the rest of what it
# writes there; and from the Makefile tools/srcmk.py writes, what it
# has that is not a program and is made: the databases and tables of
# /etc and /usr/share, spell's lists, the time zones, the games' files
# and the manual as it is read, which are installed.  Each object, the
# kernel and /boot, what config writes, and each of those files, is to
# be to the byte what the same sources make where the system is built,
# with the system's own tools as they are compiled for that machine
# (obj/nstand, obj/nPC, obj/nbin), which the disk has on another file
# system to compare with.  A page of the manual has the day it was
# formatted at its foot, and is compared without it if that is all that
# differs.  And each of the files, as installed, is to be what the
# system made.
#
# The kernel is then made once more, to say of itself that it was made
# there, and installed with /boot (make; make install); the machine is
# started again from the same disk, and must come up on that kernel,
# with the one it was built on beside it as /unix.old, and on the
# programs it made: init, getty, login, the shell.
#
# PARTS is what is made, any of "stand kernel data", and by default all
# of it.  With DATA, the files that are not programs are those of the
# kinds it names, of "etc roff learn spell zones games man", and none is
# installed.  The libraries and the programs are not made here any
# more: the system makes those from the Makefiles of their own
# directories, upstream's, and tests/self/progs.sh has it do that.
#
# TOOLS, TOOLS_KERNEL, TOOLS_PROGRAMS and TOOLS_BOOT are as in
# tests/system (tests/harness/system.sh) and say whose work the system
# that does the making is; what it makes is compared with the work of
# the system's own tools whichever.  With ROOTFS set, that root file
# system is booted in place of the one made here.
#
# With HOST set, no machine is booted: the same Makefiles are run where
# the system is built, in seconds, by this machine's make with the
# system's compiler, assembler and linker as they are compiled for it
# (tests/self/host.sh), and what they make compared the same way.  That
# says the Makefiles name the right files, flags and order; that the
# tools do on the 286 what they do here is the machine's to say, and so
# is config's work: it is not run here.
# PARTS is then of "stand kernel", both by default; for the libraries
# and the programs it is tests/make/run.sh.
#
# With SHARE set, the directory in obj/tests has that after its name:
# tests/self/split.sh boots two or three at once, each with a share.
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
name=self$case${HOST:+-host}${SHARE:+-$SHARE}
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

# What of it is made, of: stand, /boot and the boot blocks; kernel;
# data, what is not a program.
if [ -n "$HOST" ]; then
    PARTS=${PARTS:-"stand kernel"}
    case " $PARTS " in
    *" data "*)
        echo "HOST: data runs programs it makes, which are the 286's" >&2
        exit 2 ;;
    esac
fi
PARTS=${PARTS:-"stand kernel data"}
for p in $PARTS; do
    case $p in
    stand|kernel|data) ;;
    libc|libs|progs)
        echo "PARTS: the libraries and the programs are" \
            "tests/self/progs.sh's, and tests/make/run.sh's where the" \
            "system is built" >&2
        exit 2 ;;
    *)  echo "PARTS is of stand, kernel and data, not $p" >&2
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
# What config writes in sys/PC and sys/h, as the kernel's Makefile has
# had it written again here (obj/nPC/config).
if has kernel; then
    want sys/PC $(make -s -C "$src/sys/PC" table | sed -n 's/^OBJS //p' |
        tr ' ' '\n' | sed "s|^|$npc/|") "$npc/unix" "$npc"/config/PC/*
    want sys/h "$npc/config/h/localopts.h"
fi
if has progs; then
    for p in $ONLY; do
        grep -qx "$p" fs/src/programs || {
            echo "ONLY: $p is not a program the system makes" >&2
            exit 2
        }
    done
    want i286/bin $(echo ${ONLY:-$(cat fs/src/programs)} | tr ' ' '\n' |
        sed "s|^|$top/obj/nbin/|")
    # The files of ex's messages and tcsh's, which mkstr writes, and
    # of battlestar's, which its own does.
    case " ${ONLY:-ex} " in *" ex "*)
        want i286/ex "$top/obj/ex/exstrings" ;;
    esac
    case " ${ONLY:-tcsh} " in *" tcsh "*)
        want i286/tcsh "$top/obj/tcsh/tcsh6.00strgs" ;;
    esac
    case " ${ONLY:-battlestar} " in *" battlestar "*)
        want i286/games/battlestar \
            "$top/obj/games/battlestar/battle_strings" ;;
    esac
    if [ -z "$ONLY" ]; then
        # The programs as the system has them, and what compares them.
        echo "d /rootbin 755" >> want.proto
        echo "n=0" > fs/installed.sh
        while read p where; do
            grep -q "^f /rootbin/$p " want.proto ||
                echo "f /rootbin/$p 644 $top/obj/root/bin/$p" >> want.proto
            echo "if cmp -s /mnt/rootbin/$p '$where'; then n=\`expr \$n + 1\`;" \
                "else echo 'DIFFERS $where'; fi" >> fs/installed.sh
        done < fs/src/installed
        # And the files the programs read that are made with them.
        awk '$1 == "progs" { print "if cmp -s /usr/src/i286/" $2 " " $3 \
            "; then n=`expr $n + 1`; else echo \"DIFFERS " $3 "\"; fi" }' \
            fs/src/datainstalled >> fs/installed.sh
        echo 'echo "same installed $n"' >> fs/installed.sh
        echo "f /installed.sh 644 fs/installed.sh" >> want.proto
        ninst=$(($(wc -l < fs/src/installed) +
            $(grep -c '^progs ' fs/src/datainstalled)))
    fi
fi
# What is not a program: each file as it is made here, in the directory
# of obj it is in here, which is the one of /usr/src/i286 it is made in
# there; and, when all of it is made, what compares each as installed
# with what the system made.
datadirs=
if has data; then
    kinds=$(awk '{ print $1 }' fs/src/data | uniq)
    for k in $DATA; do
        echo "$kinds" | grep -qx "$k" || {
            echo "DATA is of" $kinds, "not $k" >&2
            exit 2
        }
    done
    awk -v kinds=" ${DATA:-$(echo $kinds)} " 'index(kinds, " " $1 " ") {
        print $2 }' fs/src/data > data.files
    datadirs=$(sed 's|/[^/]*$||' data.files | sort -u)
    for d in $datadirs; do
        want i286/$d
    done
    sed "s|.*|f /i286/& 644 $top/obj/&|" data.files >> want.proto
    if [ -z "$DATA" ]; then
        {
            echo "n=0"
            awk '$1 != "progs" { print "if cmp -s /usr/src/i286/" $2 " \047" \
                $3 "\047; then n=`expr $n + 1`; else echo \"DIFFERS " $3 \
                "\"; fi" }' fs/src/datainstalled
            echo 'echo "same data-installed $n"'
        } > fs/datainstalled.sh
        echo "f /datainstalled.sh 644 fs/datainstalled.sh" >> want.proto
        ninst=$((${ninst:-0} + $(grep -vc '^progs ' fs/src/datainstalled)))
    fi
fi
n=$(awk '$1 == "f" && $2 !~ /^\/rootbin\// && $2 !~ /installed.sh$/ { n++ }
    END { print n + 0 }' want.proto)
n=$((n + ${ninst:-0}))
# A page of the manual says at its foot the day it was formatted: two
# that differ are compared again without those lines.
cat > fs/check.sh <<'EOF'
n=0
t=/tmp/chk$$
d='s,Printed [0-9]*/[0-9]*/[0-9]*,Printed,'
for f in `ls /mnt/$1`
do
	if test -f /mnt/$1/$f
	then
		if cmp -s /mnt/$1/$f /usr/src/$1/$f
		then
			n=`expr $n + 1`
		elif grep 'Printed [0-9]' /mnt/$1/$f >/dev/null &&
		    sed "$d" /mnt/$1/$f >$t &&
		    sed "$d" /usr/src/$1/$f | cmp -s - $t
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
        echo "cd /usr/src/sys/conf && ./config PC && cd ../PC && make -k"
        echo "sh /mnt/check.sh sys/PC; sh /mnt/check.sh sys/h"
    fi
    if has progs; then
        targets=progs
        if [ -n "$ONLY" ]; then
            targets=$(echo $ONLY | tr ' ' '\n' | sed 's|^|bin/|' | tr '\n' ' ')
        fi
        echo "cd /usr/src/i286 && make -k $targets"
        echo "sh /mnt/check.sh i286/ex; sh /mnt/check.sh i286/tcsh"
        case " ${ONLY:-battlestar} " in *" battlestar "*)
            echo "sh /mnt/check.sh i286/games/battlestar" ;;
        esac
        echo "sh /mnt/check.sh i286/bin > /tmp/checked"
        echo "cat /tmp/checked"
        if [ -z "$ONLY" ]; then
            echo "if grep DIFFERS /tmp/checked >/dev/null"
            echo "then"
            echo "	echo 'self: programs not installed'"
            echo "else"
            echo "	make install-progs && sh /mnt/installed.sh"
            echo "fi"
        fi
    fi
    if has data; then
        echo "date"
        targets=data
        if [ -n "$DATA" ]; then
            targets=$(echo $DATA | tr ' ' '\n' | sed 's|^|data-|' | tr '\n' ' ')
        fi
        echo "cd /usr/src/i286 && make -k $targets"
        echo "for d in" $datadirs
        echo "do"
        echo "	sh /mnt/check.sh i286/\$d"
        echo "done > /tmp/checked"
        echo "cat /tmp/checked"
        if [ -z "$DATA" ]; then
            echo "if grep DIFFERS /tmp/checked >/dev/null"
            echo "then"
            echo "	echo 'self: data not installed'"
            echo "else"
            echo "	make install-data && sh /mnt/datainstalled.sh"
            echo "fi"
        fi
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
cd /usr/src/sys/PC && make && make install &&
cmp /unix unix && cmp /boot ../i286stand/boot && echo "self: installed"
EOF
for f in check.sh self.sh install.sh; do
    echo "f /$f 644 fs/$f" >> want.proto
done
if [ -n "$HOST" ]; then
    . "$top/tests/self/host.sh"
    exit 0
fi
# The sources are older than anything the system makes of them,
# whatever its clock says.
python3 -I "$top/tools/mkfs.py" -s 64000 -t $(($(date +%s) - 172800)) \
    fs/src/proto src.fs >/dev/null
python3 -I "$top/tools/mkfs.py" want.proto want.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 1000,16,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" self.img a=root.fs \
    b=swap:8000 d=src.fs e=want.fs | tail -1)
install='sh /mnt/install.sh; '
has kernel && has stand || install=
TIMEOUT=${TIMEOUT:-90000} python3 -I "$top/tests/harness/runtest.py" \
    --hd self.img,$geometry,xtide --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /usr/src; mount /dev/wd0e /mnt; sh /mnt/self.sh' \
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
