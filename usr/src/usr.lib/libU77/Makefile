#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3 (2.11BSD) 2026/11/26
#
# Makefile for f77 system lib, libU77.a
#
# The 80286: Fortran programs are of the small model alone (f77(1)),
# and so is this library.  The PDP-11's profiled one, of cc -p, is not
# made: the 286's compiler does not profile.  Nor is its ld -x -r done,
# which took out the local symbols: the 286's ld makes nothing but a
# program.

CFLAGS	      = -O
DEST	      = /usr/lib

LIBRARY	      = libU77.a

OBJS	      = access_.o \
		alarm_.o \
		chdir_.o \
		chmod_.o \
		ctime_.o \
		dtime_.o \
		etime_.o \
		f77lid_.o \
		fdate_.o \
		fgetc_.o \
		fork_.o \
		fputc_.o \
		fseek_.o \
		fstat_.o \
		ftell_.o \
		gerror_.o \
		getarg_.o \
		getc_.o \
		getcwd_.o \
		getenv_.o \
		getgid_.o \
		getlog_.o \
		getpid_.o \
		getuid_.o \
		gmtime_.o \
		hostnm_.o \
		iargc_.o \
		idate_.o \
		ierrno_.o \
		ioinit.o \
		isatty_.o \
		itime_.o \
		kill_.o \
		link_.o \
		loc_.o \
		lstat_.o \
		ltime_.o \
		malloc_.o \
		perror_.o \
		putc_.o \
		qsort_.o \
		rename_.o \
		sleep_.o \
		stat_.o \
		symlnk_.o \
		system_.o \
		tapeio.o \
		time_.o \
		ttynam_.o \
		unlink_.o \
		wait_.o

SRCS	      = access_.c \
		alarm_.c \
		chdir_.c \
		chmod_.c \
		ctime_.c \
		dtime_.c \
		etime_.c \
		f77lid_.c \
		fdate_.c \
		fgetc_.c \
		fork_.c \
		fputc_.c \
		fseek_.c \
		fstat_.c \
		ftell_.c \
		gerror_.c \
		getarg_.c \
		getc_.c \
		getcwd_.c \
		getenv_.c \
		getgid_.c \
		getlog_.c \
		getpid_.c \
		getuid_.c \
		gmtime_.c \
		hostnm_.c \
		iargc_.c \
		idate_.c \
		ierrno_.c \
		ioinit.f \
		isatty_.c \
		itime_.c \
		kill_.c \
		link_.c \
		loc_.c \
		lstat_.c \
		ltime_.c \
		malloc_.c \
		perror_.c \
		putc_.c \
		qsort_.c \
		rename_.c \
		sleep_.c \
		stat_.c \
		symlnk_.c \
		system_.c \
		tapeio.c \
		time_.c \
		ttynam_.c \
		unlink_.c \
		wait_.c

all:		$(LIBRARY)

$(LIBRARY):	$(OBJS) Version
		@echo -n "Loading $(LIBRARY) ... "
		@ar cru $(LIBRARY) \
			`lorder $(OBJS) | tsort` ioinit.f Version
		@echo "done"

Version:	$(SRCS) mkvers
		@rm -f Version.c
		./mkvers $(LIBRARY) $(SRCS) > Version.c
		$(CC) -c Version.c
		mv Version.o Version

mkvers:		mkvers.c
		@$(CC) mkvers.c -o mkvers

clean:;		@rm -f $(OBJS) Version mkvers $(LIBRARY)

depend:;	@echo Construct dependencies manually

index:;		@ctags -wx $(SRCS)

install:	$(LIBRARY)
		install -c -m 644 $(LIBRARY) $(DEST)/$(LIBRARY)
		ranlib $(DEST)/$(LIBRARY)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

.c.o:
		$(CC) $(CFLAGS) -c $*.c

.f.o:
		$(FC) -O -c $*.f
