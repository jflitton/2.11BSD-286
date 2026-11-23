#
# bench, this port's benchmark (docs/performance.md).
#
BINDIR=	/usr/local
CFLAGS=	-O
LDFLAGS= -i
PROGRAM= bench
SRCS=	bench.c
OBJS=	bench.o

all:	${PROGRAM}

${PROGRAM}: ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJS}

install: ${PROGRAM}
	install -c -s -m 755 ${PROGRAM} ${BINDIR}

clean:
	rm -f a.out core *.o ${PROGRAM}

depend:
	mkdep ${CFLAGS} ${SRCS}
