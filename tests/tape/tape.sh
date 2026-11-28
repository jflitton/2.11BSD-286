# What tests/tape/run.sh has the system do with its two tape drives:
# st0, a QIC drive whose records are blocks of 512 bytes, with a tape
# that has nothing on it, and st1, a DAT drive whose records are of any
# length, with a tape the host wrote: a tar archive of hello and bye,
# and then records of 100, 3000, 512 and 7 bytes, each file ending at a
# tape mark.  The first open looks over the bus, and the kernel says
# what it found there, before what is compared; syslogd, woken, puts it
# on the console again, which it would otherwise do whenever it next
# woke.  awk reads its input after BEGIN, which here is none; dd says
# how long it took, which is left out; and diff -r exits with 2 for
# directories, as upstream's always has.
PATH=/bin:/sbin:/usr/sbin:/usr/ucb:/usr/bin:/usr/new
export PATH
cd /tmp
mt -f /dev/nrmt0 status > /dev/null
logger -t tape woken
sleep 2
echo "== status"
mt -f /dev/nrmt0 status
mt -f /dev/nrmt1 status

echo "== the host's tape"
tcopy /dev/rmt1
tar xf /dev/rmt1
cat hello bye
mt -f /dev/nrmt1 fsf 1
dd if=/dev/nrmt1 of=recs bs=8k 2>&1 | sed "/ transferred in /d"
wc -c < recs
od -c recs | sed -n 1p
echo "at the end:"
dd if=/dev/nrmt1 of=/dev/null bs=8k 2>&1 | sed "/ transferred in /d"
mt -f /dev/rmt1 rewind

echo "== the QIC tape: two archives, one after the other"
awk 'BEGIN { for (i = 1; i <= 3000; i++) print "line", i }' </dev/null >lines
awk 'BEGIN { for (i = 1; i <= 500; i++) print "more", i }' </dev/null >more
tar cf /dev/nrmt0 lines
tar cf /dev/nrmt0 more
mt -f /dev/nrmt0 rewind
tar tf /dev/nrmt0
mt -f /dev/nrmt0 fsf 1
tar tf /dev/nrmt0
echo "blocks: past the header, and back over one"
mt -f /dev/nrmt0 rewind
mt -f /dev/nrmt0 fsr 1
dd if=/dev/nrmt0 bs=512 count=1 2>/dev/null | sed -n 1p
mt -f /dev/nrmt0 bsr 1
dd if=/dev/nrmt0 bs=512 count=1 2>/dev/null | sed -n 2p
echo "reads of 10K: each file to its mark, and the end"
mt -f /dev/nrmt0 rewind
dd if=/dev/nrmt0 of=/dev/null bs=10k 2>&1 | sed "/ transferred in /d"
dd if=/dev/nrmt0 of=/dev/null bs=10k 2>&1 | sed "/ transferred in /d"
dd if=/dev/nrmt0 of=/dev/null bs=10k 2>&1 | sed "/ transferred in /d"
echo "a read that is not of whole blocks:"
dd if=/dev/rmt0 of=/dev/null bs=1000 count=1 2>&1 | sed "/ transferred in /d"
mkdir x
cd x
tar xf /dev/rmt0
cmp lines ../lines && echo "lines: the same"
cd ..

echo "== dump and restor through the QIC tape"
dump 0f /dev/rmt0 /dev/rsd0d > dump.out 2>&1
echo "dump: $?"
newfs sd0e > newfs.out 2>&1
echo "newfs: $?"
echo | restor rf /dev/rmt0 /dev/rsd0e > restor.out 2>&1
echo "restor: $?"
fsck -y /dev/rsd0e > fsck.out 2>&1
echo "fsck: $?"
mkdir d e
mount -r /dev/sd0d /tmp/d
mount /dev/sd0e /tmp/e
diff -r d e | sed "/^Common subdirectories: /d" > diff.out
test -s diff.out || echo "restored: the same"
cat diff.out
umount /dev/sd0d
umount /dev/sd0e

echo "== the DAT tape, labelled as ANSI has it"
ansitape cf /dev/rmt1 more
ansitape tf /dev/rmt1
mkdir y
cd y
ansitape xf /dev/rmt1
cmp MORE ../more && echo "MORE: the same, its name in capitals as VMS has it"
cd ..

echo "== the DAT tape, written"
tar cf /dev/rmt1 lines more
tar tf /dev/rmt1
tcopy /dev/rmt1

echo "== the QIC tape, taken out"
mt -f /dev/rmt0 offline
mt -f /dev/rmt0 status
echo "== end"
