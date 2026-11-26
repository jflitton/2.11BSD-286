#	Makefile	4.5	(2.11BSD) 2026/1/30

DEBUG=
# The 80286: of the medium model, as uucico is, which it is linked with
# (../Makefile).
MODEL=	-mm
CFLAGS= -O  -I. ${DEBUG} ${MODEL}

OBJS= bsdtcp.o df2.o df12.o dk.o dn.o hys.o hysq.o mic.o nov.o pen.o pnet.o \
	rvmacs.o sy.o unet.o va212.o va811.o va820.o vad.o vent.o vmacs.o \
	hys24.o cds224.o att2224.o

aculib.a: ${OBJS}
	ar cr aculib.a ${OBJS}
	-ranlib aculib.a 2>&1 | sed '/no symbol table/d'

${OBJS}: ../uucp.h ../condevs.h

install: aculib.a

clean:
	rm -f *.o core errs  aculib.a
