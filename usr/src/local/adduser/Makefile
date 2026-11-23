#
# adduser, this port's, which does what upstream's adduser(8) says to
# do by hand (adduser.c).
#
BINDIR=	/usr/local
CFLAGS=	-O
LDFLAGS= -i
PROGRAM= adduser
SRCS=	adduser.c
OBJS=	adduser.o

all:	${PROGRAM}

${PROGRAM}: ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJS}

install: ${PROGRAM}
	install -c -s -m 755 ${PROGRAM} ${BINDIR}

clean:
	rm -f a.out core *.o ${PROGRAM}

depend:
	mkdep ${CFLAGS} ${SRCS}
