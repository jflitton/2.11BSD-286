#!/bin/sh
# Test UUCP: boot the system with /etc/uucp/L.sys naming it twice, as
# loop, called by TCP, and as ploop, called by a direct line that is a
# pty, and have root, by uucp.sh, queue work for each and run uucico
# to do it: by TCP, inetd starts uucpd, which logs the call in as
# Uloop, a login whose shell is uucico, and the two talk the t
# protocol; by the pty, the other end has a uucico of its own, and the
# two talk g, the protocol of serial lines and modems.  By TCP a file
# and a program go, and a letter by uux, for uuxqt to give to rmail and
# sendmail to deliver; by the pty a program goes and a file comes back.
# Then clean.daily archives the logs, as /usr/adm/daily has it do.  What
# uuname, uusnap and uulog say, what arrived and what clean.daily left
# must be as `expected` has it.
#
# TOOLS, TOOLS_KERNEL and TOOLS_PROGRAMS are as in tests/system
# (tests/harness/system.sh).
#
# Run from the top of the repository after `. tools/env.sh`.
set -e
if [ "$EMU" = qemu ]; then
    echo "tests/uucp needs 86Box: the kernel is for a 286" >&2
    exit 2
fi
top=$(cd "$(dirname "$0")/../.." && pwd)
here=$top/tests/uucp
. "$top/tests/harness/system.sh"
out=$top/obj/tests/uucp$case
mkdir -p "$out"
buildsys
proto=$top/obj/build/root.proto
dest=$top/obj/build/dest
if [ "$TOOLS_PROGRAMS" = native ]; then
    proto=$top/obj/nbuild/root.proto
    dest=$top/obj/nbuild/dest
fi
cd "$out"

# Uloop, the login a system calling by TCP gives uucpd, with no
# password; the systems to call, and the line to call ploop by; root
# let send and fetch any file, where upstream's USERFILE keeps everyone
# to /usr/spool/uucppublic, as it still keeps the systems that call;
# and inetd told to start uucpd, as upstream's inetd.conf has it to be.
rm -rf etc && mkdir etc
cp "$dest/etc/master.passwd" etc/master.passwd
echo 'Uloop::67:1::0:0:UUCP login for loop:/usr/spool/uucppublic:/usr/sbin/uucico' \
    >> etc/master.passwd
python3 -I "$top/tools/mkpasswd.py" etc/master.passwd etc
cat > etc/L.sys <<'END'
loop Any TCP uucp localhost login: Uloop
ploop Any DIR 9600 ptyp7
END
echo 'DIR ptyp7 unused 9600 direct' > etc/L-devices
printf 'root, /\n, /usr/spool/uucppublic\n' > etc/USERFILE
sed 's/^#uucpd/uucpd/' "$dest/etc/inetd.conf" > etc/inetd.conf
sed -e "s,^\(f /etc/master.passwd [^ ]*\) .*,\1 $out/etc/master.passwd," \
    -e "s,^\(f /etc/passwd [^ ]*\) .*,\1 $out/etc/passwd," \
    -e "s,^\(f /etc/passwd.dir [^ ]*\) .*,\1 $out/etc/passwd.dir," \
    -e "s,^\(f /etc/passwd.pag [^ ]*\) .*,\1 $out/etc/passwd.pag," \
    -e "s,^\(f /etc/inetd.conf [^ ]*\) .*,\1 $out/etc/inetd.conf," \
    -e "s,^\(f /etc/uucp/L.sys [^ ]*\) .*,\1 $out/etc/L.sys," \
    -e "s,^\(f /etc/uucp/L-devices [^ ]*\) .*,\1 $out/etc/L-devices," \
    -e "s,^\(f /etc/uucp/USERFILE [^ ]*\) .*,\1 $out/etc/USERFILE," \
    "$proto" > root.proto
echo "f /root/uucp.sh 644 $here/uucp.sh" >> root.proto
python3 -I "$top/tools/mkfs.py" -s 24000 root.proto root.fs >/dev/null

geometry=$(python3 -I "$top/tools/mkdisk.py" -p -g 977,5,17 \
    -b "$stand/hdboot.bin" -m "$stand/mbr.bin" uucp.img \
    a=root.fs b=swap:4000 | tail -1)
TIMEOUT=${TIMEOUT:-900} python3 -I "$top/tests/harness/runtest.py" \
    --hd uucp.img,$geometry,xtide \
    --type "login: " root \
    --type "# " 'sh /root/uucp.sh; echo "@ex""it 00"' \
    --boot uucp$case

# What the system wrote from the first check to the last.
serial=$top/obj/tests/uucp$case/serial.txt
tr -d '\r' < "$serial" | sed -n '/^== names$/,/^== end$/p' |
    sed -e '/^$/d' > got
if ! diff "$here/expected" got; then
    echo "uucp: what the system said is not as $here/expected has it" >&2
    exit 1
fi
echo "uucp: files, programs and a letter sent and fetched by TCP and by g"
