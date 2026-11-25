#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.5 (2.11BSD) 2025/11/25
#
LDFLAGS= -i
XSTR=	xstr
YACC=	yacc
SRCS =	docmd.c expand.c gram.y lookup.c main.c server.c
OBJS =	docmd.o expand.o gram.o lookup.o main.o server.o
LINT =	lint -ha
CFLAGS= -O

.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} ${CFLAGS} -c x.c
	mv -f x.o $*.o
	rm -f x.c

# The 80286: without the PDP-11's libstubs, whose ctime and the rest
# asked ctimed for them; the C library's own are linked in.
rdist:	${OBJS} strings.o
	${CC} ${LDFLAGS} -o rdist ${OBJS} strings.o

${OBJS}:  defs.h

strings.o: strings
	${XSTR}
	${CC} -c xs.c
	mv -f xs.o strings.o
	rm -f xs.c

gram.c: gram.y
	${YACC} gram.y
	mv y.tab.c gram.c

clean:
	rm -f ${OBJS} gram.c errs rdist strings strings.o x.c xs.c y.tab.c

install: rdist
	install -c -s -m 4751 rdist /usr/ucb/rdist

lint:	docmd.c expand.c gram.c lookup.c main.c server.c
	${LINT} docmd.c expand.c gram.c lookup.c main.c server.c

print:	${SRCS}
	lpr -p ${SRCS} defs.h
