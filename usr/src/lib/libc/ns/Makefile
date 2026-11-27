#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.3 (2.11BSD) 2026/11/26
#

SRCS=	ns_addr.c ns_ntoa.c
OBJS=	ns_addr.o ns_ntoa.o
CFLAGS=	-O ${DEFS}
TAGSFILE=tags

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here; and
# for profiling, which is the small model's alone, with cc -p into
# profiled, as the PDP-11's; each has a library of its own.  The
# PDP-11's ld -x -r, which took out the local symbols, is not done: the
# 286's ld makes nothing but a program.

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $*.o profiled/$*.o
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

nslib nslib_p nslib_m nslib_l: ${OBJS}
	@echo "building profiled nslib"
	@cd profiled; ar cru ../nslib_p ${OBJS}
	@echo "building medium model nslib"
	@cd m; ar cru ../nslib_m ${OBJS}
	@echo "building large model nslib"
	@cd l; ar cru ../nslib_l ${OBJS}
	@echo "building small model nslib"
	@ar cru nslib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f nslib nslib_p nslib_m nslib_l *.o profiled/*.o m/*.o l/*.o errs a.out core \
		tags Makefile.bak

depend:	${SRCS}
	mkdep ${CFLAGS} ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

ns_addr.o: ns_addr.c /usr/include/sys/types.h /usr/include/sys/select.h
ns_addr.o: /usr/include/sys/types.h /usr/include/netns/ns.h
ns_ntoa.o: ns_ntoa.c /usr/include/sys/types.h /usr/include/sys/select.h
ns_ntoa.o: /usr/include/sys/types.h /usr/include/netns/ns.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
