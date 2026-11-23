#!/bin/sh
# Run the system's own Makefiles, upstream's in each directory of
# usr/src, where the system is built, and compare what `make install`
# puts in the root with what the root file system has (obj/root/root.fs,
# which usr/src/i286's Makefile makes): each file's contents, and its
# mode.  No machine is booted.
#
#	sh tests/make/run.sh [DIR ...]
#
# DIR is a directory of usr/src with a Makefile: bin, usr.bin/spell.
# Without one it is every directory of programs.  The Makefiles are run
# by this machine's make in a copy of usr/src (obj/tests/make/root), with
# the rules the system's make has and none of its own (rules.mk), and
# with the system's commands as this machine has them (commands.sh): the
# system's compiler, assembler and linker compiled for it, and an
# install that puts things in obj/tests/make/dest.
#
# That says the Makefiles name the right files, flags and order, and
# install each where the system has it.  It does not say the system's
# make and shell read them as this machine's do, nor that the tools do
# on the 286 what they do here: tests/self has the machine do it.  A
# program a Makefile makes and then runs is the 286's and does not run
# here; what is made with one is listed as left to the 286.
#
# Run from the top of the repository after `. tools/env.sh`, with the
# system built (make -C usr/src/i286 root).
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/make
out=$top/obj/tests/make
dirs=${*:-"bin sbin libexec ucb usr.bin usr.sbin games new"}
mkdir -p "$out"
rm -rf "$out/root/usr/src" "$out/dest" "$out/log"
mkdir -p "$out/dest" "$out/log"
: > "$out/dest.meta"
python3 -I "$top/tools/fsck.py" -l "$top/obj/root/root.fs" > "$out/root.list"
awk '$2 == "d" { print $1 }' "$out/root.list" > "$out/dirs"
python3 -I "$here/stage.py" "$top" "$out"
sh "$here/commands.sh" "$top" "$out"
start=$(date +%s)
for d in $dirs; do
    log=$out/log/$(echo "$d" | tr / _)
    (cd "$out/root/usr/src/$d" && PATH="$out/bin:$PATH" &&
        MAKEFILES=$here/rules.mk && export PATH MAKEFILES &&
        for t in all install clean; do
            make -r -k SHELL="$out/bin/sh" $t
        done) > "$log" 2>&1 || true
done
python3 -I "$here/check.py" "$top" "$out" $(($(date +%s) - start)) $dirs
