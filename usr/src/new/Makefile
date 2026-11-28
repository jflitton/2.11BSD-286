#       @(#)Makefile    2.2   (2.11BSD)  2026/11/27

MANDIR= /usr/new/man

CFLAGS=	-O

#
# Default programs that are installed
#
STDINST= ansi jove la vmsbackup
SUBDIR= ansi jove la vmsbackup

#
# Special purpose software that requires custom installation or may not
# be needed by all sites.
#
OPTDIR=	kermit5.188 rn nntp

all:	${STDINST}

${STDINST}: FRC
	cd $@; make

FRC:

install:
	-mkdir ${MANDIR}
	-chmod a+r ${MANDIR}
	-for i in 1 2 3 4 5 6 7 8; do \
		(mkdir ${MANDIR}/cat$$i; chmod a+r ${MANDIR}/cat$$i); done
	-for i in ${STDINST}; do \
		(cd $$i; make install); done
	-/usr/sbin/makewhatis ${MANDIR}

clean:
	for i in ${SUBDIR}; do (cd $$i; make clean); done

depend:
tags:
lint:
