# What tests/mx has the system run as root, with sendmail.MX installed
# as sendmail (run.sh).  Each part begins with a line "== NAME".
# A letter is shown less the lines that differ from one run to the
# next, as in tests/network.
w=/tmp/w
rm -rf $w; mkdir $w; cd $w
cat > letters <<'END'
sed -e 's/^\(From [^ ]*\) .*/\1/' -e '/^Received: /d' -e '/^[ 	]*id AA/d' \
    -e '/^Date: /d' -e '/^Message-Id: /d' $1
END
cat > wait <<'END'
# sh wait FILE N: wait up to a minute for FILE to hold N letters.
n=0
while test `cat $1 2>/dev/null | grep -c '^From '` -lt $2 -a $n -lt 60
do
	sleep 1; n=`expr $n + 1`
done
END
hostname pc286
rm -f /usr/spool/mail/root

echo "== local"
echo local-body | mail root
sh wait /usr/spool/mail/root 1
sh letters /usr/spool/mail/root
rm -f /usr/spool/mail/root

echo "== newaliases"
newaliases
echo alias-body | mail help
sh wait /usr/spool/mail/root 1
sh letters /usr/spool/mail/root
rm -f /usr/spool/mail/root

echo "== freeze"
sendmail -bz
test -s /etc/sendmail.fc && echo frozen
mv /etc/sendmail.cf cf
echo frozen-body | mail root
sh wait /usr/spool/mail/root 1
mv cf /etc/sendmail.cf
cp /dev/null /etc/sendmail.fc
sh letters /usr/spool/mail/root
rm -f /usr/spool/mail/root

# The domain test: relay.test has no address, and three MX records:
# the two preferred name gone.test and lost.test, which have none
# either, and the last mail.test, which is this machine.  The zone has
# them out of order.  sendmail says which it connects to from the third
# on, so it names mail.test only if it tried the others first.
# sendmail.cf has the local domain test, and relay.test as the relay,
# where mail for another machine goes.  sendmail's daemon is started
# while the machine is named mail, and sendmail sends while it is named
# pc286, so that each is another machine to the other: sendmail will
# not talk to itself.  To the daemon, root@mail.test is local.
echo "== mx"
cat > boot <<END
directory $w
primary test test.zone
primary 0.0.127.in-addr.arpa local.zone
END
cat > test.zone <<END
@	IN SOA pc286.test. root.pc286.test. ( 1 3600 300 3600000 3600 )
	IN NS pc286.test.
pc286	IN A 127.0.0.1
mail	IN A 127.0.0.1
relay	IN MX 30 mail.test.
relay	IN MX 10 gone.test.
relay	IN MX 20 lost.test.
END
cat > local.zone <<END
@	IN SOA pc286.test. root.pc286.test. ( 1 3600 300 3600000 3600 )
	IN NS pc286.test.
1	IN PTR localhost.
END
named -b $w/boot; sleep 3
sed -e 's/^DD.*/DDtest/' -e 's/^DA.*/DArelay.test/' /etc/sendmail.cf > cf
cp cf /etc/sendmail.cf
hostname mail
sendmail -bd
sleep 2
hostname pc286
echo mx-body | sendmail -v -odi root@mail.test | sed 's/ ready at .*/ ready/'
sh wait /usr/spool/mail/root 1
sh letters /usr/spool/mail/root
rm -f /usr/spool/mail/root

# A relay with no records at all is no host: the letter comes back at
# once, and is not queued to be tried again.
echo "== nohost"
sed 's/^DA.*/DAnosuch.test/' /etc/sendmail.cf > nohost.cf
echo nohost-body | sendmail -oem -odi -Cnohost.cf root@mail.test
mailq
sh wait /usr/spool/mail/root 1
sh letters /usr/spool/mail/root > bounce
grep '^Subject: ' bounce
grep '\.\.\. Host unknown' bounce

kill `cat /var/run/named.pid`
kill `ps ax | grep "[s]endmail" | awk '{print $1}'`
echo "== end"
