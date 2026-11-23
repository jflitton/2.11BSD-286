#!/bin/sh
# tests/self for a change, and no more of it than the change calls for.
#
#	sh tests/self/changed.sh [-n] [REF]
#
# The change is what differs from REF, main if it is not given: what is
# committed since, what is not committed, and files git has not been
# told of.  First the system's Makefiles are run here, with no machine
# (HOST, tests/self/host.sh): all of them, in a couple of minutes.  Then
# one machine is booted to make only what the changed files are part
# of (tests/self/select.py says what, and why), or none if they are
# part of nothing it makes.  With -n it says what it would do, and
# does not.
#
# The machine has the faster processor (CPU=486, tests/harness/runtest.py)
# unless CPU says the 286: what is made is the same, in less than half
# the time.  The whole of tests/self/run.sh on the 286, half a day of
# its time, is for a release, not for a change.
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
. tests/harness/system.sh
mf=$top/obj/tests/self$case-host/fs/src/i286/Makefile
if [ ! -f "$mf" ]; then
    echo "changed: run without -n once, for $mf" >&2
    exit 2
fi
eval "$(echo "$names" | python3 -I tests/self/select.py "$mf")"
if [ -z "$PARTS" ] && [ -z "$CCOM" ]; then
    echo "changed: nothing the machine makes is made of what changed"
    exit 0
fi
echo "changed: on the machine: ${CCOM:+its compiler (tests/ccom/self.sh);}" \
    "${PARTS:+PARTS=\"$PARTS\"}${ONLY:+ ONLY=\"$ONLY\"}"
echo "changed: because of $WHY"
[ -z "$dry" ] || exit 0
if [ -n "$CCOM" ]; then
    sh tests/ccom/self.sh
fi
# The libraries, share, the programs and the manual are made from the
# Makefiles of their own directories (tests/self/progs.sh), in the
# order of upstream's Makefile in /usr/src, and /boot and the kernel by
# tests/self/run.sh.
dirs=
case " $PARTS " in *" libc "*) dirs="lib/libc" ;; esac
case " $PARTS " in *" libs "*) dirs="$dirs usr.lib" ;; esac
case " $PARTS " in *" share "*) dirs="$dirs share" ;; esac
case " $PARTS " in
*" progs "*) dirs="$dirs $(python3 -I tests/make/programs.py $ONLY)" ;;
esac
case " $PARTS " in *" man "*) dirs="$dirs man" ;; esac
PARTS=$(echo " $PARTS " |
    sed 's/ libc / /; s/ libs / /; s/ share / /; s/ progs / /; s/ man / /')
if [ -n "$(echo $dirs)" ]; then
    sh tests/self/progs.sh $dirs
fi
if [ -n "$(echo $PARTS)" ]; then
    export PARTS
    sh tests/self/run.sh
fi
