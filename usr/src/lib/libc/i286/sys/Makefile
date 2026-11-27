#
# The 286's system calls, after the PDP-11's ../../pdp/sys.  A stub for
# each call, which mkstubs (mkstubs.c) writes from syscalls.tab, a file
# each, so that a program gets only the calls it makes, and checks
# against the kernel's table of calls; and the calls that need more
# than a stub, in C and sigtramp.s.  Each is made for the three models:
# the small's here, the medium's in m and the large's in l; and for
# profiling, the small model's alone, in profiled, where the stubs and
# the C count their calls (mkstubs -p, cc -p) and sigtramp.s is the
# small model's.  stubs is the names of the stubs' objects.
#
SRCS=	brk.c execl.c execle.c execv.c iovec.c setlogin.c sigaction.c \
	sigaltstack.c sigtramp.s
OBJS=	brk.o execl.o execle.o execv.o iovec.o setlogin.o sigaction.o \
	sigaltstack.o sigtramp.o
CFLAGS=	-O ${DEFS}
AS=	as
SYSENT=	../../../../sys/sys/init_sysent.c

.s.o:
	${AS} -mm -DFAR -I../crt -o m/$*.o $*.s
	${AS} -ml -DFAR -DFARDATA -I../crt -o l/$*.o $*.s
	${AS} -ms -I../crt -o $*.o $*.s
	cp $*.o profiled/$*.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	mv $*.o profiled/$*.o
	${CC} -mm ${CFLAGS} -c $*.c
	mv $*.o m/$*.o
	${CC} -ml ${CFLAGS} -c $*.c
	mv $*.o l/$*.o
	${CC} ${CFLAGS} -c $*.c

syslib syslib_p syslib_m syslib_l: ${OBJS} stubs
	@echo "building profiled syslib"
	@cd profiled; ar cru ../syslib_p ${OBJS} `cat ../stubs`
	@echo "building medium model syslib"
	@cd m; ar cru ../syslib_m ${OBJS} `cat ../stubs`
	@echo "building large model syslib"
	@cd l; ar cru ../syslib_l ${OBJS} `cat ../stubs`
	@echo "building small model syslib"
	@ar cru syslib ${OBJS} `cat stubs`

mkstubs: mkstubs.c
	${CC} -O -o mkstubs mkstubs.c
	rm -f mkstubs.o

# Each model's stubs, and those for profiling, written in tmp and
# assembled where their objects are.
stubs: mkstubs syscalls.tab /usr/include/syscall.h ${SYSENT}
	rm -rf tmp
	mkdir tmp
	./mkstubs -m s -k ${SYSENT} -d tmp syscalls.tab /usr/include/syscall.h
	cd tmp; for f in *.s; do \
		${AS} -ms -o ../`basename $$f .s`.o $$f || exit 1; done
	rm -f tmp/*
	./mkstubs -m s -p -k ${SYSENT} -d tmp syscalls.tab /usr/include/syscall.h
	cd tmp; for f in *.s; do \
		${AS} -ms -o ../profiled/`basename $$f .s`.o $$f || exit 1; done
	rm -f tmp/*
	./mkstubs -m m -k ${SYSENT} -d tmp syscalls.tab /usr/include/syscall.h
	cd tmp; for f in *.s; do \
		${AS} -mm -o ../m/`basename $$f .s`.o $$f || exit 1; done
	rm -f tmp/*
	./mkstubs -m l -k ${SYSENT} -d tmp syscalls.tab /usr/include/syscall.h
	cd tmp; for f in *.s; do \
		${AS} -ml -o ../l/`basename $$f .s`.o $$f || exit 1; done
	cd tmp; ls *.s | sed 's/s$$/o/' > ../stubs
	rm -rf tmp

clean:
	rm -f *.o profiled/*.o m/*.o l/*.o syslib syslib_p syslib_m syslib_l mkstubs stubs
	rm -rf tmp

sigtramp.o: sigtramp.s ../crt/DEFS.inc
