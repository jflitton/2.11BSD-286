# What tests/slip has root do.  /etc/rc has attached sl0 to COM2, whose
# far end sends each packet back as from sloop, 10.0.3.2 (run.sh).
cd /
echo "== attach"
ifconfig sl0
netstat -rn | sed -n '/^10\.0\.3\./p' | awk '{ print $1, $2, $3 }'
echo "== ping"
# Times taken out: 56 bytes, then 900, every value a byte can have, and
# 1400, more than the line's 1006, which IP sends in two pieces.
for size in 56 900 1400
do
	ping -c 2 -s $size sloop | sed -e 's/ time=.*//' -e '/^round-trip/d'
done
echo "== tcp"
rcp /bin/sh sloop:/tmp/sh
cmp /bin/sh /tmp/sh && echo sh sent
rcp sloop:/tmp/sh /tmp/sh.back
cmp /bin/sh /tmp/sh.back && echo sh fetched
rsh sloop echo hello from rsh
echo "== errors"
# The interface's errors in and out and its queue's overflows, and the
# packets IP and TCP threw away as damaged.
netstat -i | awk '$1 == "sl0" { print $1, $2, $3, $4, $6, $8, $9 }'
netstat -s | sed -n -e '/bad header checksum/p' -e '/discarded for bad checksum/p'
echo "== end"
