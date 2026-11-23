#       @(#)Makefile    2.0 (2.11BSD) 2025/11/26

# The assembler that is made is the 80286's, which is in C, in i286.
# as0.s and as2.s are the PDP-11's, in its own assembly.
VPATH=	i286
CFLAGS=	-O
LDFLAGS= -i
OBJS=	as0.o as1.o as2.o as3.o
MAN=	as.0
MANSRC=	i286/as.1

all: as ${MAN}

as: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS}

${OBJS}: i286/as.h

${MAN}: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

install: all
	install -c -s -m 755 -o root as /bin
	install -c -o root -g staff -m 444 ${MAN} /usr/man/cat1

clean:
	rm -f *.o a.out as core errs ${MAN}
