#	@(#)Makefile	4.3	(2.11BSD) 2025/11/25
#
# The 80286: each file is compiled for the three models, the medium's
# object put in m and the large's in l, and the small's left here; each
# model has a library of its own, which is installed where cc links a
# program of that model with it, /usr/lib, /usr/lib/m and /usr/lib/l.
#
CFLAGS=-O
SRCS= pow.c gcd.c msqrt.c mdiv.c mout.c mult.c madd.c util.c
OBJS= pow.o gcd.o msqrt.o mdiv.o mout.o mult.o madd.o util.o
TAGSFILE=tags

.c.o:
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

libmp.a: $(OBJS)
	ar cr libmp.a $(OBJS)
	cd m; ar cr ../libmp_m.a $(OBJS)
	cd l; ar cr ../libmp_l.a $(OBJS)

install: libmp.a
	install -c -m 644 -o root libmp.a /usr/lib/libmp.a
	ranlib /usr/lib/libmp.a
	install -c -m 644 -o root libmp_m.a /usr/lib/m/libmp.a
	ranlib /usr/lib/m/libmp.a
	install -c -m 644 -o root libmp_l.a /usr/lib/l/libmp.a
	ranlib /usr/lib/l/libmp.a

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} m/*.o l/*.o libmp.a libmp_m.a libmp_l.a tags
