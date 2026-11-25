#
# Public domain - 1997/1/10 - sms
#
#	@(#)Makefile	1.2 (2.11BSD) 2026/9/23
#
# This file is not intended to be run but rather to serve as a guide when
# (manually) creating a distribution image.  To guard against corrupting a
# live/running system DESTDIR is defined here to be something which does 
# *NOT* exist.

# The 286's: a new root as the system has it, which usr/src/GNUmakefile
# makes so where the system is built, with DESTDIR its own.  First the
# root's directories, each with its mode, owner and group (dirs, a line
# each), as 4.4BSD's distribution makes them first, so that what the
# Makefiles of /usr/src install has where to go; and after the files,
# the devices, by MAKEDEV, which makes what the machine has (all), as
# 4.4BSD's runs it.  Not the files of lpr, autoconfig, savecore and
# uucp, which the 286 has not: dtab, printcap, shutdownlog and uucp's
# (SUBDIR); nor rc.local, whose work its rc does.  /usr/adm, /var/run
# and /var/db are 755, where the owner may search them, and not 655.
# cron's log is made, and the database of the aliases, as newaliases
# makes it, without which each sendmail waits for someone to make it.

DESTDIR=/foobar
OWN=root
GRP=wheel

SUBDIR= root

FILES=	acctd.cf aliases crontab disktab fstab ftpusers gettytab group \
	hosts hosts.allow hosts.deny inetd.conf inetd.conf.nowrappers \
	motd netstart networks ntp.conf protocols rc rc.conf \
	remote sendmail.cf services shells syslog.conf ttys

# The phones file can contain sensitive information (see phones(5)) so is
# installed readable only by uucp/tip and group daemon.  A local site can use 
# less restrictive permissions.

PHONES= phones

# NOP targets
all install depend lint clean tags:

# this will populate /foobar with prototype /dev and /etc files
distribution:
	-@while read d m o g; do mkdir -p ${DESTDIR}$$d; \
		chmod $$m ${DESTDIR}$$d; chown $$o.$$g ${DESTDIR}$$d; done < dirs
	-@mkdir -p ${DESTDIR}/dev
	@chmod 755 ${DESTDIR}/dev
	-@mkdir -p ${DESTDIR}/etc
	@chmod 755 ${DESTDIR}/etc
	@chown ${OWN}.${GRP} ${DESTDIR}/dev ${DESTDIR}/etc
	-@mkdir -p ${DESTDIR}/usr/spool/mqueue
	@chmod 755 ${DESTDIR}/usr/spool/mqueue
	@chown ${OWN}.${GRP} ${DESTDIR}/usr/spool/mqueue

	install -c -o ${OWN} -g ${GRP} -m 644 ${FILES} ${DESTDIR}/etc
	newaliases -oA${DESTDIR}/etc/aliases

	install -c -m 440 -o uucp -g daemon ${PHONES} ${DESTDIR}/etc

	mkpasswd -p master.passwd
	install -c -o root -g wheel -m 600 master.passwd ${DESTDIR}/etc
	install -c -o root -g wheel -m 644 master.passwd.dir ${DESTDIR}/etc/passwd.dir
	install -c -o root -g wheel -m 644 master.passwd.pag ${DESTDIR}/etc/passwd.pag
	install -c -o root -g wheel -m 644 master.passwd.orig ${DESTDIR}/etc/passwd
	rm -f master.passwd.dir master.passwd.pag master.passwd.orig

	install -c -o ${OWN} -g ${GRP} -m 600 hosts.equiv ${DESTDIR}/etc
	install -c -o ${OWN} -g ${GRP} -m 555 MAKEDEV.local MAKEDEV \
		${DESTDIR}/dev
	install -c -o ${OWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/dumpdates
	install -c -o ${OWN} -g operator -m 664 /dev/null \
		${DESTDIR}/etc/sendmail.fc
	-@mkdir -p ${DESTDIR}/usr/adm ${DESTDIR}/var/run ${DESTDIR}/var/db
	@chmod 755 ${DESTDIR}/usr/adm ${DESTDIR}/var/run ${DESTDIR}/var/db
	@chown ${OWN}.${GRP} ${DESTDIR}/usr/adm ${DESTDIR}/var/run \
		${DESTDIR}/var/db
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/messages
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/daemonlog
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/debuglog
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/spool/mqueue/syslog
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/lpd-errs
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/var/run/utmp
	install -c -o nobody -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/var/db/find.codes
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/lastlog
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/wtmp
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/acct
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/usracct
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/savacct
	install -c -o uucp -g daemon -m 600 /dev/null \
	    ${DESTDIR}/usr/adm/aculog
	install -c -o ${OWN} -g ${GRP} -m 644 /dev/null \
	    ${DESTDIR}/usr/adm/cron
	-for i in ${SUBDIR}; do \
		(cd $$i; make DESTDIR=${DESTDIR} distribution); done
	cd ${DESTDIR}/dev; sh ./MAKEDEV all
