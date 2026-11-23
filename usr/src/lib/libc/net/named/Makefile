#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.5 (2.11BSD) 2025/11/25
#

OBJS=	gethnamadr.o sethostent.o

SRCS=	gethnamadr.c sethostent.c

CFLAGS= -O ${DEFS}
TAGSFILE= tags

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own.  The PDP-11's ld -x -r, which took out the local symbols,
# is not done: the 286's ld makes nothing but a program.

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

hostlib hostlib_m hostlib_l: ${OBJS}
	@echo "building medium model hostlib"
	@cd m; ar cru ../hostlib_m ${OBJS}
	@echo "building large model hostlib"
	@cd l; ar cru ../hostlib_l ${OBJS}
	@echo "building small model hostlib"
	@ar cru hostlib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f *.o errs a.out core hostlib hostlib_m hostlib_l m/*.o l/*.o tags

depend:
	mkdep ${CFLAGS} ${SRCS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

gethnamadr.o: gethnamadr.c /usr/include/sys/param.h
gethnamadr.o: /usr/include/sys/localopts.h /usr/include/machine/machparam.h
gethnamadr.o: /usr/include/sys/types.h /usr/include/signal.h
gethnamadr.o: /usr/include/sys/types.h /usr/include/sys/socket.h
gethnamadr.o: /usr/include/netinet/in.h /usr/include/ctype.h
gethnamadr.o: /usr/include/netdb.h /usr/include/stdio.h /usr/include/errno.h
gethnamadr.o: /usr/include/arpa/inet.h /usr/include/arpa/nameser.h
gethnamadr.o: /usr/include/resolv.h
sethostent.o: sethostent.c /usr/include/sys/types.h /usr/include/arpa/nameser.h
sethostent.o: /usr/include/netinet/in.h /usr/include/resolv.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
