#!/bin/sh
# Test the programs that work on text and files, those for disks and
# file systems, and the games: boot the system from a hard disk to
# multi-user, with a second disk for disklabel and the rest to write on,
# log in on the first serial port as root, and have it run each of the
# checks in tests/programs/checks, which are on a file system of their
# own with the files they read.  A check is a few lines for the shell and
# what they must write, to the byte.  Then use what needs a terminal: tty
# and mesg, more, man, script, lock, tset and clear, and the games
# battlestar, hack, rogue, robots and hangman; and stop the system with
# shutdown.
# The kernel, /boot, the C library, the programs and the root file system
# are built as they would be for a real machine, as tests/system has
# them.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/programs needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
out=$top/obj/tests/programs
mkdir -p "$out"

quiet() { grep -v '^cc \|^as \|^wlib \|Warning!\|Note!\|^unix:' || true; }
make -s -C "$top/usr/src/sys/PC" | quiet
make -s -C "$top/usr/src/sys/i286stand" | quiet
make -s -C "$top/usr/src/lib/libc/i286" | quiet
make -s -C "$top/usr/src/i286" all | quiet
make -s -C "$top/usr/src/i286" root | quiet

# The file system of checks: each one's lines in /t, what they must write
# in /x, the files in /d, and check.sh, which runs them all and says of
# each whether it wrote that, and what it wrote if not.
cd "$out"
cp "$top/obj/root/root.fs" root.fs
rm -rf checks
python3 -I "$top/tests/harness/checks.py" "$top/tests/programs/checks" checks
# The kernel as tools/strip.py leaves it: the system's strip must make
# the same of /unix, which has its symbols for ps and the like.
python3 -I "$top/tools/strip.py" "$top/obj/PC/unix" checks/d/unix.stripped
# The sources of three of the manual's pages, of which the system's nroff
# must make what the build's made (tools/mkman.py).
cp "$top/usr/src/man/man1/ls.1" "$top/usr/src/bin/mkdir/mkdir.1" \
    "$top/usr/src/man/man1/csh.1" checks/d
# spell's words to stop at, of which the system's spellin must make the
# list the build made.
cp "$top/usr/src/usr.bin/spell/stop" checks/d
{
    echo "d /t 755"; echo "d /x 755"; echo "d /d 755"
    echo "f /check.sh 644 checks/check.sh"
    echo "f /names 644 checks/names"
    for f in checks/t/* checks/x/* checks/d/*; do
        echo "f /${f#checks/} 644 $f"
    done
} > checks.proto
python3 -I "$top/tools/mkfs.py" -s 800 checks.proto checks.fs >/dev/null

stand=$top/obj/stand
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -b "$stand/hdboot.bin" \
    -m "$stand/mbr.bin" programs.img a=root.fs b=swap:4000 d=checks.fs | tail -1)
# A second disk, of 3.4 MB, the AT disk controller's second drive, for
# disklabel to write the label of: in multi-user the kernel will not let
# it write the first disk's, whose partition a is the root.  Its label
# has room for a file system in a, and partition e unused.
spare=$(python3 -I "$top/tools/mkdisk.py" -p -g 100,4,17 spare.img a=fs:2000 \
    e=2000 | tail -1)

# What is typed waits for something only the system writes, as in
# tests/system: a typed line is echoed, so what it waits for is split by
# quotes where it is typed.  The shell that script starts is left with
# the end of the file: one that is at a terminal does not obey exit.
CTRLD=$(printf '\004')
CTRLC=$(printf '\003')
TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
    --hd programs.img,$geometry,xtide --hd2 spare.img,$spare --until halting \
    --type "login: " "root" \
    --type "# " 'mount /dev/wd0d /mnt; sh /mnt/check.sh; umount /mnt' \
    --type "checks: " 'tty; mesg; mesg n; mesg; mesg y' \
    --type "is n" 'more /etc/services' \
    --type "--More--" ' ' \
    --type "--More--" 'q' \
    --type "# " 'ls -l /etc | more' \
    --type "--More--" 'q' \
    --type "# " 'man ls' \
    --type "--More--" 'q' \
    --type "# " 'script /tmp/ts' \
    --type "# " 'echo "in-sc""ript"' \
    --type "# " "${CTRLD}" \
    --type "Script done" 'echo ts-`grep -c "^in-script" /tmp/ts`' \
    --type "# " 'lock' \
    --type "Key: " 'kk' \
    --type "Again: " 'kk' \
    --type "time now is" 'wrong' \
    --type "Key: " 'kk' \
    --type "# " 'echo "ts""et-"`tset - -Q`; clear | od -c | sed 1q' \
    --type "# " '/usr/games/battlestar' \
    --type ">-: " 'quit' \
    --type "# " '/usr/games/hack' \
    --type "xprncd player" "${CTRLD}" \
    --type "# " '/usr/games/rogue -s' \
    --type "-more-" ' ' \
    --type "-more-" ' ' \
    --type "-more-" ' ' \
    --type "-more-" ' ' \
    --type "Rogueists" ' ' \
    --type "-more-" ' ' \
    --type "# " '/usr/games/robots' \
    --type "Directions:" "${CTRLC}" \
    --type "# " '/usr/games/hangman' \
    --type "Guess:" "${CTRLC}" \
    --type "# " 'echo "@ex""it 00"; shutdown -h now going down' \
    --boot programs

serial=$out/serial.txt
if grep -q "^BAD " "$serial"; then
    tr -d '\r' < "$serial" | sed -n '/^BAD /,/^END /p' >&2
    echo "programs: those checks did not write what they must" >&2
    exit 1
fi
n=$(wc -l < checks/names | tr -d ' ')
if ! grep -q "^checks: $n ok, 0 bad" "$serial"; then
    echo "programs: not all $n checks ran" >&2
    exit 1
fi
for want in "^/dev/tty00" "^is y" "^is n" "--More--(.*%)" \
    "^     ls - list contents of directory" "^ts-1" \
    "Script started, file is /tmp/ts" "Script done, file is /tmp/ts" \
    "lock: /dev/tty00 on .* timeout in 15 minutes" "tset-vt100" \
    "^0000000  *033  *\[" "luxurious stateroom" "Your rating was" \
    "End f input" "cannot lock score file" "Rogueists" "Directions:" \
    "Word #:" "System going down IMMEDIATELY" "going down" \
    "System shutdown time has arrived" "halting"; do
    if ! grep -q -- "$want" "$serial"; then
        echo "programs: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" programs.img
python3 -I "$top/tools/fsck.py" -p a spare.img
python3 -I "$top/tools/fsck.py" -p e spare.img
echo "programs: $n checks of the programs that work on text and files, of nroff and man, of tbl, eqn, refer, spell and the rest documents go through, of disklabel, the file system checkers and fstat, and of the games, and tty, mesg, more, man, script, lock, tset, clear, five games and shutdown at a terminal"
