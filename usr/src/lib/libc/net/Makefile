#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	5.18 (2.11BSD) 2026/11/26
#
SRCS=	getnbyaddr.c getnbyname.c getnent.c getpent.c getpname.c	\
	getproto.c getsbyname.c getsbyport.c getsent.c herror.c 	\
	rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c		\
	res_query.c res_send.c	\
	rexec.c ruserpass.c

OBJS=	getnbyaddr.o getnbyname.o getnent.o getpent.o getpname.o	\
	getproto.o getsbyname.o getsbyport.o getsent.o rcmd.o herror.o	\
	rcmd.o res_comp.o res_debug.o res_init.o res_mkquery.o		\
	res_query.o res_send.o rexec.o ruserpass.o

CFLAGS=	-O ${DEFS}

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

netlib netlib_p netlib_m netlib_l: ${OBJS}
	@echo "building profiled netlib"
	@cd profiled; ar cru ../netlib_p ${OBJS}
	@echo "building medium model netlib"
	@cd m; ar cru ../netlib_m ${OBJS}
	@echo "building large model netlib"
	@cd l; ar cru ../netlib_l ${OBJS}
	@echo "building small model netlib"
	@ar cru netlib ${OBJS}

clean:
	rm -f *.o profiled/*.o m/*.o l/*.o errs a.out core netlib netlib_p netlib_m netlib_l tags

depend:
	mkdep ${CFLAGS} ${SRCS}
