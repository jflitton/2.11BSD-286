#!/bin/sh
# Test the network's programs: boot the system from a hard disk to
# multi-user with an Ethernet card, log in on the first serial port as
# root, add two users, guest and ftp, and have the system run each of
# the checks in tests/network/checks, which are on a file system of
# their own (tests/harness/checks.py): the interface brought up and
# inetd given more to start; ntp's check of its arithmetic; finger and
# fingerd; tcpd's rules, with a connection it refuses; rsh, rcp and
# rexecd; tftp; ftp as a user, as nobody who may, and as the anonymous
# one; nc, whois and traceroute; and the daemons that keep the network
# told, rwhod, timed, ntpd and routed, with what asks them.  Then use
# what needs a terminal: rlogin as a user with a password and as one
# /etc/hosts.equiv lets in without, one inside the other, and its
# escape; and talk between root on the serial port and the user logged
# in from the host with telnet, which tcpd lets through.  The kernel,
# /boot, the C library, the programs and the root file system are built
# as they would be for a real machine, as tests/system has them.
#
# The Ethernet card is 86Box's NETCARD, ne2k8 if it is not set, as in
# tests/system.  Run from the top of the repository after
# `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/network needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
out=$top/obj/tests/network
mkdir -p "$out"

quiet() { grep -v '^cc \|^as \|^wlib \|Warning!\|Note!\|^unix:' || true; }
make -s -C "$top/usr/src/sys/PC" | quiet
make -s -C "$top/usr/src/sys/i286stand" | quiet
make -s -C "$top/usr/src/lib/libc/i286" | quiet
make -s -C "$top/usr/src/i286" all | quiet
make -s -C "$top/usr/src/i286" root | quiet

# The file system of checks, with ntp's own test, which its Makefile
# runs when it has built the rest, and what a client of rexecd sends: a
# port for the errors, here none, a user, a password and a command.
cd "$out"
cp "$top/obj/root/root.fs" root.fs
rm -rf checks
python3 -I "$top/tests/harness/checks.py" "$top/tests/network/checks" checks
python3 -I "$top/tools/strip.py" "$top/obj/bin/ntest" checks/d/ntest
printf '0\0guest\0Gu3st-pw\0echo rexec-$USER; echo err >&2\0' > checks/d/rexec.in
{
    echo "d /t 755"; echo "d /x 755"; echo "d /d 755"
    echo "f /check.sh 644 checks/check.sh"
    echo "f /names 644 checks/names"
    for f in checks/t/* checks/x/* checks/d/*; do
        echo "f /${f#checks/} 644 $f"
    done
} > checks.proto
python3 -I "$top/tools/mkfs.py" -s 400 checks.proto checks.fs >/dev/null

stand=$top/obj/stand
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -b "$stand/hdboot.bin" \
    -m "$stand/mbr.bin" network.img a=root.fs b=swap:4000 d=checks.fs | tail -1)

# What is typed waits for something only the system writes, as in
# tests/system.  A program that sets the terminal as it starts, as
# rlogin does, throws away what was typed before, so what is typed to
# the far side waits for that side's greeting.  rlogin's escape is
# taken by the first rlogin, which the second is inside, and ends both.
# talk draws with the cursor: its last line, "[Connection closing.
# Exiting]", is written over the one before from where they differ.
CTRLC=$(printf '\003')
CTRLD=$(printf '\004')
TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
    --hd network.img,$geometry,xtide --until halting \
    --net ${NETCARD:-ne2k8} --term vt100:80x24 \
    --type "login: " "root" \
    --type "# " 'adduser' \
    --type "Login name: " 'guest' \
    --type "Full name: " 'Guest User' \
    --type "Group [staff]: " 'guest' \
    --type "Shell [/bin/csh]: " '/bin/sh' \
    --type "Home directory [/home/guest]: " '' \
    --type "Uid [100]: " '' \
    --type "Add this user [y]: " '' \
    --type "New password:" 'Gu3st-pw' \
    --type "Retype new password:" 'Gu3st-pw' \
    --type "# " 'sleep 3; adduser' \
    --type "Login name: " 'ftp' \
    --type "Full name: " 'Anonymous FTP' \
    --type "Group [staff]: " 'guest' \
    --type "Shell [/bin/csh]: " '/bin/sh' \
    --type "Home directory [/home/ftp]: " '' \
    --type "Uid [101]: " '' \
    --type "Add this user [y]: " '' \
    --type "New password:" 'N0-0ne-in' \
    --type "Retype new password:" 'N0-0ne-in' \
    --type "# " 'sleep 3; mount /dev/wd0d /mnt; sh /mnt/check.sh; umount /mnt' \
    --type "checks: " 'rlogin localhost -l guest' \
    --type "$ " 'echo "rl""-in" $TERM `tty`; who am i' \
    --type "rl-in" "${CTRLD}" \
    --type "# " 'rm /home/guest/.rhosts; rlogin localhost -l guest' \
    --type "Password:" 'Gu3st-pw' \
    --type "Last login" 'echo "rl""-pw"; rsh localhost echo "rsh""-equiv"' \
    --type "rsh-equiv" 'rlogin localhost' \
    --type "Last login" 'echo "rl""-two" `tty`' \
    --type "rl-two" '~.' \
    --telnet "login: " 'guest' \
    --telnet "Password:" 'Gu3st-pw' \
    --telnet "$ " 'echo "tel""-in" $TERM; mesg; who' \
    --type "Closed connection." 'talk guest' \
    --telnet "respond with" 'talk root@pc286' \
    --telnet "established" 'hello-from-guest' \
    --type "hello-from-guest" 'hello-from-root' \
    --telnet "hello-from-root" "${CTRLC}" \
    --type "Exiting]" 'echo "after""-talk"' \
    --telnet "$ " "${CTRLD}" \
    --type "after-talk" 'sleep 2; who; echo "@ex""it 00"; sync; reboot -h' \
    --boot network

serial=$out/serial.txt
if grep -q "^BAD " "$serial"; then
    tr -d '\r' < "$serial" | sed -n '/^BAD /,/^END /p' >&2
    echo "network: those checks did not write what they must" >&2
    exit 1
fi
n=$(wc -l < checks/names | tr -d ' ')
if ! grep -q "^checks: $n ok, 0 bad" "$serial"; then
    echo "network: not all $n checks ran" >&2
    exit 1
fi
for want in "^rl-in vt100 /dev/ttyp0" "pc286.guest  *ttyp0 .*(localhost)" \
    "Connection closed\." "^rl-pw" "^rsh-equiv" "^rl-two /dev/ttyp1" \
    "^Closed connection." "Waiting for your party to respond" \
    "Connection established" "hello-from-guest" "^after-talk" \
    "^root  *tty00" "halting"; do
    if ! grep -q -- "$want" "$serial"; then
        echo "network: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
# What the user logged in from the host saw: the login, which tcpd let
# through to telnetd; talk's announcement, written to the terminal by
# the daemon; and what root typed.
for want in "tel-in vt100" "^is y" "guest  *ttyp0 .*(10.0.2.2)" \
    "Message from Talk_Daemon@pc286" \
    "talk: connection requested by root@pc286" \
    "talk: respond with:  talk root@pc286" "Connection established" \
    "hello-from-root" "closing. Exiting"; do
    if ! grep -a -q -- "$want" "$out/telnet.txt"; then
        echo "network: no \"$want\" in what telnet showed" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" network.img
echo "network: $n checks of the network's programs, and rlogin and talk at terminals, one of them a telnet from the host"
