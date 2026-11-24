#!/bin/sh
# tests/self for a change, and no more of it than the change calls for.
#
#	sh tests/self/changed.sh [-n] [REF]
#
# The change is what differs from REF, main if it is not given: what is
# committed since, what is not committed, and files git has not been
# told of.  First the system is made here, with no machine: the kernel
# and /boot (HOST, tests/self/host.sh), and the rest with its Makefiles
# (tests/make), which says what each compiles.  Then one machine is
# booted to make only what the changed files are part of
# (tests/self/select.py says what, and why), or none if they are part of
# nothing it makes.  With -n it says what it would do, and does not.
#
# The machine has the faster processor (CPU=486, tests/harness/runtest.py)
# unless CPU says the 286: what is made is the same, in less than half
# the time.  All of it, make build in /usr/src (tests/self/progs.sh)
# and tests/self/run.sh, half a day of the 286's time, is for a
# release, not for a change.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
top=$(cd "$(dirname "$0")/../.." && pwd)
cd "$top"
dry=
if [ "$1" = -n ]; then
    dry=1
    shift
fi
ref=${1:-main}
export CPU=${CPU:-486}
names=$( { git diff --name-only "$(git merge-base "$ref" HEAD)"
    git ls-files -o --exclude-standard; } | sort -u)
if [ -z "$names" ]; then
    echo "changed: nothing differs from $ref"
    exit 0
fi
if [ -z "$dry" ]; then
    HOST=1 sh tests/self/run.sh
    sh tests/make/run.sh
fi
eval "$(echo "$names" | python3 -I tests/self/select.py)"
if [ -z "$PARTS" ] && [ -z "$DIRS" ] && [ -z "$CCOM" ]; then
    echo "changed: nothing the machine makes is made of what changed"
    exit 0
fi
echo "changed: on the machine: ${CCOM:+its compiler (tests/ccom/self.sh);}" \
    "${PARTS:+PARTS=\"$PARTS\"}${DIRS:+ DIRS=\"$DIRS\"}"
echo "changed: because of $WHY"
[ -z "$dry" ] || exit 0
if [ -n "$CCOM" ]; then
    sh tests/ccom/self.sh
fi
# What is made from the Makefiles of usr/src, in the directories that
# make what changed (tests/self/progs.sh), and /boot and the kernel
# (tests/self/run.sh).
if [ -n "$DIRS" ]; then
    sh tests/self/progs.sh $DIRS
fi
if [ -n "$PARTS" ]; then
    export PARTS
    sh tests/self/run.sh
fi
