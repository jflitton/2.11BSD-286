#!/bin/sh
# Test the system: boot it from a hard disk to multi-user, log in on the
# first serial port as root, use the shell, the C shell, the editor, tcsh
# and some programs; log in on two of the console's screens with the
# keyboard, and use the editor there; ping the loopback interface and log
# in over it with telnet; set up its Ethernet interface with /etc/rc.conf
# and /etc/netstart, ping the network's gateway, and log in from the host
# with telnet, sending a file each way and using vi in a big window; and
# halt it.  The kernel, /boot,
# the C library, the programs and the root file system are all built as
# they would be for a real machine (usr/src/sys/PC, usr/src/sys/i286stand,
# usr/src/lib/libc/i286, usr/src/i286).
#
# The Ethernet card is 86Box's NETCARD, ne2k8 if it is not set: an NE2000
# that is 8 bits wide, as the PicoMEM's is.  ne2k is one 16 bits wide,
# and novell_ne1k an NE1000, with half the memory (tests/harness/runtest.py
# says what they are, and docs/network.md how the driver finds out).
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/system needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
out=$top/obj/tests/system
mkdir -p "$out"

quiet() { grep -v '^cc \|^as \|^wlib \|Warning!\|Note!\|^unix:' || true; }
make -s -C "$top/usr/src/sys/PC" | quiet
make -s -C "$top/usr/src/sys/i286stand" | quiet
make -s -C "$top/usr/src/lib/libc/i286" | quiet
make -s -C "$top/usr/src/i286" all | quiet
make -s -C "$top/usr/src/i286" root | quiet

# An IDE disk shared by a partition table, as tests/kernel's first, with
# the root file system in partition a and a swap area in b.
cd "$out"
stand=$top/obj/stand
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -b "$stand/hdboot.bin" \
    -m "$stand/mbr.bin" system.img a="$top/obj/root/root.fs" b=swap:4000 |
    tail -1)

