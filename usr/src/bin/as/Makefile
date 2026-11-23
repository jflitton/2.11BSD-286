#       @(#)Makefile    2.0 (2.11BSD) 2025/11/26

# The assembler that is made is the 80286's, which is in C, in i286.
# as0.s and as2.s are the PDP-11's, in its own assembly.
VPATH=	i286
CFLAGS=	-O
LDFLAGS= -i
OBJS=	as0.o as1.o as2.o as3.o

all: as

as: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS}

${OBJS}: i286/as.h

install: all
	install -c -s -m 755 -o root as /bin

clean:
	rm -f *.o a.out as core errs
