#	Makefile	5.11.5 (2.11BSD)	2026/2/2
#
# LIBS can be set to reference other object modules and libraries needed
# by uucico, such as -lunet (needed when UNET is defined).
#
# Common object files are assembled into a library {uulib.a}
#
# The 80286: not acucntrl, which turns a DZ11's or a DH11's line between
# dialling in and dialling out through the driver's structures in
# /dev/kmem, the PDP-11's and the VAX's, and which uucico runs only when
# DIALINOUT is set in uucp.h, as it is not.
ALL=	uucico uucp uux uuxqt uulog uuclean uuname uupoll uusnap \
	uuencode uudecode uusend uucpd uuq
# Common object files placed in uulib.a
COMMON=	anlwrk.o anyread.o cfgets.o chkpth.o \
	cpmv.o expfile.o gename.o getargs.o getprm.o \
	getpwinfo.o gnamef.o gnxseq.o lastpart.o logent.o mailst.o \
	prefix.o subdir.o systat.o \
	ulockf.o uucpname.o versys.o xqt.o uucpdefs.o
SRCS=	${USRCS} ${OSRCS} ${LSRCS}
USRCS=	cico.c cntrl.c conn.c condevs.c gnsys.c pk0.c chksum.c pk1.c tio.c \
	fio.c gio.c imsg.c 
OSRCS=	uuclean.c uucp.c uulog.c uuname.c uupoll.c \
	uusend.c uusnap.c uux.c uuxqt.c uuq.c acucntrl.c
LSRCS=	anlwrk.c anyread.c assert.c cfgets.c chkpth.c \
	cpmv.c expfile.c \
	gename.c getargs.c getprm.c getpwinfo.c \
	gnamef.c gnxseq.c \
	lastpart.c logent.c mailst.c prefix.c \
	subdir.c systat.c ulockf.c \
	uucpdefs.c uucpname.c versys.c xqt.c
OWNER=	uucp
GROUP=	daemon
# The 80286: uucp is of the medium model, uucico's code being more than
# a segment of 64K, where the PDP-11 had it in overlays; the others are
# linked with uulib.a, as uucico is, and so are of its model.
MODEL=	-mm
CFLAGS=	-O ${MODEL}
LDFLAGS=-i ${MODEL}
LIBS= 
SBINDIR=	/usr/sbin
LIBEXECDIR=	/usr/libexec
BIN=	/usr/bin
PUBDIR=	/usr/spool/uucppublic
SPOOL=	/usr/spool/uucp
XQTDIR=	${SPOOL}/XTMP
CORRUPT= ${SPOOL}/CORRUPT
AUDIT=	${SPOOL}/AUDIT
LCK= ${SPOOL}/LCK
LOG= ${SPOOL}/LOG
STST=	${SPOOL}/STST
# The six subdirs {not counting XTMP}.
HOSTNAME=`uuname -l | sed 's/\(.......\).*/\1/'`
SUBDIRS=C. D.${HOSTNAME}X D.${HOSTNAME} D. X. TM.

all:	${ALL}

acucntrl.0: acucntrl.8
	/usr/man/manroff acucntrl.8 > acucntrl.0

uucp:	uucp.o assert.o uulib.a
	${CC} -o uucp ${LDFLAGS} uucp.o assert.o uulib.a ${LIBS}

uux:	uux.o assert.o uulib.a
	${CC} -o uux ${LDFLAGS} uux.o assert.o uulib.a ${LIBS}

uuxqt:	uuxqt.o assert.o uulib.a
	${CC} -o uuxqt ${LDFLAGS} uuxqt.o assert.o uulib.a ${LIBS}

# Previously "egrep -s" was used to search uucp.h but the exit status of the 
# "if" had to be ignored so make would not think an error had happened.  This
# had the side effect of ignoring the "ld: too big for type 431" error and
# installing a corrupt executable
#
# If anyone wants to build a non networking uucico then connect this block
# and uncomment the block below
uucico: cico.o cntrl.o conn.o condevs.o gnsys.o pk0.o chksum.o pk1.o tio.o \
	fio.o gio.o imsg.o assert.o uulib.a aculib/aculib.a
	${CC} -o uucico ${LDFLAGS} cico.o cntrl.o condevs.o \
		gnsys.o pk0.o chksum.o tio.o fio.o imsg.o gio.o assert.o \
		pk1.o conn.o uulib.a aculib/aculib.a ${LIBS}

