#!/bin/sh
# All of tests/self/run.sh, shared between two or three machines booted
# at once, and never more: each is a processor of this one's, kept busy
# for as long as it runs.
#
#	sh tests/self/split.sh [-n] [2|3]
#
# Three if it is not said.  The shares are fixed, so that a machine
# always has the same work:
#
#	of three	1  /boot, the kernel, the libraries, a quarter
#			   of the programs
#			2  half of the programs
#			3  what is not a program, a quarter of the
#			   programs
#	of two		1  /boot, the kernel, the libraries, what is
#			   not a program
#			2  the programs
#
# A program's share is by its place in the list of them
# (tools/srcmk.py), not by its size.  Machines 2 and 3 link with the
# libraries the system came with, which are what machine 1 makes, to
# the byte, when it passes.  Left to run.sh with everything in one
# machine: that the programs are installed where the system has them,
# and that it starts on the ones it made.  With -n this says each
# machine's share and boots none.
#
# The machines have the faster processor (CPU=486,
# tests/harness/runtest.py) unless CPU says the 286.
#
# Each machine's output is in obj/tests/self$case-split/N.log, and the
# machine in obj/tests/self$case-N.
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
src=$top/usr/src
. "$top/tests/harness/system.sh"
out=$top/obj/tests/self$case-split
mkdir -p "$out"

# Everything is made here once, before the machines start, so that none
# of them makes anything another is reading.
if [ -z "$dry" ]; then
    buildsys
    mk -C "$src/i286" syslibs
    cp "$top/obj/root/root.fs" "$out/root.fs"
fi
rm -rf "$out/src"
python3 -I "$top/tools/srcmk.py" "$out/src"
# quarter N...: the programs of those quarters, by place in the list.
quarter() {
    awk -v q=" $* " 'index(q, " " (NR - 1) % 4 + 1 " ") { printf "%s ", $1 }' \
        "$out/src/programs"
}
if [ $n = 3 ]; then
    parts1="stand kernel libc libs progs" only1=$(quarter 1)
    parts2="progs" only2=$(quarter 2 3)
    parts3="data progs" only3=$(quarter 4)
else
    parts1="stand kernel libc libs data" only1=
    parts2="progs" only2=$(quarter 1 2 3 4)
fi
pids=
i=0
while [ $i -lt $n ]; do
    i=$((i + 1))
    eval "parts=\$parts$i only=\$only$i"
    echo "split: machine $i: $parts${only:+, $(echo $only | wc -w | tr -d ' ') programs}"
    [ -z "$dry" ] || continue
    (
        export SHARE=$i PARTS="$parts" ROOTFS="$out/root.fs"
        [ -z "$only" ] || export ONLY="$only"
        start=$(date +%s)
        if sh "$top/tests/self/run.sh" > "$out/$i.log" 2>&1; then
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
