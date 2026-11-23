#
# ve, this port's editor (ve.1).  It keeps the file it edits in segments
# it asks the system for (segalloc(2)), and is of the medium model for
# its code alone.  fmem.s is assembly with the C library's conventions,
# which cc gives it.
#
BINDIR=	/usr/local
CFLAGS=	-O -mm
LDFLAGS= -i
PROGRAM= ve
SRCS=	main.c term.c screen.c text.c edit.c normal.c insert.c search.c ex.c
COBJS=	main.o term.o screen.o text.o edit.o normal.o insert.o search.o ex.o
OBJS=	${COBJS} fmem.o
MAN=	ve.0
LIBS=	-ltermcap

all:	${PROGRAM} ${MAN}

${PROGRAM}: ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

${COBJS}: ve.h

fmem.o: fmem.s
	${CC} ${CFLAGS} -c fmem.s

install: ${PROGRAM} ${MAN}
	install -c -s -m 755 ${PROGRAM} ${BINDIR}
	install -c -m 444 -o root -g staff ${MAN} /usr/local/man/cat1

${MAN}: ve.1
	/usr/man/manroff ve.1 > ${MAN}

clean:
	rm -f a.out core *.o ${MAN} ${PROGRAM}

depend:
	mkdep ${CFLAGS} ${SRCS}
