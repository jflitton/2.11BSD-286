#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.4.5 (2.11BSD) 2026/5/14
#
# The 286's: not the pages of the PDP-11's devices.
MDIR=	/usr/man/cat4
SRCS=	arp.4 fl.4 icmp.4 inet.4 ip.4 lo.4 lp.4 networking.4 null.4 \
	pty.4 tcp.4 tty.4 udp.4
OBJS=	arp.0 fl.0 icmp.0 inet.0 ip.0 lo.0 lp.0 networking.0 null.0 \
	pty.0 tcp.0 tty.0 udp.0

.SUFFIXES: .4 .0

.4.0:
	/usr/man/manroff $*.4 > $*.0

all: ${OBJS}

clean: FRC
	rm -f ${OBJS}

install: all
	install -c -o root -g staff -m 444 ${OBJS} ${MDIR}

FRC:
