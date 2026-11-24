#       Makefile        2.0 (2.11BSD) 2025/11/25

LDFLAGS=-i
CFLAGS=	-O

LIB=	/lib

# The 80286's, in i286: c20.c and c21.c here are the PDP-11's.  It is
# made of each of two models: c2, the small, and c2l, the large, whose
# memory is its own, for what c2 has no room for, a function of more
# than its 64K of data, which cc then gives it (bin/cc.c).  The large
# model's objects are made in l, as a library's are.
OBJS=	c20.o c21.o

all: c2 c2l

c2 c2l: ${OBJS}
	cc ${CFLAGS} ${LDFLAGS} -o c2 ${OBJS}
	cd l; cc -ml ${LDFLAGS} -o ../c2l ${OBJS}

c20.o: i286/c20.c i286/c2.h
	cc ${CFLAGS} -ml -c i286/c20.c
	mv c20.o l/c20.o
	cc ${CFLAGS} -c i286/c20.c

c21.o: i286/c21.c i286/c2.h
	cc ${CFLAGS} -ml -c i286/c21.c
	mv c21.o l/c21.o
	cc ${CFLAGS} -c i286/c21.c

install: c2 c2l
	-mv ${LIB}/c2 ${LIB}/oc2
	install -s c2 ${LIB}
	-mv ${LIB}/c2l ${LIB}/oc2l
	install -s c2l ${LIB}

restore:
	mv ${LIB}/oc2 ${LIB}/c2
	mv ${LIB}/oc2l ${LIB}/c2l

clean:
	rm -f *.o l/*.o c2 c2l
