#	@(#)Makefile	4.3 (2.11BSD) 2025/11/27
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here; each
# model has a library of its own, which is installed where cc links a
# program of that model with it, /usr/lib, /usr/lib/m and /usr/lib/l.
#
CFLAGS = -O
SRCS=	allprint.c main.c reject.c yyless.c yywrap.c
OBJS=	allprint.o main.o reject.o yyless.o yywrap.o
TAGSFILE=tags

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

all: libln.a

libln.a: ${OBJS}
	ar rc libln.a ${OBJS}
	cd m; ar rc ../libln_m.a ${OBJS}
	cd l; ar rc ../libln_l.a ${OBJS}

install: all
	install -c -m 644 -o root libln.a /usr/lib
	rm -f /usr/lib/libl.a
	ln /usr/lib/libln.a /usr/lib/libl.a
	ranlib /usr/lib/libln.a
	install -c -m 644 -o root libln_m.a /usr/lib/m/libln.a
	rm -f /usr/lib/m/libl.a
	ln /usr/lib/m/libln.a /usr/lib/m/libl.a
	ranlib /usr/lib/m/libln.a
	install -c -m 644 -o root libln_l.a /usr/lib/l/libln.a
	rm -f /usr/lib/l/libl.a
	ln /usr/lib/l/libln.a /usr/lib/l/libl.a
	ranlib /usr/lib/l/libln.a

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} m/*.o l/*.o libln.a libln_m.a libln_l.a tags
