#!/bin/sh
# Test the programs that work on text and files, those for disks and
# file systems, and the games: boot the system from a hard disk to
# multi-user, with a second disk for disklabel and the rest to write on
# and a blank diskette in the second drive for mtools, log in on the
# first serial port as root, and have it run each of the checks in
# tests/programs/checks, which are on a file system of their own with
# the files they read.  A check is a few lines for the shell and what
# they must write, to the byte.  Then use what needs a terminal: tty
# and mesg, more, man, script, lock, tset and clear, the games
# battlestar, hack, backgammon, rogue, robots and hangman, window and
# ve, less, jove, tip, qterm and tk; and stop the system with shutdown.
# The file mtools left on the diskette is read from its image here, as
# MS-DOS would read it.
# The kernel, /boot, the C library, the programs and the root file system
# are built as they would be for a real machine, as tests/system has
# them.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).  With ROOTFS set, that root file system is
# booted in place of the one made here, with the kernel that is /unix
# in it given as KERNEL, or the one the switches choose.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/programs needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
out=$top/obj/tests/programs$case
mkdir -p "$out"
if [ -z "$ROOTFS" ]; then
    buildsys
    ROOTFS=$top/obj/root/root.fs
else
    buildbase
    kernel=${KERNEL:-$kernel}
fi

# The file system of checks: each one's lines in /t, what they must write
# in /x, the files in /d, and check.sh, which runs them all and says of
# each whether it wrote that, and what it wrote if not.
cd "$out"
cp "$ROOTFS" root.fs
rm -rf checks
python3 -I "$top/tests/harness/checks.py" "$top/tests/programs/checks" checks
# The kernel as tools/strip.py leaves it: the system's strip must make
# the same of /unix, which has its symbols for ps and the like.
python3 -I "$top/tools/strip.py" "$kernel" checks/d/unix.stripped
# The sources of three of the manual's pages, of which the system's nroff
# must make what the build's made (tools/build/nroff.py).
cp "$top/usr/src/man/man1/ls.1" "$top/usr/src/bin/mkdir/mkdir.1" \
    "$top/usr/src/man/man1/csh.1" checks/d
