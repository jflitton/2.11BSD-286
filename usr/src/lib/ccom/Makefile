CFLAGS = -O
LDC0FLAGS= -i
LDC1FLAGS= -n

# The 80286's: c0, the first pass, is the PDP-11's, which has what
# differs under #ifdef i286; c1, the second, is i286's, with a code
# generator of its own in place of the PDP-11's and its tables (optable,
# which cvopt made into table.o).
C1OBJS=	c10.o c11.o c12.o c13.o c14.o c15.o

all: c0 c1
c0: c00.o c01.o c02.o c03.o c04.o c05.o ansi.o
	${CC} ${LDC0FLAGS} ${CFLAGS} -o c0 c00.o c01.o c02.o c03.o c04.o \
	c05.o ansi.o

c00.o c01.o c02.o c03.o c04.o c05.o ansi.o: c0.h

c1: ${C1OBJS}
	${CC} ${LDC1FLAGS} ${CFLAGS} -o c1 ${C1OBJS}

c10.o: i286/c10.c i286/c1.h
	${CC} ${CFLAGS} -c i286/c10.c
c11.o: i286/c11.c i286/c1.h
	${CC} ${CFLAGS} -c i286/c11.c
c12.o: i286/c12.c i286/c1.h
	${CC} ${CFLAGS} -c i286/c12.c
c13.o: i286/c13.c i286/c1.h
	${CC} ${CFLAGS} -c i286/c13.c
c14.o: i286/c14.c i286/c1.h
	${CC} ${CFLAGS} -c i286/c14.c
c15.o: i286/c15.c i286/c1.h
	${CC} ${CFLAGS} -c i286/c15.c

install: c0 c1
	-mv /lib/c0 /lib/oc0
	-mv /lib/c1 /lib/oc1
	install -s -m 755 -o root -g wheel c0 /lib/c0
	install -s -m 755 -o root -g wheel c1 /lib/c1

restore:
	mv /lib/oc0 /lib/c0
	mv /lib/oc1 /lib/c1

lint:
	lint -haxc -I. c0?.c > lint.c0
	lint -haxc -Ii286 i286/c1?.c > lint.c1

clean:
	rm -f *.o c0 c1 lint.c?
