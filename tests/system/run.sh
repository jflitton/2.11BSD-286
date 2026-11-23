#!/bin/sh
# Test the system: boot it from a hard disk whose root file system was
# not unmounted cleanly, which /etc/rc has fsck check before it starts
# the system again, to multi-user; log in on the first serial port as
# root, use the shell, the C shell, in which a file read a second time
# must be read without a block from the disk (the "0+" in time's line:
# the kernel's buffers keep files' data when there are more than 64 of
# them), the editor, tcsh and some programs;
# log in on two of the console's screens with the keyboard, and use the
# editor there; make the password database as tools/mkpasswd.py made the
# one on the disk, which must be the same; add a user with adduser, and
# make, check and mount a file system; look at the processes, the
# kernel's tables, the system's counts and the disk's with ps, pstat,
# vmstat and iostat; find in the log what the kernel printed as it
# started, a line from logger, and the kernel's complaint when a small
# file system is filled; use find, and read the accounting with lastcomm
# and sa; reckon with expr, awk, bc and dc, search with egrep, make a
# file with make, and have yacc and lex make of awk's grammar and
# scanner what the ones the system was built with made; ping the
# loopback interface and log in over it with
# telnet as that user, in the C shell, who looks at its processes, the
# users and the connections with ps, w and netstat, gives cron a table
# and at a job, changes the full name with chfn and the password with
# passwd and logs in again with the new one; set up its Ethernet
# interface with /etc/rc.conf and /etc/netstart, ping the network's
# gateway, show the address it found for it with arp and flush the
# routes with route, and log in from the host with telnet, sending a
# file each way and using vi in a big window; run the user's job with
# atrun and wait for cron to run the user's table; and halt it.  The
# kernel, /boot, the C library, the programs
# and the root file system are all built as they would be for a real
# machine (usr/src/sys/PC, usr/src/sys/i286stand, usr/src/lib/libc/i286,
# usr/src/i286).
#
# The Ethernet card is 86Box's NETCARD, ne2k8 if it is not set: an NE2000
# that is 8 bits wide, as the PicoMEM's is.  ne2k is one 16 bits wide,
# and novell_ne1k an NE1000, with half the memory (tests/harness/runtest.py
# says what they are, and docs/network.md how the driver finds out).
# The system is the cross compiler's work with TOOLS=cross, and its own
# compiler's, assembler's and linker's with TOOLS=native; TOOLS_KERNEL,
# TOOLS_PROGRAMS and TOOLS_BOOT choose for the kernel, the programs or
# the boot programs alone, and without any it is the default build
# (tests/harness/system.sh).  Each case is in a directory of its own,
# obj/tests/system-n and so on.
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/system needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
. "$top/tests/harness/system.sh"
out=$top/obj/tests/system$case
mkdir -p "$out"
buildsys

# The root file system as a power cut leaves one: its superblock not
# marked clean (MNT_CLEAN in fs_flags, usr/src/sys/h/fs.h).
cd "$out"
cp "$top/obj/root/root.fs" root.fs
python3 -I - root.fs <<'EOF'
import struct, sys
at = 1024 + struct.calcsize("<Hlh50lh100H4bllHhh12sHH")
with open(sys.argv[1], "r+b") as f:
    f.seek(at)
    flags, = struct.unpack("<H", f.read(2))
    assert flags & 0x2000, "root.fs is not marked clean"
    f.seek(at)
    f.write(struct.pack("<H", flags & ~0x2000))
EOF

# A file system of awk's grammar and scanner, and the C that the yacc and
# lex the system's programs are built with (obj/host) make of them: the
# system's own must make the same.
rm -rf gram && mkdir gram
cp "$top/usr/src/bin/awk/awk.g.y" "$top/usr/src/bin/awk/awk.lx.l" gram
(cd gram && "$top/obj/host/yacc" -d awk.g.y >/dev/null &&
    mv y.tab.c awk.g.c && mv y.tab.h awk.g.h &&
    "$top/obj/host/lex" awk.lx.l && mv lex.yy.c awk.lx.c)
