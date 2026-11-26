# What tests/lp has root do (run.sh): print on the parallel port's
# printer, straight to /dev/lp0 and by lpr, and say what the system made
# of it.  What the printer printed the harness keeps (printed.txt).
echo "== probe"
dmesg | grep '^lp'
echo "== direct"
# More than a page, which the printer ends itself; and the driver takes
# more than it holds (LPHWAT), so the write waits for the printer.
lptest 79 70 > /dev/lp0
# A tab, a carriage return and the line struck over, spaces at the end
# of a line, and a form feed between two pages.
echo 'tab	here' > /tmp/d
echo 'xyz, then a carriage return#abc' | tr '#' '\015' >> /tmp/d
echo 'spaces at the end   ' >> /tmp/d
echo 'page one' >> /tmp/d
echo '#page two' | tr '#' '\014' >> /tmp/d
cat /tmp/d > /dev/lp0
echo "== lpr"
# By lpd and lpf, as /etc/printcap has the printer, with no banner.
lptest 40 3 | lpr -h
lpr -h /tmp/d
n=0
while ls /usr/spool/lpd | grep '^cf' > /dev/null && test $n -lt 120
do
	sleep 1; n=`expr $n + 1`
done
sleep 2
lpq
lpc status lp
echo "== end"
