#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6.4 (2.11BSD) 2025/11/29
#
#	crt0	Normal C run time startoff
#
# The 286's, after the PDP-11's ../../pdp/csu: for each model, the
# small's here, the medium's in m and the large's in l, which cc links a
# program of that model with first, from /lib, /lib/m and /lib/l.  It
# has no profiling, and so no mcrt0.
#
SRCS=	crt0.s
OBJS=	crt0.o
AS=	as

all: ${OBJS}

install: ${OBJS}
	install -c -m 0644 crt0.o /lib/crt0.o
	install -c -m 0644 m/crt0.o /lib/m/crt0.o
	install -c -m 0644 l/crt0.o /lib/l/crt0.o

crt0.o:	crt0.s ../crt/DEFS.inc
	${AS} -mm -DFAR -I../crt -o m/crt0.o crt0.s
	${AS} -ml -DFAR -DFARDATA -I../crt -o l/crt0.o crt0.s
	${AS} -ms -I../crt -o crt0.o crt0.s

clean:
	rm -f ${OBJS} m/*.o l/*.o
