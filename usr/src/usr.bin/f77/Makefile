#       Makefile        2.3 (2.11BSD) 2026/11/26

# Fortran 77 compiler generating code for the PDP11 using the Ritchie pass 2

#	1996/10/25 - 2.11BSD
#	2020/2/8 - remove unused malloc.c from SRCS
#		   Add -O to CFLAGS.  "Don't use -O here" was a historical relic
#       2025/11/27 - cleanup
#	2026/6/28 - Don't use -O and then run /lib/c2 on the assembly file.
#		    That runs the optimizer twice.  Borrow from pascal/pi and
#		    use tmp/ as the prefix for mkstr.
#	2026/11/26 - the 80286: what is the PDP-11's in pdp11.c and pdp11x.c
#		    is the 286's in i286.c and i286x.c, and the code goes
#		    to the 286's second pass, as does the data.  f77pass1
#		    is of the medium model, its code being more than a
#		    segment of 64K, where the PDP-11 had it in overlays;
#		    and so is f77, which has i286x.o too.

STRINGS=	f77_strings
MODEL=		-mm
CFLAGS=		-O -I. -w ${MODEL}
LDFLAGS= -i ${MODEL}

SRCS= 	data.c driver.c equiv.c error.c exec.c expr.c gram.c init.c \
	intr.c io.c lex.c main.c misc.c i286.c i286x.c \
	proc.c put.c putdmr.c

PASS1OBJECTS=	data.o equiv.o error.o exec.o expr.o gram.o init.o \
		intr.o io.o lex.o main.o misc.o i286.o i286x.o proc.o \
		put.o putdmr.o

all:	tmp f77 f77pass1

f77:	driver.o i286x.o
	${CC} ${LDFLAGS} driver.o i286x.o -o f77
	@size f77

f77pass1:	${PASS1OBJECTS}
	${CC} ${LDFLAGS} -o f77pass1 \
		main.o misc.o gram.o lex.o \
		proc.o init.o \
		equiv.o data.o error.o \
		io.o expr.o intr.o exec.o \
		put.o putdmr.o i286.o i286x.o
	@size f77pass1

gram.o:	gram.c
	rm -f tmp/gram.c
	mkstr - f77_strings tmp/ gram.c
	${CC} ${CFLAGS} -c tmp/gram.c 
	rm -f tmp/gram.c

gram.c:	gram.head gram.dcl gram.expr gram.exec gram.io tokdefs
	( sed <tokdefs "s/#define/%token/" ;\
	cat gram.head gram.dcl gram.expr gram.exec gram.io ) >gram.in
	${YACC} ${YFLAGS} gram.in
	mv y.tab.c gram.c
	rm gram.in

tmp:
	-mkdir tmp

.c.o:
	rm -f tmp/$*.c
	mkstr - f77_strings tmp/ $*.c
	${CC} ${CFLAGS} -c tmp/$*.c
	rm -f tmp/$*.c

driver.o:
	${CC} -c ${CFLAGS} $*.c

i286x.o:
	${CC} -c ${CFLAGS} $*.c

defs:	ftypes defines i286defs
	touch defs

tokdefs:	tokens
	grep -n . <tokens | sed "s/\([^:]*\):\(.*\)/#define \2 \1/" >tokdefs

install: all
	install -c -s -m 755 -o root f77 /usr/bin
	install -c -s -m 755 -o root f77pass1 /usr/libexec
	install -c -m 644 ${STRINGS} /usr/share/misc

clean:
	rm -f f77 f77pass1 *.o gram.c f77_strings
	rm -fr tmp

tags:
	ctags ${SRCS}

data.o:		defs string_defs data.c
driver.o:	defs drivedefs dmrdefs driver.c
equiv.o:	defs string_defs equiv.c
error.o:	defs string_defs error.c
exec.o:		defs string_defs exec.c
expr.o:		defs string_defs expr.c
gram.o:		defs string_defs gram.c
init.o:		defs string_defs init.c
intr.o:		defs string_defs intr.c
io.o:		defs string_defs io.c
lex.o:		defs string_defs tokdefs lex.c
main.o:		defs string_defs main.c
misc.o:		defs string_defs misc.c
i286.o:		defs string_defs dmrdefs i286.c
i286x.o:	defines i286defs dmrdefs i286x.c
proc.o:		defs string_defs proc.c
put.o:		defs string_defs put.c
putdmr.o:	defs string_defs dmrdefs putdmr.c
