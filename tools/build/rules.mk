# The rules the system's make has of its own (usr/src/bin/make/files.c),
# for the make of the machine the system is built on, which
# tools/build/build.sh runs without its own (make -r, and this in
# MAKEFILES): a Makefile that counts on a rule gets the one it gets on
# the system, and none that the system's make does not have.
.SUFFIXES:
.SUFFIXES: .out .a .o .c .y .l .s
YACC=yacc
YFLAGS=
LEX=lex
LFLAGS=
CC=cc
AS=as
CFLAGS=
LOADLIBES=

.c.a:
	$(CC) $(CFLAGS) -c $<
	ar r $@ $*.o
	rm -f $*.o

.c.o:
	$(CC) $(CFLAGS) -c $<

.s.o:
	$(AS) -o $@ $<

.y.o:
	$(YACC) $(YFLAGS) $<
	$(CC) $(CFLAGS) -c y.tab.c
	rm y.tab.c
	mv y.tab.o $@

.l.o:
	$(LEX) $(LFLAGS) $<
	$(CC) $(CFLAGS) -c lex.yy.c
	rm lex.yy.c
	mv lex.yy.o $@

.y.c:
	$(YACC) $(YFLAGS) $<
	mv y.tab.c $@

.l.c:
	$(LEX) $(LFLAGS) $<
	mv lex.yy.c $@

.s.out .c.out .o.out:
	$(CC) $(CFLAGS) $< $(LOADLIBES) -o $@

.y.out:
	$(YACC) $(YFLAGS) $<
	$(CC) $(CFLAGS) y.tab.c $(LOADLIBES) -ly -o $@
	rm y.tab.c

.l.out:
	$(LEX) $(LFLAGS) $<
	$(CC) $(CFLAGS) lex.yy.c $(LOADLIBES) -ll -o $@
	rm lex.yy.c
