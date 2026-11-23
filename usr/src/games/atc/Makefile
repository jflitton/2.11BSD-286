#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.3	(2.11BSD)	2025/11/25
#

CFLAGS=	-O -DBSD -DDEST=\"/usr/games/lib/atc/\"
LDFLAGS= -i
YFLAGS=	-d
LIBS=	-ll -lm -lcurses -ltermcap
SRCS=	extern.c grammar.c graphics.c input.c lex.c list.c log.c \
	main.c tunable.c update.c
OBJS=	extern.o grammar.o input.o lex.o list.o log.o main.o tunable.o \
	graphics.o update.o

# Its page as well, as sail's Makefile has sail's.
all: atc atc.0

atc: ${OBJS}
	${CC} ${LDFLAGS} ${OBJS} ${LIBS} -o $@

atc.0: atc.6
	/usr/man/manroff atc.6 > atc.0

clean: FRC
	rm -f ${OBJS} core atc grammar.c y.tab.h y.tab.c lex.yy.c lex.c atc.0

depend: lex.c grammar.c FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	-[ -d /usr/games/lib/atc ] || mkdir /usr/games/lib/atc
	chmod 700 /usr/games/lib/atc
	chown games.staff /usr/games/lib/atc
	(cd games; install -c -o games -g staff -m 600 * /usr/games/lib/atc)
# There is no dm here to run it from /usr/games/hide.
	install -c -s -o games -g staff -m 4711 atc /usr/games/atc
	install -c -o root -g staff -m 444 atc.0 /usr/man/cat6/atc.0

lint: lex.c grammar.c FRC
	lint ${CFLAGS} ${SRCS}

tags: lex.c grammar.c FRC
	ctags ${SRCS}

FRC:
