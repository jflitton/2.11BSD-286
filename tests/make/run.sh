#!/bin/sh
# Run the system's own Makefiles, upstream's in each directory of
# usr/src, where the system is built, and compare what `make install`
# puts in the root with what the root file system has (obj/root/root.fs,
# which usr/src/i286's Makefile makes): each file's contents, mode,
# owner and group, and each of the root's directories still there with
# its own (check.py).  No machine is booted.
#
#	sh tests/make/run.sh [DIR ...]
#
# DIR is a directory of usr/src with a Makefile: lib/libc, bin,
# usr.bin/spell, sys/PC, where make, make install and make clean are
# run.  Without one it is usr/src's own, upstream's, with make build,
# make installsrc and make clean, as tests/self/progs.sh has the system
# do: the libraries, share, every directory of programs and man; and
# then /boot's and the kernel's, which is the Makefile config wrote
# (usr/src/sys/conf).  The Makefiles are run
# by this machine's make in a copy of usr/src (obj/tests/make/root), with
# the rules the system's make has and none of its own (rules.mk), and
# with the system's commands as this machine has them (commands.sh): the
# system's compiler, assembler and linker compiled for it, and an
# install that puts things in obj/tests/make/dest, where the root's
# directories are made first, and which make takes to be there as the
# system's make does.
#
# That says the Makefiles name the right files, flags and order, and
# install each where the system has it.  It does not say the system's
# make and shell read them as this machine's do, nor that the tools do
# on the 286 what they do here: tests/self has the machine do it.  A
# program a Makefile makes and then runs is the 286's and does not run
# here; what is made with one is listed as left to the 286.  A line
# that does no more than run one is passed over, and make goes on
# (sh.py).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
buildsys
here=$top/tests/make
out=$top/obj/tests/make
dirs=${*:-". sys/i286stand sys/PC"}
# The kernel says the day it was made, and is told the one the root's
# says (usr/src/sys/conf/newvers.sh).
DATE=$(sed -n 's/.*(\(.*\)).*/\1/p' "$top/obj/nPC/vers.c")
export DATE
mkdir -p "$out"
rm -rf "$out/root/usr/src" "$out/dest" "$out/log"
mkdir -p "$out/dest" "$out/log"
: > "$out/dest.meta"
python3 -I "$top/tools/fsck.py" -l "$top/obj/root/root.fs" > "$out/root.list"
# The root's directories are there before anything is installed.
python3 -I "$here/dest.py" "$out"
python3 -I "$here/stage.py" "$top" "$out"
sh "$here/commands.sh" "$top" "$out"
start=$(date +%s)
for d in $dirs; do
    if [ "$d" = . ]; then
        log=$out/log/build targets="build installsrc clean"
    else
        log=$out/log/$(echo "$d" | tr / _) targets='"" install clean'
    fi
    (cd "$out/root/usr/src/$d" && PATH="$out/bin:$PATH" &&
        MAKEFILES=$here/rules.mk && export PATH MAKEFILES &&
        eval "for t in $targets; do
            make -r -k SHELL=\"$out/bin/sh\" \$t
        done") > "$log" 2>&1 || true
done
# usr/src's Makefile makes each of the directories it names.
python3 -I "$here/check.py" "$top" "$out" $(($(date +%s) - start)) \
    $(for d in $dirs; do
        [ "$d" = . ] || { echo "$d"; continue; }
        sed -n 's/^[LS][IR][BC]DIR=//p' "$top/usr/src/Makefile"
    done)
