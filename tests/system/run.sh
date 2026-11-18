#!/bin/sh
# Test the system: boot it from a hard disk to multi-user, log in on the
# console as root, use the shell, the C shell, the editor, tcsh and some
# programs, and halt it.  The kernel, /boot, the C library, the programs and the root file system are
# all built as they would be for a real machine (usr/src/sys/PC,
# usr/src/sys/i286stand, usr/src/lib/libc/i286, usr/src/i286).
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
# characters, for tcsh's line editor.
CR=$(printf '\r')
ESC=$(printf '\033')
TAB=$(printf '\t')
CTRLA=$(printf '\001')
CTRLP=$(printf '\020')
TIMEOUT=${TIMEOUT:-400} python3 -I "$top/tests/harness/runtest.py" \
    --hd system.img,$geometry,xtide --until halting \
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
    --type "# " 'who; echo "@ex""it 00"; sync; /sbin/reboot -h' \
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
    "root  *console" "halting"; do
    if ! grep -q "$want" "$serial"; then
        echo "system: no \"$want\" in what the system wrote" >&2
        exit 1
    fi
done
python3 -I "$top/tools/fsck.py" system.img
python3 -I "$top/tools/fsck.py" -x /tmp/dir/m system.img | grep -q "for the 286"
echo "system: logged in, used sh, csh, vi and tcsh, and halted"
