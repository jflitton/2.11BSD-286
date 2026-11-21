#!/bin/sh
# Measure the system: boot it from a hard disk to multi-user, log in on
# the first serial port as root and run bench(1), this port's benchmark
# (usr/src/i286/bench), while the harness times each of its tests from
# outside by the marks it writes and samples where the processor is
# (runtest.py --mark and --profile).  Then print the table of what each
# test took, and say where tools/kprof.py's account of where the time
# went is.  Unlike the other suites this one is run for its numbers: it
# fails only if the tools do, bench, the marks and the profile, or the
# other programs for looking at a running system that it tries on the
# way, tracebuf, sysctl and top.
#
# usage: tests/bench/run.sh [TEST ...]
#
# With tests named, bench runs those alone.  In the environment, MS is
# how many milliseconds of emulated time lie between samples on average,
# 5 if not set: 1 gives five times the samples and takes the host about
# four times as long.  BASE is the results.txt of an earlier run to
# compare with: copy the results.txt somewhere before a change, and give
# it here after.
#
# The system is the cross compiler's unless it is said otherwise:
# NATIVE_KERNEL=1 boots the kernel the system's own compiler, assembler
# and linker make (usr/src/sys/PC CCOM=1), as tests/system has it, and
# NATIVE_PROGRAMS=1 a root of the programs those make, with the C
# library and the others as they make them (usr/src/i286 CCOM=1, in
# obj/nbin), as tests/ccom/programs.sh has it.  The profile is read with
# the kernel and the programs that ran.  Each case has a directory of its
# own, so that one may be compared with another: obj/tests/bench, with
# -nk after it for the native kernel and -np for the native programs.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/bench needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)

quiet() { grep -v '^cc \|^as \|^wlib \|^ar \|Warning!\|Note!\|^unix:' || true; }
name=bench
make -s -C "$top/usr/src/sys/PC" | quiet
kernel=$top/obj/PC/unix
if [ -n "$NATIVE_KERNEL" ]; then
    make -s -C "$top/usr/src/sys/PC" CCOM=1 | quiet
    kernel=$top/obj/nPC/unix
    name=$name-nk
fi
make -s -C "$top/usr/src/sys/i286stand" | quiet
make -s -C "$top/usr/src/lib/libc/i286" | quiet
bin=$top/obj/bin
if [ -n "$NATIVE_PROGRAMS" ]; then
    make -s -C "$top/usr/src/lib/libc/i286" CCOM=1 | quiet
    make -s -C "$top/usr/src/i286" CCOM=1 all | quiet
    bin=$top/obj/nbin
    name=$name-np
else
    make -s -C "$top/usr/src/i286" all | quiet
fi
make -s -C "$top/usr/src/i286" O="$bin" root KERNEL="$kernel" | quiet
for f in "$kernel" "$bin/bench"; do
    if [ ! -f "$f" ]; then
        echo "bench: no $f" >&2
        exit 1
    fi
done

out=$top/obj/tests/$name
mkdir -p "$out"
cd "$out"
cp "$top/obj/root/root.fs" root.fs
stand=$top/obj/stand
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" bench.img a=root.fs b=swap:4000 | tail -1)

# What is typed waits for something only the system writes: a typed line
# is echoed, so what it waits for, and the mark, are split by quotes
# where they are typed.  The second bench has no marks and is not in the
# table: it shows that tests can be named and the work multiplied.
rm -f marks.txt profile.txt results.txt kprof.txt
TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
    --hd bench.img,$geometry,xtide --until halting \
    --mark '@@' --profile "$kernel:${MS:-5}" \
    --type "login: " "root" \
    --type "# " "bench -m $*; echo \"ben\"\"ch-\$?\"" \
    --type "bench-" 'bench -s 2 stat getpid; echo "sca""le-$?"' \
    --type "scale-" 'tracebuf; echo "trace""buf-$?"' \
    --type "tracebuf-" 'sysctl hw.machine hw.physmem kern.ostype machdep.console_device; sysctl -a | grep -c " = "; echo "sys""ctl-$?"' \
    --type "sysctl-" 'top -s 1' \
    --type "COMMAND" 'q' \
    --type "# " 'echo "@ex""it 00"; shutdown -h now going down' \
    --boot $name > run.out 2>&1 || { cat run.out; exit 1; }

serial=$out/serial.txt
for want in "^bench-0" "^scale-0" "^tracebuf-0" "^sysctl-0" \
    "^getpid  *[0-9.]*  *[0-9.]*  *[0-9.]*  *40000 " \
    "^stat  *[0-9.]*  *[0-9.]*  *[0-9.]*  *4000 " \
    "breadhit  *breadmiss  *bwrite" "^hw.machine = i286" \
    "^hw.physmem = [0-9]" "^kern.ostype = BSD" \
    "^machdep.console_device = " "load average:" "Tasks: *[0-9]* total" \
    "Cpu  : .* us, .* sy, .* id" "Mem  : .*K total" \
    "PID USER .* TEXT  DATA STACK .* COMMAND" " top" "halting"; do
    if ! grep -q -- "$want" "$serial"; then
        cat run.out
        echo "bench: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
python3 -I "$top/tests/bench/report.py" ${BASE:+-b "$BASE"} "$out" "$@"

# The profile: every part must have its samples, and the two tests whose
# time can be in one place only must have it there.
python3 -I "$top/tools/kprof.py" -b "$bin" -m marks.txt "$kernel" profile.txt > kprof.txt
for t in ${*:-sieve getpid}; do
    case $t in
    sieve)  want="bench: sieve" ;;
    getpid) want="kernel: syscall" ;;
    *)      want="" ;;
    esac
    if ! awk -v t="$t:" -v w="$want" '
        BEGIN { found = w == "" }
        $1 == t { part = 1; if ($2 + 0 >= 20) samples = 1; next }
        /^$/ { part = 0 }
        part && index($0, w) { found = 1 }
        END { exit !(samples && found) }' kprof.txt; then
        echo "bench: the profile has no account of $t; see $out/kprof.txt" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" bench.img
echo "bench: the table is $out/results.txt, and where the time went $out/kprof.txt"
