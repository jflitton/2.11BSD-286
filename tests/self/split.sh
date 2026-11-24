#!/bin/sh
# All of tests/self/run.sh, and all that tests/self/progs.sh has the
# system make with make build, made a directory at a time, shared
# between two or three machines booted at once, and never more: each
# is a processor of this one's, kept busy for as long as it runs.
#
#	sh tests/self/split.sh [-n] [2|3]
#
# Three if it is not said.  The shares are fixed, so that a machine
# always has the same work:
#
#	of three	1  /boot and the kernel (run.sh), and then
#			   share and the manual (progs.sh)
#			2  the compiler and the libraries, and the programs
#			   of bin, sbin, libexec and ucb
#			3  those of usr.bin, usr.sbin, games, local and
#			   new
#	of two		1  as of three
#			2  the compiler, the libraries and the programs
#
# Machine 2 makes its programs with the compiler and the libraries it
# made and installed, and machine 3 with those the system came with,
# which are those, byte for byte and member for member, when machine 2
# passes.  make build makes the compiler twice, the second time with
# itself, and all in one machine, in the order of usr/src's Makefile:
# progs.sh does that alone.  With -n this says each machine's share and
# boots none.
#
# The machines have the faster processor (CPU=486,
# tests/harness/runtest.py) unless CPU says the 286.
#
# Each machine's output is in obj/tests/self$case-split/N.log, and the
# machine in obj/tests/self$case-N, or self$case-progs-N.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/self needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
dry=
if [ "$1" = -n ]; then
    dry=1
    shift
fi
n=${1:-3}
export CPU=${CPU:-486}
case $n in
2|3) ;;
*)  echo "split: two machines or three, not $n" >&2; exit 2 ;;
esac
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
out=$top/obj/tests/self$case-split
mkdir -p "$out"

# Everything is made here once, before the machines start, so that none
# of them makes anything another is reading.
if [ -z "$dry" ]; then
    buildsys
    cp "$top/obj/root/root.fs" "$out/root.fs"
fi
all="stand kernel"
dirs1="share man"
if [ $n = 3 ]; then
    dirs2="lib usr.lib bin sbin libexec ucb"
    dirs3="usr.bin usr.sbin games local new"
else
    dirs2="lib usr.lib bin sbin libexec ucb usr.bin usr.sbin"
    dirs2="$dirs2 games local new"
fi
pids=
i=0
while [ $i -lt $n ]; do
    i=$((i + 1))
    eval "dirs=\$dirs$i"
    if [ $i = 1 ]; then
        echo "split: machine 1: $all, then $dirs"
    else
        echo "split: machine $i: $dirs"
    fi
    [ -z "$dry" ] || continue
    (
        export SHARE=$i ROOTFS="$out/root.fs"
        start=$(date +%s)
        if { { [ $i != 1 ] || PARTS=$all sh "$top/tests/self/run.sh"; } &&
            sh "$top/tests/self/progs.sh" $dirs; } > "$out/$i.log" 2>&1; then
            echo "PASS machine $i $(($(date +%s) - start)) s"
        else
            echo "FAIL machine $i $(($(date +%s) - start)) s: $out/$i.log"
            exit 1
        fi
    ) &
    pids="$pids $!"
done
[ -z "$dry" ] || exit 0
failed=0
for pid in $pids; do
    wait $pid || failed=1
done
if [ $failed != 0 ]; then
    echo "self$case: a share of the system was not made as it is here" >&2
    exit 1
fi
echo "self$case: the system made itself, a share to each of $n machines"
