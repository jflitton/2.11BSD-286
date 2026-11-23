#       @(#)Makefile    2.3 (2.11BSD) 2025/11/26

# The linker that is made is the 80286's, in i286. ld.c here is the
# PDP-11's, which has its tables in a file (libvmf).
SRCS=	i286/ld.c
OBJS=	ld.o
MAN=	ld.0
MANSRC=	i286/ld.1
CFLAGS=	-O

# a.out(5), the 286's, which is what ld makes: its page is here, as ar's
# is with ar.
all:	ld ld.0 a.out.0

ld: ${OBJS}
	cc -i -o ld ${OBJS}

ld.o: i286/ld.c
	${CC} ${CFLAGS} -c i286/ld.c

install: all
	install -c -s -o root -g staff -m 755 ld /bin/ld
	install -c -o root -g staff -m 444 ${MAN} /usr/man/cat1
	install -c -o root -g staff -m 444 a.out.0 /usr/man/cat5

lint:
	lint -havx ${SRCS}

clean:
	rm -f ${OBJS} ${MAN} a.out.0 ld

${MAN}: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

a.out.0: i286/a.out.5
	/usr/man/manroff i286/a.out.5 > a.out.0
