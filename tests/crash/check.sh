# What savecore saved of dump $1, and what adb finds in it, each line
# without what changes from one run to the next: the time, where in its
# function each frame of the stack is, its arguments, and so the spaces
# that adb makes tabs of where they reach a tab stop.  With "panic"
# after, the panic's message as well.
cd /usr/crash
echo "== dump $1"
echo *
cat bounds
cmp /unix unix.$1 && echo "unix.$1 is /unix"
wc -c < core.$1 | sed 's/ //g'
sed 's/.*\.  Reboot/Reboot/' /usr/adm/shutdownlog
adb -k unix.$1 core.$1 <<'E' |
$c
E
sed -e 's/^adb> //' -e 's/^[0-9a-f]*: //' -e 's/([^)]*)/()/' \
    -e 's/)[ 	]*from[ 	]*/) from /' -e 's/+[0-9a-f]*$//' -e '/^$/d'
if [ "$2" = panic ]; then
	echo '*_panicstr/s' | adb -k unix.$1 core.$1 |
	    sed -e 's/^adb> //' -e 's/.*:[ 	]*//' -e '/^$/d'
fi
# ps's commands without their process numbers, terminals and times,
# those of the system's first processes and the ones the test started,
# sorted; and pstat's totals, with the size of each table and not how
# much of it was in use.
ps -ax -k core.$1:unix.$1 |
    sed -e 1d -e 's/^ *[0-9]* .. *[0-9]*:[0-9][0-9]  *//' -e 's/ *$//' |
    egrep '^(\[swapper\]|\(init\)|-sh|sh |/tmp/crash|reboot)' | sort
pstat -T -k unix.$1:core.$1 | sed -e 's/^ *[0-9]*\//n\//' -e 's/,.*//'
echo "== end"
