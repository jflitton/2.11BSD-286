#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.5 (2.11BSDs) 2025/11/29
#

SRCS=	inet_addr.c inet_network.c \
	inet_netof.c inet_ntoa.c inet_lnaof.c \
	inet_maddr.c
OBJS=	inet_addr.o inet_network.o \
	inet_netof.o inet_ntoa.o inet_lnaof.o \
	inet_maddr.o
CFLAGS=	-O ${DEFS}
TAGSFILE=tags

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

inetlib inetlib_m inetlib_l: ${OBJS}
	@echo "building medium model inetlib"
	@cd m; ar cru ../inetlib_m ${OBJS}
	@echo "building large model inetlib"
	@cd l; ar cru ../inetlib_l ${OBJS}
	@echo "building small model inetlib"
	@ar cru inetlib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f inetlib inetlib_m inetlib_l *.o m/*.o l/*.o errs a.out core \
		tags Makefile.bak

depend:	${SRCS}
	mkdep ${CFLAGS} ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

inet_addr.o: inet_addr.c /usr/include/sys/types.h /usr/include/sys/select.h
inet_addr.o: /usr/include/sys/types.h /usr/include/ctype.h
inet_addr.o: /usr/include/netinet/in.h
inet_network.o: inet_network.c /usr/include/sys/types.h
inet_network.o: /usr/include/sys/select.h /usr/include/sys/types.h
inet_network.o: /usr/include/netinet/in.h /usr/include/ctype.h
inet_netof.o: inet_netof.c /usr/include/sys/param.h
inet_netof.o: /usr/include/sys/localopts.h /usr/include/sys/stddef.h
inet_netof.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
inet_netof.o: /usr/include/sys/select.h /usr/include/sys/types.h
inet_netof.o: /usr/include/signal.h /usr/include/sys/types.h
inet_netof.o: /usr/include/netinet/in.h /usr/include/arpa/inet.h
inet_ntoa.o: inet_ntoa.c /usr/include/sys/types.h /usr/include/sys/select.h
inet_ntoa.o: /usr/include/sys/types.h /usr/include/netinet/in.h
inet_lnaof.o: inet_lnaof.c /usr/include/sys/types.h /usr/include/sys/select.h
inet_lnaof.o: /usr/include/sys/types.h /usr/include/netinet/in.h
inet_maddr.o: inet_maddr.c /usr/include/sys/types.h /usr/include/sys/select.h
inet_maddr.o: /usr/include/sys/types.h /usr/include/netinet/in.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