for f in gram/*; do echo "f /${f#gram/} 644 $f"; done > gram.proto
python3 -I "$top/tools/mkfs.py" -s 400 gram.proto gram.fs >/dev/null

# An IDE disk shared by a partition table, as tests/kernel's first, with
# that in partition a, a swap area in b, room for a file system in d,
# for one in e that the kernel is too big a file for, and that one in f.
geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" system.img a=root.fs b=swap:4000 d=fs:4000 e=fs:200 f=gram.fs | tail -1)

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
CTRLR=$(printf '\022')
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
TIMEOUT=${TIMEOUT:-600} python3 -I "$top/tests/harness/runtest.py" \
    --hd system.img,$geometry,xtide --until halting --resets 1 \
    --net ${NETCARD:-ne2k8} \
    --term xterm-ghostty:200x80 \
    --key "*Marking file system clean" '{Alt-F1}' \
    --type "login: " "root" \
    --type "# " 'echo "she""ll-ok" $HOME' \
    --type "shell-ok /root" 'cd /tmp; mkdir dir; cd dir; pwd' \
    --type "/tmp/dir" 'cp /etc/motd m; cmp /etc/motd m && echo "sa""me"' \
    --type "same" 'for i in a b c; do echo "x$i"; done' \
    --type "xc" 'sh -c "exit 3"; echo "status $?"' \
    --type "status 3" "echo ta${TAB}x${DEL}${DEL}b\"\"-ok" \
    --type "tab-ok" "echo ret${CTRLR}yped\"\"-ok" \
    --type "retyped-ok" 'echo /dev/*' \
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
    --type "sw" 'cat /usr/share/misc/termcap > /dev/null; time cat /usr/share/misc/termcap > /dev/null' \
    --type "% 0+" 'exit' \
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
    --type "# " 'cp /etc/master.passwd /tmp/mp; mkpasswd -p /tmp/mp; cmp /tmp/mp.dir /etc/passwd.dir && cmp /tmp/mp.pag /etc/passwd.pag && cmp /tmp/mp.orig /etc/passwd && echo "db-s""ame"' \
    --type "# " '/usr/local/adduser' \
    --type "Login name: " 'guest' \
    --type "Full name: " 'Guest User' \
    --type "Group [staff]: " 'guest' \
    --type "Shell [/bin/csh]: " '' \
    --type "Home directory [/home/guest]: " '' \
    --type "Uid [100]: " '' \
    --type "Add this user [y]: " '' \
    --type "New password:" 'Gu3st-pw' \
    --type "Retype new password:" 'Gu3st-pw' \
    --type "# " 'sleep 3; ls -lga /home/guest; newfs wd0d' \
    --type "# " 'fsck /dev/rwd0d' \
    --type "# " 'mount /dev/wd0d /mnt; cp /etc/termcap /etc/motd /mnt; mkdir /mnt/d; df /mnt' \
    --type "# " 'umount /mnt; fsck -p /dev/rwd0d; fsck -f /dev/rwd0d' \
    --type "# " 'ps axl | grep "[i]netd"; vmstat -s | grep "system c""alls"; iostat; pstat -T' \
    --type "# " 'echo dmesg-`dmesg | grep -c "mem  *="`; logger -t test "lo""gged"; sleep 2; grep "t: log""ged" /usr/adm/messages /usr/adm/debuglog' \
    --type "# " 'newfs wd0e >/dev/null; mount /dev/wd0e /mnt; cp /unix /mnt; umount /mnt; logger -t test woken; sleep 2; grep "unix: [a-z/]" /usr/adm/messages' \
    --type "# " 'find /usr/spool -type d -print; find /tmp -name mp.orig -exec echo "fou""nd" {} \; ; echo acct-`lastcomm stty | grep -c tty00`; sa -m' \
    --type "# " 'echo ex-`expr 3 + 4 \* 5`-`expr abcdef : "ab\(.*\)f"`-`expr substr abcdef 2 3`; echo eg-`egrep -c "^(root|daemon):" /etc/passwd`; echo "2^64; scale=10; 4*a(1)" | bc -l; echo "16 o 255 p" | dc' \
    --type "# " 'awk "BEGIN { printf \"%d %5.2f %s|\", 7/2, 22/7, substr(\"awkward\", 1, 3); print sqrt(2), int(exp(1)*100), length(\"four\") }" </dev/null' \
    --type "# " 'awk "\$3 == \"swap\" { print \"awk-\" NR \"-\" NF \"-\" \$1 } { n += length(\$2) } END { print \"len-\" n }" /etc/fstab' \
    --type "# " 'echo a b a c a b | awk "{ for (i = 1; i <= NF; i++) c[\$i]++ } END { print \"cnt-\" c[\"a\"] c[\"b\"] c[\"c\"]; n = split(\"x:y:z\", f, \":\"); print \"spl-\" n f[3] }"' \
    --type "# " "cd /tmp; echo 'b: a' >Makefile; echo '${TAB}cp a b; echo made-\$@-from-\$?' >>Makefile; echo 1 >a; make; make; cd /" \
    --type "# " 'mount /dev/wd0f /mnt; cd /mnt; yacc -d awk.g.y; cmp y.tab.c awk.g.c && cmp y.tab.h awk.g.h && echo "yacc""-same"; lex awk.lx.l; cmp lex.yy.c awk.lx.c && echo "lex""-same"; cd /; umount /mnt' \
    --type "# " 'ifconfig lo0' \
    --type "netmask" 'ping -c 2 127.0.0.1' \
    --type "packet loss" 'telnet 127.0.0.1' \
    --type "login: " 'guest' \
    --type "Password:" 'not-the-pw' \
    --type "login: " 'guest' \
    --type "Password:" 'Gu3st-pw' \
    --type "% " 'echo "tel""net-in"; who; echo "as-gu""est"; ls -l /etc/passwd; cat /etc/master.passwd' \
    --type "% " 'sleep 20 & ps axu | grep "[s]leep 20"; w -s | grep "^gu""est"; echo "est-`netstat -n | grep -c ESTAB`"; kill %1' \
    --type "% " 'crontab -l; echo "* * * * * date >/tmp/gcron" >ct; crontab ct; crontab -l | grep gcron; echo "echo ran >/tmp/gat" | at 2359; atq' \
    --type "% " "setenv EDITOR ed; chfn${CR}!sleep 2${CR}/^Full Name/s/Guest User/Gus Guest/${CR}w${CR}q" \
    --type "% " 'sleep 3; grep guest /etc/passwd; passwd' \
    --type "Old password:" 'Gu3st-pw' \
    --type "New password:" 'N3w-gu3st' \
    --type "Retype new password:" 'N3w-gu3st' \
    --type "% " 'sleep 3' \
    --type "% " "${CTRLD}" \
    --type "foreign host" 'telnet 127.0.0.1' \
    --type "login: " 'guest' \
    --type "Password:" 'N3w-gu3st' \
    --type "% " 'echo "new""-pw-ok"' \
    --type "new-pw-ok" "${CTRLD}" \
    --type "foreign host" '. /etc/rc.conf; address=10.0.2.15 gateway=10.0.2.2; . /etc/netstart; ifconfig ne0' \
    --type "broadcast 10.0.2.255" 'ping -c 3 10.0.2.2; route add default 10.0.2.2 1' \
    --type "File exists" 'arp -a; route -f; echo "fl""ushed"' \
    --telnet "login: " 'root' \
    --telnet "# " 'echo "over""-ne0" $TERM; who' \
    --telnet "(10.0.2.2)" "cat >/tmp/in; echo \"in\"\"-done\"
$services$services$services${CTRLD}" \
    --telnet "in-done" 'cat /etc/services /etc/services /etc/services >/tmp/s3; cmp /tmp/s3 /tmp/in && echo "sa""me-in"; dd if=/etc/termcap bs=512 count=40 2>/dev/null; echo "dd""-done"' \
    --telnet "dd-done" "echo \"era\"\"x${DEL}se-ok\"; vi /tmp/bs" \
    --telnet "[New file]" "ione${CR}twoX${DEL}${CR}three${ESC}${KHOME}dd${KEND}${KDELETE}:wq" \
    --telnet "# " 'cat /tmp/bs; echo "vi""-done"' \
    --telnet "vi-done" "${CTRLD}" \
    --type "flushed" 'cd /usr/spool/at; mv 2* 1999.001.0000.aa; /usr/libexec/atrun; cd /; while test ! -f /tmp/gcron; do sleep 5; done; echo "wai""ted"' \
    --type "waited" 'ls -l /tmp/gat /tmp/gcron; atq; echo cronlog-`grep -c "(guest) CMD (date" /usr/adm/cron`; echo tabs-`ls /var/cron/tabs`; echo "cr""on-done"' \
    --type "cron-done" 'sleep 5; who; cat /tmp/k; echo "@ex""it 00"; sync; reboot -h' \
    --boot system$case

serial=$out/serial.txt
for want in "shell-ok /root" "/tmp/dir" "same" "xa" "xb" "xc" "status 3" \
    "/dev/acctlog /dev/console /dev/errlog /dev/fl0a" "master.passwd" \
    "in-csh" "n=3 b v42" "f:/etc/motd" "f:/etc/group" "hist one two" \
    "said xy" "isdir" "st=4" "nosuch: Undefined variable" "/etc /tmp" \
    "Terminated *sleep 30" "[0-9.]*u [0-9.]*s 0:0[12] " \
    "alpha delta" "More \\\\('s than \\\\)'s in regular expression" \
    "No files saved." \
    'e" 2 lines, 14 characters' "gamma epsilon" \
    "host-i286" "tcsh-edit" "view\\*" "Running *sleep 30" \
    "lo0: flags=9<UP,LOOPBACK>" "inet 127.0.0.1 netmask 0xff000000" \
    "2 packets transmitted, 2 packets received" "Connected to 127.0.0.1" \
    "db-same" "Added guest, uid 100, in /home/guest." \
    "rw-r--r-- *1 guest *guest .* .cshrc" \
    "guest:\*:100:31:Gus Guest,,,:/home/guest:/bin/csh" \
    "newfs: /sbin/mkfs -m 2 -n [0-9]* -i 4096 -s 2000 /dev//*rwd0d" "m/n = 2 " \
    "\*\* Phase 5 - Check Free List" "FILE SYSTEM WAS MODIFIED" \
    "^/dev/wd0d .* /mnt" "/dev/rwd0d: file system is clean; not checking" \
    "^5 files, " \
    "selwait .* inetd" " [0-9][0-9]* system calls" " tin tout bps tps msps" \
    "[0-9]/ *[0-9]* processes" "swapmap entries, .* kB used" \
    "dmesg-3" "/usr/adm/messages:.* test: logged" \
    "/usr/adm/debuglog:.* test: logged" "unix: phys mem  = [0-9]" \
    "unix: /mnt: file system full" "^/usr/spool/at/past" "found /tmp/mp.orig" \
    "acct-[1-9]" "^root  *[0-9]*  *[0-9.]*cpu" \
    "ex-23-cde-bcd" "eg-2" "^18446744073709551616" "^3.1415926532" "^FF" \
    "^3  3.14 awk|1.41421 271 4" "awk-2-6-/dev/wd0b" "len-5" "cnt-321" \
    "spl-3z" "made-b-from-a" "\`b' is up to date" "yacc-same" "lex-same" \
    "(ptyp0)" "Login incorrect" "telnet-in" "guest  *ttyp0 .*(localhost)" \
    "^guest  *[0-9]*  *0  *[0-9]* p0  *0:00 sleep 20" "^guest  *p0 " "est-2" \
    "no crontab for guest" "^\* \* \* \* \* date >/tmp/gcron" \
    " 1st .* 23:59  *guest  *[0-9]*  *stdin" \
    "rw-r--r--  1 guest .*/tmp/gat" "rw-r-----  1 guest .*/tmp/gcron" \
    "no files in queue" "cronlog-[1-9]" "tabs-guest" "cron-done" \
    "/etc/master.passwd: Permission denied" "Changing password for guest" \
    "new-pw-ok" "Connection closed by foreign host" \
    "ne0 at 0x300 irq 3: " \
    "inet 10.0.2.15 netmask 0xffffff00 broadcast 10.0.2.255" \
    "3 packets transmitted, 3 packets received" "File exists" \
    "(10.0.2.2) at " "default  *10.0.2.2  *done" \
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
# A tab erased and a line typed again: the terminal driver reads back
# what is waiting in the queue, out of the character blocks' segment,
# to find how wide the tab was, seven columns after "# echo ta", and
# to write the line again after ^R.
if ! python3 -I - "$serial" <<'EOF'
import sys
text = open(sys.argv[1], "rb").read()
sys.exit(not (b"# echo ta" + b" " * 7 + b"x\b \b" + b"\b" * 7 + b'b""-ok\r' in text
    and b"# echo ret^R\r\necho retyped" in text))
