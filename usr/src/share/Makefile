#
# Public domain - 1996/10/21 - sms
#
#	@(#)Makefile	1.2 (2.11BSD) 2025/11/25
#
CFLAGS=	-O

# The 286's: not font and fontinfo, the typesetter's and the Versatec's,
# which it has no troff nor vtroff for.
SUBDIR=	adb learn me misc ms tmac termcap zoneinfo

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS}

FRC:

install: FRC
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} install); done

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