# spell's words to stop at, of which the system's spellin must make the
# list the build made.
cp "$top/usr/src/usr.bin/spell/stop" checks/d
# The tables of time zones, of which the system's zic must make the
# files the build made.
(cd "$top/usr/src/share/zoneinfo" && cp asia australasia europe etcetera \
    northamerica pacificnew systemv "$out/checks/d")
{
    echo "d /t 755"; echo "d /x 755"; echo "d /d 755"
    echo "f /check.sh 644 checks/check.sh"
    echo "f /names 644 checks/names"
    for f in checks/t/* checks/x/* checks/d/*; do
        echo "f /${f#checks/} 644 $f"
    done
} > checks.proto
python3 -I "$top/tools/mkfs.py" -s 800 checks.proto checks.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" programs.img a=root.fs b=swap:4000 d=checks.fs | tail -1)
# A second disk, of 3.4 MB, the AT disk controller's second drive, for
# disklabel to write the label of: in multi-user the kernel will not let
# it write the first disk's, whose partition a is the root.  Its label
# has room for a file system in a, and partition e unused.
spare=$(python3 -I "$top/tools/mkdisk.py" -p -g 100,4,17 spare.img a=fs:2000 \
    e=2000 | tail -1)
# A diskette of 1.44 MB, blank, which mformat makes an MS-DOS one.
python3 -I -c 'open("dos.img", "wb").write(bytes(1474560))'

# What is typed waits for something only the system writes, as in
# tests/system: a typed line is echoed, so what it waits for is split by
# quotes where it is typed.  The shell that script starts is left with
# the end of the file: one that is at a terminal does not obey exit.
# window draws the space after a prompt as a move of the cursor, so what
# is typed in it waits for its own message instead.  It is run on a
# terminal of 132 columns by 50 lines, which it had no memory for before
# it was a program of the large model.  The q, the y and the end of the
# line reach it together or apart, as it happens, and it asks "Really
# quit [yn]?" on the screen only if the y was not there yet: what is
# waited for is the shell's prompt after it.  ve, on the same terminal,
# is given a line to add at the end of a file, and says what it read
# and wrote.  less, whose prompt says how far into the file it is, finds
# a line and shows its help, which is another less that the shell finds
# in /usr/local; the Return after the q that ends the help answers the
# first less's "End of help".  jove, for which a Return would be a new
# line, is given its text and then the name to write it to, which a
# Return ends, and the Return after the command that leaves it is the
# shell's.  tip takes the second serial port, which nothing is on, and
# its escape ends that.  qterm asks the terminal what it is, which this
# one does not answer, and takes its word from TERM.  tk, which pages for
# a Tektronix 4014, waits at the end for a Return from the terminal.
CTRLD=$(printf '\004')
CTRLC=$(printf '\003')
CTRLP=$(printf '\020')
CTRLW=$(printf '\027')
CTRLX=$(printf '\030')
ESC=$(printf '\033')
TIMEOUT=${TIMEOUT:-2400} python3 -I "$top/tests/harness/runtest.py" \
    --hd programs.img,$geometry,xtide --hd2 spare.img,$spare --fdb dos.img \
    --until halting \
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
    --type "# " '/usr/games/backgammon' \
    --type "rules of the game?" 'n' \
    --type "this program?" 'n' \
    --type "play both:" "${CTRLC}" \
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
    --type "# " 'stty rows 50 columns 132; window' \
    --type "Escape character is" 'echo "in-win""dow"; tty' \
    --type "in-window" "${CTRLP}" \
    --type "Command:" 'qy' \
    --type "# " 'cp /etc/services /tmp/sv; /usr/local/ve /tmp/sv' \
    --type "lines, " "Gototal ${ESC}:f" \
    --type "changed, line" ':w' \
    --type "lines, " ':q' \
    --type "# " 'echo "ve""-"`tail -1 /tmp/sv`' \
    --type "# " 'PATH=$PATH:/usr/local; less -m /etc/services' \
    --type "/etc/services (" '/^timed' \
    --type "525/udp" 'h' \
    --type "q when done" 'q' \
    --type "End of help" 'q' \
    --type "# " '/usr/new/jove /tmp/j' \
    --type "JOVE" "jove-text${CTRLX}${CTRLW}/tmp/j" \
    --type "characters." "${CTRLX}${CTRLC}" \
    --type "# " 'echo "jo""ve-"`cat /tmp/j`' \
    --type "# " 'tip com2' \
    --type "connected" '~.' \
    --type "EOT]" 'echo "ti""p-done"' \
    --type "# " '/usr/local/qterm; echo "qte""rm-$?"' \
    --type "# " 'echo "tk""-go"; tk /etc/motd > /tmp/tk.out; echo "tk-$? `wc -c < /tmp/tk.out`"' \
    --type "tk-go" '' \
    --type "# " 'echo "@ex""it 00"; shutdown -h now going down' \
    --boot programs$case

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
    "End f input" "cannot lock score file" \
    "Do you need instructions for this program?" "play both:" \
    "Rogueists" "Directions:" \
    "Word #:" "in-window" "/dev/ttyp[0-9]" "changed, line" "^ve-total" \
    "/etc/services ([0-9]*%)" "525/udp" "End of help" "^jove-jove-text" \
    "^.connected" "EOT]" "^tip-done" \
    'Terminal NOT recognized - defaults to "vt100"' "^qterm-0" \
    "^tk-0  *[1-9]" \
    "System going down IMMEDIATELY" "going down" \
    "System shutdown time has arrived" "halting"; do
    if ! grep -q -- "$want" "$serial"; then
        echo "programs: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" programs.img
# The diskette, by its boot sector's numbers: the root directory must
# have WORDS.TXT, and its chain of clusters in the first FAT the words.
python3 -I - dos.img "$out/checks/d/words" <<'EOF2'
import struct, sys
d = open(sys.argv[1], "rb").read()
bps, spc, rsv, nfat, nroot, tot, media, spf = struct.unpack_from("<HBHBHHBH", d, 11)
root = (rsv + nfat * spf) * bps
data = root + nroot * 32
fat = d[rsv * bps:(rsv + spf) * bps]
for i in range(nroot):
    e = d[root + 32 * i:root + 32 * i + 32]
    if e[:11] == b"WORDS   TXT":
        c, size = struct.unpack_from("<HI", e, 26)
        got = b""
        while 2 <= c < 0xff8:
            got += d[data + (c - 2) * spc * bps:data + (c - 1) * spc * bps]
            v = struct.unpack_from("<H", fat, c * 3 // 2)[0]
            c = v >> 4 if c & 1 else v & 0xfff
        if got[:size] == open(sys.argv[2], "rb").read():
            sys.exit(0)
sys.exit("programs: the diskette has not WORDS.TXT as mtools wrote it")
EOF2
python3 -I "$top/tools/fsck.py" -p a spare.img
python3 -I "$top/tools/fsck.py" -p e spare.img
echo "programs: $n checks of the programs that work on text and files, of nroff and man, of tbl, eqn, refer, spell and the rest documents go through, of zic and /usr/adm's scripts, of disklabel, the file system checkers, dump, restor and the rest of the administrator's, and fstat, of the games, and of lptest, crc, cxref, error, afio, ddd, welcome and mtools, of libmp, libvmf and libplot in each model, graph, plot and its filters, troff, tc, vgrind, mset and tn3270, and tty, mesg, more, man, script, lock, tset, clear, six games, window, ve, less, jove, tip, qterm, tk and shutdown at a terminal"
