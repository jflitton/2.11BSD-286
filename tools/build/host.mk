# What the system is built with where it is built, besides its compiler,
# assembler and linker (usr/src/lib/ccom/i286): made in obj/host by the
# C compiler of the machine it is built on.  tools/build/build.sh makes
# it first.  GNU make.
#
# The tree's yacc and lex, so that the parsers and scanners in the
# system's programs are what upstream's yacc and lex make of the
# grammars, whatever the machine has by those names.  Both are C of
# before the standard, which the compiler is asked to take as it finds
# it; it is left its own knowledge of the library's functions, which is
# all that a program that calls printf undeclared has to go by where
# such a function is not called as others are, but for index, since lex
# has one of its own.  This yacc has the larger of its two sets of
# tables, its items being twice the size where a pointer is twice an
# int, and reads the tree's yaccpar; this lex keeps a pointer or a
# number in a long, and reads the tree's ncform.  The tables they write
# are the same as those of the system's own (tests/system).

TOP:=	$(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
SRC=	$(TOP)/usr/src
H=	$(TOP)/obj/host
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing \
	-Wno-error=return-mismatch -Wno-error=int-conversion
YSRC=	$(SRC)/usr.bin/yacc
LSRC=	$(SRC)/usr.bin/lex
TM=	$(SRC)/share/tmac
MDOC=	doc doc-ditroff doc-common doc-nroff doc-syms
HTMAC=	$(addprefix $(H)/tmac/tmac.,an an.new an6n an6t $(MDOC))

all:	$(H)/yacc $(H)/lex $(H)/nroff $(H)/eqn $(H)/zic $(HTMAC) \
	$(H)/term/tabtn300.tab $(H)/term/tabcrt.tab

$(H) $(H)/tmac $(H)/term:
	mkdir -p $@

$(H)/yacc: $(addprefix $(YSRC)/,y1.c y2.c y3.c y4.c dextern files yaccpar) \
		| $(H)
	@echo hostcc yacc
	@$(HOSTCC) $(HOSTCFLAGS) -Wno-error=implicit-function-declaration \
	    -Wno-error=implicit-int -DHUGE '-DPARSER="$(YSRC)/yaccpar"' \
	    -o $@ $(filter %.c,$^)

# yacc leaves its files, under names of its own, in the directory it
# runs in: it runs in one made for it.
$(H)/lex: $(addprefix $(LSRC)/,lmain.c sub1.c sub2.c header.c ldefs.c once.c \
		parser.y ncform nrform) $(H)/yacc
	@echo hostcc lex
	@rm -rf $(H)/lex.d && mkdir $(H)/lex.d && cd $(H)/lex.d && \
	    $(H)/yacc $(LSRC)/parser.y && mv y.tab.c ../lex.y.c && \
	    cd .. && rm -rf lex.d
	@$(HOSTCC) $(HOSTCFLAGS) -Wno-error=implicit-function-declaration \
	    -Wno-error=implicit-int -fno-builtin-index -Dunix -DINTPTR=long \
	    '-DNCFORM="$(LSRC)/ncform"' '-DNRFORM="$(LSRC)/nrform"' \
	    -I$(LSRC) -o $@ $(addprefix $(LSRC)/,lmain.c sub1.c sub2.c \
	    header.c) $(H)/lex.y.c

# nroff, which formats the manual as the system's would, which
# tests/programs checks.  It is the system's but that it looks for its
# macros in tmac/, and its printers' tables in term/, of the directory
# it is run in, where it is (tools/build/nroff.py).  Its macros are the
# system's, but that they look for each other in tmac/, where the
# system's say /usr/share/tmac/; mdoc's, which are the doc macros and
# the files they read, without their comments, as share/tmac's Makefile
# installs them (strip.sed).  Of the printers', the line printer's,
# which the system has as tabtn300 and tabcrt too: phantasia's help is
# formatted with -Ttn300, and backgammon's rules with -Tcrt.
NR=	$(SRC)/usr.bin/roff
NROFF=	$(addprefix common_source/,ni.c nii.c n1.c n2.c n3.c n4.c n5.c) \
	nroff/n6.c $(addprefix common_source/,n7.c n8.c n9.c) nroff/n10.c \
	common_source/hytab.c nroff/ntab.c common_source/suftab.c
$(H)/nroff: $(addprefix $(NR)/,$(NROFF)) $(wildcard $(NR)/common_source/*.h) \
		| $(H)
	@echo hostcc nroff
	@$(HOSTCC) $(HOSTCFLAGS) -Wno-error=implicit-function-declaration \
	    -Wno-error=implicit-int -DNROFF '-D_PATH_TMAC="tmac/"' \
	    '-D_PATH_TERMTAB="term/"' -I$(NR)/common_source -o $@ \
	    $(addprefix $(NR)/,$(NROFF))
$(filter-out $(addprefix %.,$(MDOC)),$(HTMAC)): $(H)/tmac/%: $(TM)/% | $(H)/tmac
	LC_ALL=C sed 's,/usr/share/tmac/,tmac/,' $< > $@
$(filter $(addprefix %.,$(MDOC)),$(HTMAC)): $(H)/tmac/tmac.%: $(TM)/% \
		$(TM)/strip.sed | $(H)/tmac
	LC_ALL=C sed -f $(TM)/strip.sed $< | \
	    LC_ALL=C sed 's,/usr/share/tmac/,tmac/,' > $@
NT=	$(NR)/nroff_term
$(H)/term/tabtn300.tab: $(NT)/tablpr.c $(NT)/mktab.c $(NR)/common_source/tw.h \
		| $(H)/term
	@$(HOSTCC) $(HOSTCFLAGS) -Wno-error=implicit-function-declaration \
	    -Wno-error=implicit-int -I$(NR)/common_source -o $(H)/mktablpr \
	    $(NT)/mktab.c $(NT)/tablpr.c
	$(H)/mktablpr > $@
$(H)/term/tabcrt.tab: $(H)/term/tabtn300.tab
	cp $< $@

# eqn, which eqn(1)'s page goes through before nroff, as man1's Makefile
# has it: the system's, with yacc's values a long, as a pointer is here
# (usr/src/usr.bin/eqn/e.h), its io.c's getline and inline not this
# machine's, and what it declares of the C library its own.
ESRC=	$(SRC)/usr.bin/eqn
EQN=	diacrit.c eqnbox.c font.c fromto.c funny.c glob.c integral.c io.c \
	lex.c lookup.c mark.c matrix.c move.c over.c paren.c pile.c shift.c \
	size.c sqrt.c text.c
$(H)/eqn.d/e.c: $(ESRC)/e.y $(H)/yacc
	@rm -rf $(H)/eqn.d && mkdir $(H)/eqn.d && cd $(H)/eqn.d && \
	    $(H)/yacc -d $< && mv y.tab.c e.c && mv y.tab.h e.def
$(H)/eqn: $(H)/eqn.d/e.c $(addprefix $(ESRC)/,$(EQN) e.h)
	@echo hostcc eqn
	@$(HOSTCC) $(HOSTCFLAGS) -Wno-error=implicit-function-declaration \
	    -Wno-error=implicit-int -fno-builtin -DYYSTYPE=long \
	    -D_POSIX_C_SOURCE=200112L -Dinline=e_inline -I$(H)/eqn.d \
	    -I$(ESRC) -o $@ $(H)/eqn.d/e.c $(addprefix $(ESRC)/,$(EQN))

# zic, which makes the zone files, as the system's own zic makes them
# (tests/programs): their numbers are four bytes, high byte first, on
# any machine.  This one is built for 32-bit time_t
# (tools/hostzic.h), whose sums it expects to wrap.
ZI=	$(SRC)/share/zoneinfo
$(H)/zic: $(addprefix $(ZI)/,zic.c scheck.c ialloc.c) \
		$(TOP)/usr/include/tzfile.h $(TOP)/tools/hostzic.h | $(H)
	@echo hostcc zic
	@mkdir -p $(H)/include
	@cp $(TOP)/usr/include/tzfile.h $(H)/include
	@$(HOSTCC) $(HOSTCFLAGS) -Wno-error=implicit-function-declaration \
	    -Wno-error=implicit-int -fwrapv -include $(TOP)/tools/hostzic.h \
	    -I$(H)/include -o $@ $(filter %.c,$^)

.PHONY:	all
