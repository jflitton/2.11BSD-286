#
# Place in the public domain 1996/12/13 - sms.
#
# @(#)Makefile 1.1 (2.11BSD) 2025/11/25
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, where the PDP-11's profiled one
# went in profiled, and the small's left here; each model has a library
# of its own, which is installed where cc links a program of that model
# with it, /usr/lib, /usr/lib/m and /usr/lib/l.  The PDP-11's ld -x -r,
# which took out the local symbols, is not done: the 286's ld makes
# nothing but a program.
#

LIBDIR=/usr/lib
INCDIR=/usr/include
MANDIR=/usr/man/cat3

CFLAGS=-O -DSIGRETURNTYPE=int -DHAVE_ANSIHEADERS

LFLAGS=-i
LIB= libident.a

SRCS = ident.c id_open.c id_close.c id_query.c id_parse.c support.c version.c
OBJS = ident.o id_open.o id_close.o id_query.o id_parse.o support.o version.o

.c.o:
	@${CC} -mm ${CFLAGS} -c $*.c
	@mv $*.o m/$*.o
	@${CC} -ml ${CFLAGS} -c $*.c
	@mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

all: 	libident.a libident_m.a libident_l.a lookup-tester ident-tester ident.0

libident.a libident_m.a libident_l.a:	$(OBJS)
	@echo building small model libident
	@ar ru libident.a ${OBJS}
	ranlib libident.a
	@echo building medium model libident
	@cd m; ar ru ../libident_m.a ${OBJS}
	ranlib libident_m.a
	@echo building large model libident
	@cd l; ar ru ../libident_l.a ${OBJS}
	ranlib libident_l.a

ident-tester: libident.a ident-tester.o
	${CC} $(CFLAGS) ${LFLAGS} -o ident-tester ident-tester.o $(LIB)

lookup-tester: libident.a lookup-tester.o
	${CC} $(CFLAGS) ${LFLAGS} -o lookup-tester lookup-tester.o $(LIB)

ident.0: ident.3
	/usr/man/manroff ident.3 > ident.0

install: all
	install -c -m 644  libident.a $(LIBDIR)/libident.a
	install -c -m 644 libident_m.a $(LIBDIR)/m/libident.a
	install -c -m 644 libident_l.a $(LIBDIR)/l/libident.a
	ranlib $(LIBDIR)/libident.a $(LIBDIR)/m/libident.a \
		$(LIBDIR)/l/libident.a
	install -c -m 444  ident.h $(INCDIR)/ident.h
	install -c -o root -g staff -m 444 ident.0 $(MANDIR)/ident.0

clean:
	-rm -f libident.a libident_m.a libident_l.a ident-tester lookup-tester \
		*.o ident.0
	-rm -f m/*.o l/*.o

depend:
	mkdep ${CFLAGS} ${SRCS}
