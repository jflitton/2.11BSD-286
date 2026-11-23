# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2.2 (2.11BSD) 2025/11/27
#
# curses package
#
CFILES=	addch.c addstr.c box.c clear.c clrtobot.c clrtoeol.c cr_put.c \
	cr_tty.c curses.c delch.c deleteln.c delwin.c endwin.c erase.c \
	fullname.c getch.c getstr.c idlok.c id_subwins.c initscr.c insch.c \
	insertln.c longname.c move.c mvprintw.c mvscanw.c mvwin.c newwin.c \
	overlay.c overwrite.c printw.c putchar.c refresh.c scanw.c scroll.c \
	toucholap.c standout.c touchwin.c tstp.c unctrl.c
OBJS=	addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o cr_put.o \
	cr_tty.o curses.o delch.o deleteln.o delwin.o endwin.o erase.o \
	fullname.o getch.o getstr.o idlok.o id_subwins.o initscr.o insch.o \
	insertln.o longname.o move.o mvprintw.o mvscanw.o mvwin.o newwin.o \
	overlay.o overwrite.o printw.o putchar.o refresh.o scanw.o scroll.o \
	toucholap.o standout.o touchwin.o tstp.o unctrl.o

CTAGS=	ctags
CFLAGS=	-O $(DEFS) -DNOSCCS
TAGSFILE=tags

# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own, which is installed where cc links a program of that model
# with it.  The PDP-11's ld -x -r, which took out the local symbols, is
# not done: the 286's ld makes nothing but a program.

.c.o:
	${CC} -c -mm ${CFLAGS} $*.c
	mv $*.o m/$*.o
	${CC} -c -ml ${CFLAGS} $*.c
	mv $*.o l/$*.o
	${CC} -c ${CFLAGS} $*.c

libcurses libcurses_m libcurses_l: ${OBJS}
	@echo building medium model libcurses
	@cd m; ar ru ../libcurses_m ${OBJS}
	ranlib libcurses_m
	@echo building large model libcurses
	@cd l; ar ru ../libcurses_l ${OBJS}
	ranlib libcurses_l
	@echo building small model libcurses
	@ar ru libcurses ${OBJS}
	ranlib libcurses

install: libcurses libcurses_m libcurses_l
	install -c -m 644 libcurses /usr/lib/libcurses.a
	ranlib /usr/lib/libcurses.a
	install -c -m 644 libcurses_m /usr/lib/m/libcurses.a
	ranlib /usr/lib/m/libcurses.a
	install -c -m 644 libcurses_l /usr/lib/l/libcurses.a
	ranlib /usr/lib/l/libcurses.a

tags:
	cwd=`pwd`; \
	for i in ${CFILES}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} m/*.o l/*.o a.out core test errs \
	    libcurses libcurses_m libcurses_l tags

ctags:
	${CTAGS} ${CFILES} curses.h

lint:
	lint -hxb ${CFILES} -lcurses > lint.out

test:	libcurses test.o
	${CC} ${LDFLAGS} ${CFLAGS} -o test test.o libcurses -ltermlib

test.o:	test.c
	${CC} ${CFLAGS} -c test.c
