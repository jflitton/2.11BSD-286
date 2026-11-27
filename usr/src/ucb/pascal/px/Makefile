#
#       #(@)Makefile    2.1 (2.11BSD) 2026/11/26
#
# The 80286: the interpreter is i286/as.s, the files of the PDP-11's
# here written again for the 286 in i286, with the getc and putc of the
# V6 library there too, which pxp has as well; E.inc is to it what E.s
# was, of i286/Emake.ed.  The C library's atof is the interpreter's,
# for 30atof.s.

AS=	as

DEFS=		-DHZ=60.0
CFLAGS=		-O -w
LDFLAGS=	-X -i
STACK=		-stack 16384

SRCS=	i286/as.s i286/00int.s i286/02rel.s i286/02relset.s i286/03bool.s \
	i286/04as.s i286/05lv.s i286/06add.s i286/07sub.s i286/10mul.s \
	i286/12div.s i286/13mod.s i286/14neg.s i286/16dvd.s i286/17ind.s \
	i286/17rv.s i286/20con.s i286/21rang.s i286/24case.s i286/24pxp.s \
	i286/25set.s i286/26for.s i286/27conv.s i286/30getname.s \
	i286/30io.s i286/30iosubs.s i286/30read.s i286/30write.s \
	i286/34fun.s i286/opcode.s

OTHERS= Edata i286/Emake.ed exit.c int.c palloc.c pcttot.c pdattim.c \
	perror.c pwrite.c gather.c

OBJS=	exit.o int.o palloc.o pcttot.o pdattim.o perror.o pwrite.o \
	getc.o putc.o

ALL=	px gather

all: ${ALL}

.c.o:
	${CC} ${CFLAGS} ${DEFS} -c $*.c

px: as.o ${OBJS}
	${CC} ${LDFLAGS} ${STACK} -o px as.o ${OBJS} -lm

gather: gather.o
	cc ${LDFLAGS} -o gather gather.o

as.o: E.inc ${SRCS}
	${AS} -o as.o i286/as.s

getc.o: i286/getc.s
	${AS} -o getc.o i286/getc.s

putc.o: i286/putc.s
	${AS} -o putc.o i286/putc.s

int.c palloc.c pcttot.c perror.c: E.h

E.inc E.h: Edata i286/Emake.ed
	ed - Edata < i286/Emake.ed

install: ${ALL}
	install -c -s -m 755 px /usr/ucb/px
	install -c -s -m 755 gather /usr/libexec/gather

tags: FRC

FRC:

clean:
	rm -f ${ALL} core E.h E.inc *.o tags
