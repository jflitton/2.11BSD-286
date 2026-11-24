#
# adduser, this port's, which does what upstream's adduser(8) says to
# do by hand (adduser.c), and what it gives a new user, the files of
# /usr/skel, which it copies: .cshrc, .login and .profile (dot.*).
#
BINDIR=	/usr/local
CFLAGS=	-O
LDFLAGS= -i
PROGRAM= adduser
SRCS=	adduser.c
OBJS=	adduser.o

all:	${PROGRAM}

${PROGRAM}: ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJS}

install: ${PROGRAM}
	install -c -s -m 755 ${PROGRAM} ${BINDIR}
	-mkdir -p /usr/skel
	install -c -m 644 dot.cshrc /usr/skel/.cshrc
	install -c -m 644 dot.login /usr/skel/.login
	install -c -m 644 dot.profile /usr/skel/.profile

clean:
	rm -f a.out core *.o ${PROGRAM}

depend:
	mkdep ${CFLAGS} ${SRCS}
