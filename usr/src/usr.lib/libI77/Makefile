#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6 (2.11BSD) 2026/11/26
#

# Makefile for f77 I/O lib, libI77.a
#
# The 80286: Fortran programs are of the small model alone (f77(1)),
# and so is this library.  The PDP-11's profiled one, of cc -p, is not
# made: the 286's compiler does not profile.  Nor is its ld -x -r done,
# which took out the local symbols: the 286's ld makes nothing but a
# program.

CFLAGS	      = -O
DEST	      = /usr/lib

HDRS	      = f_errno.h \
		fio.h \
		fiodefs.h \
		format.h \
		lio.h \
		nmlio.h

LIBRARY	      = libI77.a

OBJS	      = backspace.o \
		c_dfe.o \
		c_iio.o \
		c_sfe.o \
		close.o \
		dofio.o \
		dolio.o \
		douio.o \
		due.o \
		endfile.o \
		err.o \
		f77_abort.o \
		f_errlist.o \
		fmt.o \
		fmtlib.o \
		inquire.o \
		lread.o \
		lwrite.o \
		open.o \
		rdfe.o \
		rdfmt.o \
		rewind.o \
		rfi.o \
		rsfe.o \
		rsli.o \
		rsnmle.o \
		sue.o \
		util.o \
		wdfe.o \
		wfi.o \
		wrtfmt.o \
		wsfe.o \
		wsli.o \
		wsnmle.o

SRCS	      = backspace.c \
		c_dfe.c \
		c_iio.c \
		c_sfe.c \
		close.c \
		dofio.c \
		dolio.c \
		douio.c \
		due.c \
		endfile.c \
		err.c \
		f77_abort.c \
		f_errlist.c \
		fmt.c \
		fmtlib.c \
		inquire.c \
		lread.c \
		lwrite.c \
		open.c \
		rdfe.c \
		rdfmt.c \
		rewind.c \
		rfi.c \
		rsfe.c \
		rsli.c \
		rsnmle.c \
		sue.c \
		util.c \
		wdfe.c \
		wfi.c \
		wrtfmt.c \
		wsfe.c \
		wsli.c \
		wsnmle.c

all:		$(LIBRARY) libI66.o

$(LIBRARY):	$(OBJS) Version
		@echo -n "Loading $(LIBRARY) ... "
		@ar cru $(LIBRARY) \
			`lorder $(OBJS) | tsort` Version
		@echo "done"

Version:	$(SRCS) mkvers
		@rm -f Version.c
		./mkvers $(LIBRARY) $(SRCS) > Version.c
		$(CC) -c Version.c
		mv Version.o Version

mkvers:		mkvers.c
		@$(CC) mkvers.c -o mkvers

clean:;		@rm -f $(OBJS) mkvers Version $(LIBRARY) libI66.o

depend:;	@echo Construct dependencies manually

index:;		@ctags -wx $(HDRS) $(SRCS)

install:	$(LIBRARY) libI66.o
		install -c -m 644 $(LIBRARY) $(DEST)/$(LIBRARY)
		ranlib $(DEST)/$(LIBRARY)
		install -c -m 644 -c libI66.o $(DEST)/libI66.a

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

f_errlist.o:	f_errlist.c
		$(CC) $(CFLAGS) -c f_errlist.c
.c.o:
		$(CC) $(CFLAGS) -c $*.c

backspace.o:	fio.h f_errno.h fiodefs.h backspace.c
c_dfe.o:	fio.h f_errno.h fiodefs.h c_dfe.c
rdfe.o:		fio.h f_errno.h fiodefs.h rdfe.c
wdfe.o:		fio.h f_errno.h fiodefs.h wdfe.c
due.o:		fio.h f_errno.h fiodefs.h due.c
rfi.o:		fio.h f_errno.h fiodefs.h rfi.c
wfi.o:		fio.h f_errno.h fiodefs.h wfi.c
rsli.o:		fio.h f_errno.h fiodefs.h lio.h rsli.c
wsli.o:		fio.h f_errno.h fiodefs.h lio.h wsli.c
c_iio.o:	fio.h f_errno.h fiodefs.h lio.h c_iio.c
inquire.o:	fio.h f_errno.h fiodefs.h inquire.c
rewind.o:	fio.h f_errno.h fiodefs.h rewind.c
rdfmt.o:	fio.h f_errno.h fiodefs.h format.h rdfmt.c
sue.o:		fio.h f_errno.h fiodefs.h sue.c
douio.o:	fio.h f_errno.h fiodefs.h douio.c
rsfe.o:		fio.h f_errno.h fiodefs.h rsfe.c
wsfe.o:		fio.h f_errno.h fiodefs.h wsfe.c
c_sfe.o:	fio.h f_errno.h fiodefs.h c_sfe.c
fmt.o:		fio.h f_errno.h fiodefs.h format.h fmt.c
dofio.o:	fio.h f_errno.h fiodefs.h format.h dofio.c
lwrite.o:	fio.h f_errno.h fiodefs.h lio.h lwrite.c
lread.o:	fio.h f_errno.h fiodefs.h lio.h lread.c
dolio.o:	fio.h f_errno.h fiodefs.h lio.h dolio.c
open.o:		fio.h f_errno.h fiodefs.h open.c
close.o:	fio.h f_errno.h fiodefs.h close.c
util.o:		fio.h f_errno.h fiodefs.h util.c
endfile.o:	fio.h f_errno.h fiodefs.h endfile.c
wrtfmt.o:	fio.h f_errno.h fiodefs.h format.h wrtfmt.c
err.o:		fio.h f_errno.h fiodefs.h err.c
fmtlib.o:	fio.h f_errno.h fiodefs.h fmtlib.c
f77_abort.o:	fio.h f_errno.h fiodefs.h f77_abort.c
rsnmle.o:	fio.h f_errno.h fiodefs.h lio.h nmlio.h rsnmle.c
wsnmle.o:	fio.h f_errno.h fiodefs.h lio.h nmlio.h wsnmle.c
