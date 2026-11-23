#
# Public Domain.  1996/3/20 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD) 2025/11/27
#
# The 80286: the library is one for each of its three models, installed
# where cc links a program of that model with it, /usr/lib, /usr/lib/m
# and /usr/lib/l.  Where the PDP-11's is the object that ld -r -X
# makes, the 286's ld makes nothing but a program: it is a library of
# ar's, with the table of ranlib's that the 286's ld looks in.
#
CFLAGS=-O
TAGSFILE=tags

all: liberrlst.a liberrlst_m.a liberrlst_l.a

liberrlst.a: errlst.c
	${CC} -c ${CFLAGS} errlst.c
	ar cr liberrlst.a errlst.o
	ranlib liberrlst.a
	rm -f errlst.o

liberrlst_m.a: errlst.c
	${CC} -mm -c ${CFLAGS} errlst.c
	ar cr liberrlst_m.a errlst.o
	ranlib liberrlst_m.a
	rm -f errlst.o

liberrlst_l.a: errlst.c
	${CC} -ml -c ${CFLAGS} errlst.c
	ar cr liberrlst_l.a errlst.o
	ranlib liberrlst_l.a
	rm -f errlst.o

install: all
	install -c -m 644 liberrlst.a /usr/lib
	install -c -m 644 liberrlst_m.a /usr/lib/m/liberrlst.a
	install -c -m 644 liberrlst_l.a /usr/lib/l/liberrlst.a

tags:
	ctags -a -f ${TAGSFILE} `pwd`/errlst.c

clean:
	rm -f *.o liberrlst.a liberrlst_m.a liberrlst_l.a
