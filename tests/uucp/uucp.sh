# What tests/uucp has root do.  The system calls itself twice: as loop,
# by TCP, where inetd starts uucpd, which logs the call in as Uloop and
# starts uucico, and the two talk the t protocol; and as ploop, by a
# direct line that is a pty, ptyp7, where a uucico waits at the other
# end, ttyp7, and the two talk g, the protocol of serial lines.  By loop
# a file and a program go, and a letter by uux to rmail, with the From
# line that sendmail's uucp mailer gives it; by ploop a program goes and
# a file comes back.  uucp and uux are given -r, to queue the work and
# not start a uucico to do it, so that it is done by the one this
# starts.  The uucico that answers ploop starts after the one that
# calls has the line, as at the far end of a real line, where it starts
# only once the caller has dialled and logged in: started first, it
# could say who it is before the caller had set the line's modes, which
# throws away what the line holds.  Last, clean.daily moves the logs to
# its archive, as /usr/adm/daily has it do each night, as root, and
# uucp, which is not root, must still be able to write the new log.
cd /
case "$1" in
"")
	echo "== names"
	uuname
	uuname -l
	ls /usr/spool/uucp

	echo "== tcp"
	uucp -r -C /etc/motd 'loop!~/motd'
	uucp -r /bin/sh 'loop!~/sh'
	(echo 'From root Thu Nov 26 00:00:00 2026 remote from example'
	 echo hello by uux and rmail) | uux -r - 'loop!rmail' root
	uusnap
	/usr/sbin/uucico -r1 -sloop
	sleep 10
	cmp /etc/motd /usr/spool/uucppublic/motd && echo motd sent
	cmp /bin/sh /usr/spool/uucppublic/sh && echo sh sent
	sed -n -e 's/^From \([^ ]*\) .*/From \1/p' -e '/^hello/p' \
	    /usr/spool/mail/root

	echo "== g"
	uucp -r /bin/csh 'ploop!~/csh'
	uucp -r 'ploop!~/motd' /usr/spool/uucppublic/motd.back
	/usr/sbin/uucico -r1 -sploop &
	sleep 5
	/usr/sbin/uucico </dev/ttyp7 >/dev/ttyp7 2>/dev/null
	wait
	cmp /bin/csh /usr/spool/uucppublic/csh && echo csh sent
	cmp /etc/motd /usr/spool/uucppublic/motd.back && echo motd fetched
	uusnap
	echo "== log"
	uulog -sloop | sed 's/ ([^)]*)//' | sort
	uulog -sploop | sed 's/ ([^)]*)//' | sort
	echo "== clean"
	/etc/uucp/clean.daily
	ls /usr/spool/uucp/uucplog.archives
	ls -lg /usr/spool/uucp/LOGFILE | sed 's/ [A-Z][a-z][a-z] .*//'
	uucp -r /etc/group 'loop!~/group'
	uulog -sloop | sed 's/ ([^)]*)//'
	echo "== end"
	# How long each file took, which is not compared.
	cat /usr/spool/uucp/uucplog.archives/SYSLOG.0
	;;
esac