# Each text to type waits for something only the system writes: a typed
# line is echoed, so what it waits for is split by quotes when typed.
# A text may hold several lines, which the terminal keeps until the
# program reads them, and an escape, for vi, or other control
# characters, for tcsh's line editor.  --type types on the serial port;
# --key on the keyboard, once the screen shows what it waits for (see
# tests/harness/runtest.py).
CR=$(printf '\r')
ESC=$(printf '\033')
TAB=$(printf '\t')
CTRLA=$(printf '\001')
CTRLP=$(printf '\020')
CTRLU=$(printf '\025')
CTRLD=$(printf '\004')
DEL=$(printf '\177')
# Keys as an xterm sends them in the keypad mode vi sets.
KHOME=${ESC}OH
KEND=${ESC}OF
KDELETE=${ESC}[3~
# /etc/services three times, its last newline kept, for the host to send.
services=$(cat "$top/usr/src/etc/services"; echo x)
services=${services%x}
TIMEOUT=${TIMEOUT:-500} python3 -I "$top/tests/harness/runtest.py" \
    --hd system.img,$geometry,xtide --until halting --net ${NETCARD:-ne2k8} \
    --term xterm-ghostty:200x80 \
    --type "login: " "root" \
    --type "# " 'echo "she""ll-ok" $HOME' \
    --type "shell-ok /root" 'cd /tmp; mkdir dir; cd dir; pwd' \
    --type "/tmp/dir" 'cp /etc/motd m; cmp /etc/motd m && echo "sa""me"' \
    --type "same" 'for i in a b c; do echo "x$i"; done' \
    --type "xc" 'sh -c "exit 3"; echo "status $?"' \
    --type "status 3" 'echo /dev/*' \
    --type "/dev/wd1h" 'ls -l /etc | grep pass""wd' \
    --type "master.passwd" 'csh -f' \
    --type "# " 'set prompt = "c""sh% "; echo "in-c""sh"' \
    --type "in-csh" 'set l = (a b c); @ n = 40 + 2; echo "n=$#l $l[2] v$n"' \
    --type "n=3 b v42" 'foreach f (/etc/mo* /etc/gr*)' \
    --type "? " 'echo "f:$f"' \
    --type "? " 'end' \
    --type "f:/etc/group" 'echo "hi""st" one' \
    --type "hist one" '!! two' \
    --type "hist one two" 'alias say echo "sa""id"; echo "ali""ased"' \
    --type "aliased" 'say xy' \
    --type "said xy" 'if (-d /tmp && ! -e /nonesuch) echo "is""dir"' \
    --type "isdir" 'sh -c "exit 4"; echo "st=$status"' \
    --type "st=4" 'echo $nosuch' \
    --type "Undefined variable" 'cd /tmp; pushd /etc; dirs' \
    --type "/etc /tmp" 'sleep 30 &' \
    --type "[1] " 'kill %1' \
    --type "Terminated" 'time sleep 1' \
    --type "sw" 'exit' \
    --type "# " 'ex /tmp/e' \
    --type "[New file]" "a${CR}alpha beta${CR}gamma${CR}.${CR}1s/beta/delta/p" \
    --type "alpha delta" 's/a\(/x/' \
    --type "in regular expression" 'w' \
    --type "2 lines, 18 char" 'q' \
    --type "# " 'ex -r' \
    --type "No files saved." 'vi /tmp/e' \
    --type 'e" 2 lines, 18 char' "Goepsilon${ESC}1Gdd:wq" \
    --type 'e" 2 lines, 14 char' 'echo `cat /tmp/e`' \
    --type "gamma epsilon" 'tcsh -f' \
    --type "# " 'set prompt = "t""csh% "; echo "host-"$HOSTTYPE' \
    --type "host-i286" "cho t\"\"csh-ed\"\"it${CTRLA}e" \
    --type "tcsh-edit" "cat /etc/mot${TAB}" \
    --type "for the 286" "${CTRLP}" \
    --type "for the 286" 'ls-F /usr/ucb' \
    --type "view*" 'sleep 30 &' \
    --type "[1] " 'jobs' \
    --type "Running" 'kill %1' \
    --type "Terminated" 'exit' \
    --key "login: " 'root' \
    --key "# " 'echo abcX{Backspace}d' \
    --key "# " 'echo "te""rm-$TERM"' \
    --key "# " 'vi /tmp/k' \
    --key '*"/tmp/k" [New file]' "ione${CR}two${CR}three${ESC}1GOzero${ESC}" \
    --key "*zero" '1G{Down}{Down}dd:wq' \
    --key "# " '{Alt-F2}' \
    --key "login: " 'root' \
    --key "# " 'who am i' \
    --key "# " "echo wr${CTRLU}echo ri\"\"ght" \
    --key "# " "echo 'AZ!@#\$%^&*()_+{}|:<>?~'" \
    --key "# " '{Alt-F1}' \
    --type "# " 'ifconfig lo0' \
    --type "netmask" 'ping -c 2 127.0.0.1' \
    --type "packet loss" 'telnet 127.0.0.1' \
    --type "login: " 'root' \
    --type "# " 'echo "tel""net-in"; who' \
    --type "telnet-in" "${CTRLD}" \
    --type "foreign host" '. /etc/rc.conf; address=10.0.2.15 gateway=10.0.2.2; . /etc/netstart; ifconfig ne0' \
    --type "broadcast 10.0.2.255" 'ping -c 3 10.0.2.2; route add default 10.0.2.2 1' \
    --telnet "login: " 'root' \
    --telnet "# " 'echo "over""-ne0" $TERM; who' \
    --telnet "(10.0.2.2)" "cat >/tmp/in; echo \"in\"\"-done\"
$services$services$services${CTRLD}" \
    --telnet "in-done" 'cat /etc/services /etc/services /etc/services >/tmp/s3; cmp /tmp/s3 /tmp/in && echo "sa""me-in"; dd if=/etc/termcap bs=512 count=40 2>/dev/null; echo "dd""-done"' \
    --telnet "dd-done" "echo \"era\"\"x${DEL}se-ok\"; vi /tmp/bs" \
    --telnet "[New file]" "ione${CR}twoX${DEL}${CR}three${ESC}${KHOME}dd${KEND}${KDELETE}:wq" \
    --telnet "# " 'cat /tmp/bs; echo "vi""-done"' \
    --telnet "vi-done" "${CTRLD}" \
    --type "File exists" 'sleep 5; who; cat /tmp/k; echo "@ex""it 00"; sync; reboot -h' \
    --boot system

serial=$top/obj/tests/system/serial.txt
for want in "shell-ok /root" "/tmp/dir" "same" "xa" "xb" "xc" "status 3" \
    "/dev/console /dev/kmem" "master.passwd" \
    "in-csh" "n=3 b v42" "f:/etc/motd" "f:/etc/group" "hist one two" \
    "said xy" "isdir" "st=4" "nosuch: Undefined variable" "/etc /tmp" \
    "Terminated *sleep 30" "[0-9.]*u [0-9.]*s 0:0[12] " \
    "alpha delta" "More \\\\('s than \\\\)'s in regular expression" \
    "No files saved." \
    'e" 2 lines, 14 characters' "gamma epsilon" \
    "host-i286" "tcsh-edit" "view\\*" "Running *sleep 30" \
    "lo0: flags=9<UP,LOOPBACK>" "inet 127.0.0.1 netmask 0xff000000" \
    "2 packets transmitted, 2 packets received" "Connected to 127.0.0.1" \
    "(ptyp0)" "telnet-in" "root  *ttyp0 .*(localhost)" \
    "Connection closed by foreign host" \
    "ne0 at 0x300 irq 3: " \
    "inet 10.0.2.15 netmask 0xffffff00 broadcast 10.0.2.255" \
    "3 packets transmitted, 3 packets received" "File exists" \
    "root  *console" "root  *ttyv1" "root  *tty00" "halting"; do
    if ! grep -q "$want" "$serial"; then
        echo "system: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
if ! tr -d '\r' < "$serial" | grep -a -A2 '^zero$' | tr '\n' ' ' |
    grep -q '^zero one three $'; then
    echo "system: /tmp/k is not as vi on the console left it" >&2
    exit 1
fi

# What came over telnet from the host: the login, with TERM the xterm
# that telnetd made of xterm-ghostty and DEL the erase character; the
# file it sent; the first 20K of /etc/termcap, with the terminal's
# newlines, and its tabs expanded if it was set to; and vi, which in a
# window of 200 by 80 has its last line on the window's last, takes DEL
# for erase too, and has Home from the termcap (to the top of the
# screen), and End and Delete from root's .exrc.
python3 -I - "$out/telnet.txt" "$top/obj/root/termcap" <<'EOF'
import re, sys
text = open(sys.argv[1], encoding="latin-1").read().replace("\r\n", "\n")
termcap = open(sys.argv[2], encoding="latin-1").read()[:20480]
bad = [w for w in ("erase DEL, kill ^U, intr ^C", "over-ne0 xterm\n",
                   "same-in", "dd-done", "\nerase-ok\n", "\033[80;1H",
                   "\ntwo\nhree\nvi-done") if w not in text]
if "Screen too large" in text:
    bad.append("vi's window")
if not re.search(r"root\s+ttyp\d.*\(10\.0\.2\.2\)", text):
    bad.append("who on a pseudo-terminal")
lines = termcap.split("\n")
expanded = "\n".join(l.expandtabs(8) for l in lines)
if termcap not in text and expanded not in text:
    bad.append("/etc/termcap as it is")
if bad:
    sys.exit("system: telnet did not show %s" % ", ".join(bad))
EOF

# What the screens showed: vi's screen, all 25 lines of it, as the
# console's terminal drew it, and what the keyboard typed, with the
# Backspace key erasing a character, on the screen too, as root's
# .profile has it do.
python3 -I - "$out/screen.txt" <<'EOF'
import re, sys
screens = open(sys.argv[1], encoding="utf-8").read().split("=" * 80 + "\n")
rows = [s.split("\n")[:25] for s in screens]
lines = {r for s in rows for r in s}
vi = ["zero", "one", "two", "three"] + ["~"] * 20
want = ["# echo abcd", "abcd", "term-pccons", "right",
        "AZ!@#$%^&*()_+{}|:<>?~"]
bad = [w for w in want if w not in lines]
if not any(s[:24] == vi for s in rows):
    bad.append("vi's screen")
if not any(re.search(r"root\s+ttyv1", r) for r in lines):
    bad.append("who am i on ttyv1")
if bad:
    sys.exit("system: the screens did not show %s" % ", ".join(bad))
EOF
python3 -I "$top/tools/fsck.py" system.img
python3 -I "$top/tools/fsck.py" -x /tmp/dir/m system.img | grep -q "for the 286"
echo "system: logged in on a serial port, the screens and over telnet by the loopback interface and the Ethernet card, used sh, csh, vi and tcsh, and halted"
