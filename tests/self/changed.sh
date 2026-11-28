#!/bin/sh
# tests/self for a change, and no more of it than the change calls for.
#
#	sh tests/self/changed.sh [-n] [REF]
#
# The change is what differs from REF, main if it is not given: what is
# committed since, what is not committed, and files git has not been
# told of.  First the system is made here, with no machine: the kernel
# and /boot (HOST, tests/self/host.sh), and the rest with its Makefiles
# (tests/make), which says what each compiles.  Then machines are booted
# to make only what the changed files are part of (tests/self/select.py
# says what, and why), or none if they are part of nothing they make.
# With -n it says what it would do, and does not.
#
# The machines are three at once at most, each a processor of this
# one's kept busy (tests/self/split.sh): the first makes the compiler
# (tests/ccom/self.sh) and /boot and the kernel (tests/self/run.sh), and
# the others the directories that make what changed (tests/self/progs.sh),
# dealt out the one with the most files first, each to the machine with
# the fewest so far, and made on it in the order select.py gives.  A
# machine makes with the compiler and libraries its root has, which
# are those another makes when that one passes.  Each boots the root
# tests/make made, and nothing is made here while they run.  What each
# says is in obj/tests/changed/N.log.
#
# The machines have the faster processor (CPU=486,
# tests/harness/runtest.py) unless CPU says the 286: what is made is the
# same, in less than half the time.  All of it, make build in /usr/src
# (tests/self/progs.sh) and tests/self/run.sh, half a day of the 286's
# time, is for a release, not for a change.
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
echo "changed: because of $WHY"

# The directories, a line of them for each machine that has some.
shares=$(python3 -I - "$top" "$CCOM$PARTS" $DIRS <<'END'
import os, sys
top, first, dirs = sys.argv[1], sys.argv[2], sys.argv[3:]
n = 2 if first else 3
def files(d):
    return sum(len(f) for _, _, f in os.walk(os.path.join(top, "usr/src", d)))
load, got = [0] * n, [[] for _ in range(n)]
for d in sorted(dirs, key=files, reverse=True):
    i = load.index(min(load))
    got[i].append(d)
    load[i] += files(d)
for g in got:
    if g:
        print(" ".join(d for d in dirs if d in g))
END
)

out=$top/obj/tests/changed
if [ -z "$dry" ]; then
    mkdir -p "$out"
    rm -f "$out"/*.log
    cp obj/root/root.fs "$out/root.fs"
    export ROOTFS=$out/root.fs
fi
pids=
# machine N WHAT COMMAND...: machine N, which makes WHAT, by COMMAND,
# started.
machine() {
    n=$1 what=$2
    shift 2
    echo "changed: machine $n: $what"
    [ -z "$dry" ] || return 0
    (
        start=$(date +%s)
        if "$@" > "$out/$n.log" 2>&1; then
            echo "PASS machine $n, $(($(date +%s) - start)) s"
            grep ': the system made' "$out/$n.log" || true
        else
            echo "FAIL machine $n, $(($(date +%s) - start)) s: $out/$n.log"
            tail -15 "$out/$n.log"
            exit 1
        fi
    ) &
    pids="$pids $!"
}
first() {
    if [ -n "$CCOM" ]; then
        sh tests/ccom/self.sh || return 1
    fi
    if [ -n "$PARTS" ]; then
        PARTS=$PARTS sh tests/self/run.sh
    fi
}
i=0
if [ -n "$CCOM$PARTS" ]; then
    i=1
    machine 1 "${CCOM:+the compiler}${CCOM:+${PARTS:+, then }}$PARTS" first
fi
while read -r dirs; do
    [ -n "$dirs" ] || continue
    i=$((i + 1))
    machine $i "$dirs" env SHARE=$i sh tests/self/progs.sh $dirs
done <<EOF
$shares
EOF
[ -z "$dry" ] || exit 0
failed=0
for pid in $pids; do
    wait $pid || failed=1
done
if [ $failed != 0 ]; then
    echo "changed: the system did not make all of that as it is made here" >&2
    exit 1
fi
echo "changed: the system made what the change is part of, on $i machines"
