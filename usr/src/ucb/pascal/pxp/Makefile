#
#       #(@)Makefile    2.2 (2.11BSD) 2026/11/26
#
# The 80286: printf, treen and yycopy are of C, in i286, for the PDP-11's
# printf.s, treen.s and yycopy.s; getc and putc are px's, for the 286.

ED=	-ed
AS=	as
EYACC=	../eyacc/eyacc

DEFS=		-DPXP
CFLAGS=		-O -w
LDFLAGS=	-X -i

HDRS=	0.h opcode.h tree.h yy.h

SRCS=	TRdata.c Version.c call.c case.c const.c cset.c error.c fdec.c \
	func.c hash.c lab.c lval.c main.c nl.c pmon.c pp.c proc.c rec.c \
	rval.c stat.c string.c subr.c tree.c type.c var.c yycomm.c yycosts.c \
	yyerror.c yyget.c yyid.c yylex.c yymain.c yypanic.c yyparse.c \
	yyprint.c yyput.c yyrecover.c yyseman.c yytree.c

OTHERS=	gram.ex how_pxp pas.y i286/printf.c i286/treen.c i286/yycopy.c \
	../px/i286/getc.s ../px/i286/putc.s

OBJS=	TRdata.o Version.o call.o case.o const.o cset.o error.o fdec.o \
	func.o hash.o lab.o lval.o main.o nl.o pmon.o pp.o proc.o rec.o \
	rval.o stat.o string.o subr.o tree.o type.o var.o yycomm.o yycosts.o \
	yyerror.o yyget.o yyid.o yylex.o yymain.o yypanic.o yyparse.o \
	yyprint.o yyput.o yyrecover.o yyseman.o yytree.o

OBJS2=	printf.o treen.o yycopy.o y.tab.o getc.o putc.o

all: pxp

.c.o:
	${CC} ${CFLAGS} ${DEFS} -c $*.c

printf.o: i286/printf.c
	${CC} ${CFLAGS} ${DEFS} -c i286/printf.c

treen.o: i286/treen.c
	${CC} ${CFLAGS} ${DEFS} -c i286/treen.c

yycopy.o: i286/yycopy.c y.tab.h
	${CC} ${CFLAGS} ${DEFS} -c i286/yycopy.c

getc.o: ../px/i286/getc.s
	${AS} -o getc.o ../px/i286/getc.s

putc.o: ../px/i286/putc.s
	${AS} -o putc.o ../px/i286/putc.s

pxp: y.tab.h ${OBJS} ${OBJS2}
	${CC} ${CFLAGS} ${DEFS} -c Version.c
	${CC} ${LDFLAGS} -o pxp ${OBJS} ${OBJS2} Version.o

y.tab.c y.tab.h: pas.y
	${EYACC} -v pas.y
	${ED} - y.tab.c < gram.ex

install: pxp how_pxp
	install -c -s -m 755 pxp /usr/ucb/pxp
	install -c -m 644 how_pxp /usr/share/pascal/how_pxp

tags: FRC
	ctags ${HDRS} ${SRCS}

FRC:

clean:
	rm -f pxp pi1.2strings a.out core y.tab.c y.tab.h y.output *.o x* tags
