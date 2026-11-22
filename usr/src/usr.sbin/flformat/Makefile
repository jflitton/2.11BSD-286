#
# Public Domain.  rxformat's, for flformat.
#
#	@(#)Makefile	1.0 (2.11BSD) 2026/11/22
#
CFLAGS=	 -O
LDFLAGS= -i
SRCS=	flformat.c
OBJS=	flformat.o
MAN=	flformat.0
MANSRC=	flformat.8

all: flformat flformat.0

flformat: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS}

flformat.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} flformat tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: flformat
	install -c -o root -g staff -m 444 ${MAN} /usr/man/cat8
	install -c -s -o root -g staff -m 755 flformat /usr/sbin/flformat

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