EOF
then
    echo "system: a tab was not erased, or a line not typed again, as typed" >&2
    exit 1
fi
# The password database, which the user could read and root's
# master.passwd not.
if ! tr -d '\r' < "$serial" | grep -a -A1 '^as-guest$' |
    grep -q '^-rw-r--r--  1 root '; then
    echo "system: the user could not find out who owns /etc/passwd" >&2
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

# What the screens showed: what /etc/rc said as the system came up,
# with fsck checking the root and finding it not clean, and then, the
# system started again, clean; vi's screen, all 25 lines of it, as the
# console's terminal drew it; and what the keyboard typed, with the
# Backspace key erasing a character, on the screen too, as root's
# .profile has it do.
python3 -I - "$out/screen.txt" <<'EOF'
import re, sys
screens = open(sys.argv[1], encoding="utf-8").read().split("=" * 80 + "\n")
rows = [s.split("\n")[:25] for s in screens]
lines = {r for s in rows for r in s}
vi = ["zero", "one", "two", "three"] + ["~"] * 20
want = ["/dev/wd0a: Marking file system clean",
        "/dev/wd0a: file system is clean; not checking",
        "starting system logger", "clearing /tmp",
        "standard daemons: update cron accounting.",
        "starting network daemons: inetd.",
        "# echo abcd", "abcd", "term-pccons", "right",
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
python3 -I "$top/tools/fsck.py" -p d system.img
python3 -I "$top/tools/fsck.py" -p d -x /termcap system.img |
    cmp -s - "$top/obj/root/termcap"
python3 -I "$top/tools/fsck.py" -p f system.img
echo "system: checked its root, logged in on a serial port, the screens and over telnet by the loopback interface and the Ethernet card, used sh, csh, vi and tcsh, added a user with a password, made a file system, read the log and the accounting, used expr, awk, egrep, bc, dc and make, had yacc and lex make what the build's made, ran the user's jobs with cron and at, and halted"