uulog:	uulog.o assert.o uulib.a
	${CC} -o uulog ${LDFLAGS} uulog.o assert.o uulib.a ${LIBS}

uuclean: uuclean.o assert.o uulib.a
	${CC} -o uuclean ${LDFLAGS} uuclean.o assert.o uulib.a ${LIBS}

uuname:	uuname.o assert.o uulib.a
	${CC} -o uuname ${LDFLAGS} uuname.o assert.o uulib.a ${LIBS}

uupoll:	uupoll.o assert.o uulib.a
	${CC} -o uupoll ${LDFLAGS} uupoll.o assert.o uulib.a ${LIBS}

uusnap:	uusnap.o
	${CC} -o uusnap ${LDFLAGS} uusnap.o ${LIBS}

uuencode: uuencode.o
	${CC} -o uuencode ${LDFLAGS} uuencode.o ${LIBS}

uudecode: uudecode.o
	${CC} -o uudecode ${LDFLAGS} uudecode.o ${LIBS}

uusend: uusend.o
	${CC} -o uusend ${LDFLAGS} uusend.o uulib.a ${LIBS}

uucpd: uucpd.o uucp.h
	${CC} -o uucpd ${LDFLAGS} uucpd.o ${LIBS}

uuq: uuq.o uulib.a
	${CC} -o uuq ${LDFLAGS} uuq.o uulib.a ${LIBS}

acucntrl: acucntrl.o
	${CC} -o acucntrl ${LDFLAGS} acucntrl.o ${LIBS}

mkdirs:
	-for i in ${DESTDIR}${SPOOL} ${DESTDIR}${PUBDIR} ${DESTDIR}${XQTDIR} \
		${DESTDIR}${CORRUPT} ${DESTDIR}${AUDIT} ${DESTDIR}${LCK} \
		${DESTDIR}${LOG} ${DESTDIR}${STST} ; do \
		rm -f $$i ; mkdir $$i ; \
		chmod 755 $$i; chown ${OWNER} $$i; chgrp ${GROUP} $$i ; \
	done
	chmod 777 ${DESTDIR}${PUBDIR} ${DESTDIR}${LCK}
	-cd ${DESTDIR}${LOG}; for i in uucico uux uuxqt uucp xferstats; do\
		rm -f $$i ; mkdir $$i ; \
		chmod 755 $$i; chown ${OWNER} $$i; chgrp ${GROUP} $$i ; \
	done
	-cd ${DESTDIR}${SPOOL}; for i in ${SUBDIRS} ; do \
		mkdir $$i; \
		chown ${OWNER} $$i; chgrp ${GROUP} $$i; chmod 755 $$i; \
	done

uulib.a: ${COMMON}
	rm -f $@
	ar cr $@ ${COMMON}
	ranlib $@

aculib/aculib.a:
	cd aculib; make

install: mkdirs ${ALL}
	for i in uucico uuclean; do \
		install -c -s -m 6555 -o ${OWNER} -g ${GROUP} $$i ${DESTDIR}${SBINDIR} ;\
	done
	for i in uucp uux uulog uuname uusnap uupoll uuq uusend; do \
		install -c -s -m 6555 -o ${OWNER} -g ${GROUP} $$i ${DESTDIR}${BIN} ;\
	done
	rm -f ${DESTDIR}${BIN}/ruusend
	ln ${DESTDIR}${BIN}/uusend ${DESTDIR}${BIN}/ruusend
	install -c -s -m 6550 -o ${OWNER} -g ${GROUP} uuxqt ${DESTDIR}${LIBEXECDIR}
	install -c -s -m 500 -o uucp uucpd ${DESTDIR}${LIBEXECDIR}
	@echo Dont forget to fork off /usr/libexec/uucpd in /etc/rc.local
	@echo if you are not running 'inetd'
	install -s uuencode ${DESTDIR}${BIN}
	install -s uudecode ${DESTDIR}${BIN}

clean:
	rm -f *.o ${ALL} uulib.a *.0
	cd aculib; make clean

depend:
	mkdep ${CFLAGS} *.c
