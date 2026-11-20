#
# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	7.0 (2.11BSD) 2026/11/10
#
# NOTE:  You need to generate a new disktab.man on a system with groff
#        and put that file here, since 2BSD's sed can't handle the ex-
#        pressions. This only needs to be done if the actual disktab
#        changes

# NOTE: There is no pdf target.  Terri Kennedy (terri-decus@glaver.org), who 
#	did the documentation said: That part (pdf target) was done on a 
#	Windows PC. It might be possible to do in Ghostscript (but not 
#	by me, and no guarantees). I do note that the circa-2000 PDF was 
#	created by "Acrobat Distiller 3.0 for Power Macintosh". That's two 
#	whole Apple architectures ago 8-}
#
#	That said "eqn ${SRC} | tbl | groff -T ps ${PAGES} -ms | pstopdf"
#	should produce something readable.

SRC=	0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t c.t #d.t

all:
	@echo pick one of troff, nroff, or ps

troff: ${SRC}
	expand GENERIC > GENERIC.out
	expand EXAMPLE > EXAMPLE.out
	eqn ${SRC} | tbl | troff -t ${PAGES} -ms > setup.out

nroff: ${SRC}
	expand GENERIC > GENERIC.out
	expand EXAMPLE > EXAMPLE.out
	eqn ${SRC} | tbl | nroff -Tlpr ${PAGES} -ms > setup.out

ps:	${SRC}
	@if [ "`uname -m`" = "pdp11" ]; then \
	   echo "This only works on systems with groff and modern sed"; \
	   exit 1; \
	fi
	./fixdisktab.sh
	expand GENERIC > GENERIC.out
	expand EXAMPLE > EXAMPLE.out
	eqn ${SRC} | tbl | groff -T ps ${PAGES} -ms > setup.ps

clean:
	rm -f setup.out setup.ps ?.t.spell GENERIC.out EXAMPLE.out

spell:	${SRC}
	@for i in ${SRC}; do \
		echo $$i; spell $$i | sort | comm -23 - spell.ok > $$i.spell; \
	done
