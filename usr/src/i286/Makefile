# The system's programs, built for the 286 with the C library
# (../lib/libc/i286).
#
# From the top of the repository, after `. tools/env.sh` and making the
# library:
#
#	make -C usr/src/i286
#
# makes each program in obj/bin, under its own name, and the libraries
# they use in obj/lib/M, M being the model's letter.  GNU make.
#
# A program is listed with its sources, relative to usr/src unless they
# are absolute, in NAME_SRCS, and may have NAME_FLAGS for the compiler
# and linker (tools/cc), NAME_LIBS, and NAME_MODEL: s, the small model,
# unless it is m, the medium, or l, the large (docs/memory-model.md).
# Each is compiled
# and linked in one run of tools/cc; what the compiler says goes to
# obj/bin/log/NAME, and is shown only if it fails.
#
# With CCOM=1 they are made by the system's own compiler, assembler and
# linker, as built for this machine (tools/cc -ccom): the programs in
# obj/nbin and the libraries in obj/nlib/M, libNAME.a, where the C
# library as those make it is to be (make -C ../lib/libc/i286 CCOM=1).
# The compiler's code is nearly a tenth larger than the cross
# compiler's, and each program is of the model it has here.  That
# linker reads a library once, so a program's libraries are named in
# the order of what needs what, as they are for ld anywhere.

TOP:=	$(abspath ../../..)
SRC:=	$(TOP)/usr/src
O=	$(TOP)/obj/$(if $(CCOM),nbin,bin)
NLIB=	$(TOP)/obj/nlib
LIB=	$(if $(CCOM),$(NLIB),$(TOP)/obj/lib)
CC=	$(TOP)/tools/cc $(if $(CCOM),-ccom)
# what a library's name ends with
LX=	$(if $(CCOM),a,lib)

# Programs of one file, bin/NAME.c.
BINSTD=	cat chgrp chmod cmp cp date dd df du echo ed grep hostid kill ln \
	mv nice od pagesize pr pwd rm rmdir sync tee time who wall write
$(foreach p,$(BINSTD),$(eval $(p)_SRCS = bin/$(p).c))

init_SRCS=	sbin/init/init.c
getty_SRCS=	$(addprefix libexec/getty/,main.c init.c subr.c gettytab.c \
		get_date.c)
login_SRCS=	bin/login/login.c $(addprefix libexec/getty/,gettytab.c \
		subr.c init.c)
login_FLAGS=	-I$(SRC)/libexec/getty
login_LIBS=	-lutil
sh_SRCS=	$(addprefix bin/sh/,setbrk.c builtin.c blok.c stak.c cmd.c \
		fault.c main.c word.c string.c name.c args.c xec.c service.c \
		error.c io.c print.c macro.c expand.c ctype.c msg.c)
ls_SRCS=	bin/ls/ls.c bin/ls/stat_flags.c
mkdir_SRCS=	bin/mkdir/mkdir.c
stty_SRCS=	bin/stty/stty.c
test_SRCS=	bin/test/test.c bin/test/operators.c
hostname_SRCS=	bin/hostname/hostname.c
mknod_SRCS=	sbin/mknod/mknod.c
mount_SRCS=	sbin/mount/mount.c sbin/mount/mount_ufs.c \
		sbin/mount/getmntopts.c
umount_SRCS=	sbin/umount/umount.c
reboot_SRCS=	sbin/reboot/reboot.c
sleep_SRCS=	usr.bin/sleep.c
csh_SRCS=	$(addprefix bin/csh/,alloc11.c printf.c sh.c sh.char.c \
		sh.dir.c sh.dol.c sh.err.c sh.exec.c sh.exec2.c sh.exp.c \
		sh.file.c sh.func.c sh.glob.c sh.hist.c sh.init.c sh.lex.c \
		sh.misc.c sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c \
		sh.time.c)
csh_FLAGS=	-DTELL -DVFORK -DFILEC
csh_MODEL=	m

# ex, which is also vi.  Its error messages are taken out of its C into
# a file, /usr/share/misc/exstrings, that it reads them from
# (tools/mkstr.py): the PDP-11 needed the room in its 64K of data, and
# so does the medium model.  The sources mkstr makes are in obj/ex.
EX=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c ex_data.c \
	ex_extern.c ex_get.c ex_io.c ex_put.c ex_re.c ex_set.c ex_subr.c \
	ex_tagio.c ex_temp.c ex_tty.c ex_unix.c ex_v.c ex_vadj.c ex_vget.c \
	ex_vmain.c ex_voper.c ex_vops.c ex_vops2.c ex_vops3.c ex_vput.c \
	ex_vwind.c printf.c
EXFLAGS=	-I$(SRC)/ucb/ex -DTABS=8 -DLISPCODE -DCHDIR -DVFORK -DFASTTAG \
		-DTEMP512K
EXO=	$(TOP)/obj/ex
ex_SRCS=	$(addprefix $(EXO)/,$(EX))
ex_FLAGS=	$(EXFLAGS)
ex_LIBS=	-ltermcap
ex_MODEL=	m
exrecover_SRCS=	ucb/ex/exrecover.c ucb/ex/ex_extern.c
exrecover_FLAGS= $(EXFLAGS)
expreserve_SRCS= ucb/ex/expreserve.c
expreserve_FLAGS= $(EXFLAGS)

# tcsh.  Its messages are taken out of its C into /etc/tcsh6.00strgs too,
# by its own kind of mkstr, mkprstr: a call to xprintf with a string
# becomes one to mkprintf with the string's place in the file
# (jpnerror.c).  Three of its headers are made from its C, as its
# Makefile makes them; they and the C mkstr makes are in obj/tcsh.
TCSH=	sh.c sh.dir.c sh.dol.c sh.err.c sh.exec.c sh.char.c sh.exp.c \
	sh.func.c sh.glob.c sh.hist.c sh.init.c sh.lex.c sh.misc.c \
	sh.parse.c sh.print.c sh.proc.c sh.sem.c sh.set.c sh.time.c glob.c \
	tw.help.c tw.init.c tw.parse.c tw.spell.c ed.chared.c ed.refresh.c \
	ed.screen.c ed.init.c ed.inputl.c ed.defns.c ed.xmap.c tc.alloc.c \
	tc.bind.c tc.const.c tc.disc.c tc.func.c tc.os.c tc.printf.c \
	tc.prompt.c tc.sched.c tc.str.c tc.vers.c tc.who.c alloc11.c \
	jpnerror.c
TCO=	$(TOP)/obj/tcsh
TCHDRS=	$(addprefix $(TCO)/,sh.err.h ed.defns.h tc.const.h)
tcsh_SRCS=	$(addprefix $(TCO)/,$(TCSH))
tcsh_FLAGS=	-I$(TCO) -I$(SRC)/bin/tcsh
tcsh_LIBS=	-ltermcap
tcsh_MODEL=	m

# The network's.
ifconfig_SRCS=	sbin/ifconfig/ifconfig.c
ping_SRCS=	bin/ping/ping.c
telnet_SRCS=	ucb/telnet.c
telnetd_SRCS=	libexec/telnetd/telnetd.c $(addprefix libexec/getty/,\
		gettytab.c subr.c init.c)
telnetd_FLAGS=	-I$(SRC)/libexec/getty
telnetd_LIBS=	-ltermcap
inetd_SRCS=	usr.sbin/inetd/inetd.c
route_SRCS=	sbin/route/route.c
arp_SRCS=	usr.sbin/arp/arp.c
NETPROGS=	ifconfig ping route arp telnet telnetd inetd

# Those that look after the file systems, the password file and users.
update_SRCS=	usr.sbin/update/update.c
fsck_SRCS=	$(addprefix sbin/fsck/,dir.c inode.c main.c pass1.c pass1b.c \
		pass2.c pass3.c pass4.c pass5.c setup.c utilities.c)
mkfs_SRCS=	sbin/mkfs/mkfs.c
newfs_SRCS=	sbin/newfs/newfs.c
passwd_SRCS=	bin/passwd/passwd.c
mkpasswd_SRCS=	sbin/mkpasswd/mkpasswd.c
vipw_SRCS=	usr.sbin/vipw/vipw.c
chown_SRCS=	usr.sbin/chown/chown.c
chpass_SRCS=	$(addprefix bin/chpass/,chpass.c field.c util.c)
chpass_FLAGS=	-I$(SRC)/bin/chpass
SYSPROGS=	update fsck mkfs newfs passwd mkpasswd vipw chown chpass

# Those that read a disk or a file system directly: disklabel, which
# reads and writes a disk's label; dcheck, icheck and ncheck, which check
# a file system's directories and blocks and name its files; clri, which
# clears an inode; badsect, which makes a file of a bad sector; and quot,
# which adds up what each user has on a file system.
disklabel_SRCS=	bin/disklabel/disklabel.c
disklabel_LIBS=	-lutil
dcheck_SRCS=	sbin/dcheck/dcheck.c
icheck_SRCS=	sbin/icheck/icheck.c
ncheck_SRCS=	sbin/ncheck/ncheck.c
clri_SRCS=	sbin/clri/clri.c
badsect_SRCS=	sbin/badsect/badsect.c
quot_SRCS=	usr.sbin/quot/quot.c
DISKPROGS=	disklabel dcheck icheck ncheck clri badsect quot

# Those that read the kernel's tables, and dev_mkdb, whose database
# devname(3) looks devices up in for them.  fstat lists the files each
# process has open.
ps_SRCS=	bin/ps/ps.c
ps_FLAGS=	-I$(SRC)/bin/ps
dev_mkdb_SRCS=	usr.sbin/dev_mkdb/dev_mkdb.c
w_SRCS=		ucb/w.c
vmstat_SRCS=	ucb/vmstat.c
iostat_SRCS=	usr.bin/iostat.c
netstat_SRCS=	$(addprefix ucb/netstat/,host.c inet.c if.c main.c mbuf.c \
		route.c unix.c ns.c)
pstat_SRCS=	usr.sbin/pstat/pstat.c
# top shows the processes that are using the processor, and tracebuf the
# kernel's counts of what its buffers saved the disk; sysctl asks the
# kernel, and needs neither its memory nor its names.
top_SRCS=	ucb/top/top.c
top_LIBS=	-lcurses -ltermcap
tracebuf_SRCS=	usr.sbin/tracebuf/tracebuf.c
sysctl_SRCS=	bin/sysctl/sysctl.c
fstat_SRCS=	usr.bin/fstat/fstat.c
KMEMPROGS=	ps w vmstat iostat netstat pstat dev_mkdb top tracebuf sysctl \
		fstat

# The system log and what the clock runs: syslogd, logger which writes
# to it and dmesg which shows what the kernel has printed; cron and
# crontab; at and what looks after its queue; the accounting of
# processes, accton and acctd which keep it and lastcomm and sa which
# show it; and find, with which /etc/rc clears /tmp.  syslogd reads all
# of the kernel's 4K of messages onto its stack and formats lines of 1K
# in each of three functions below that, more than the 8K a program's
# stack is here unless it is given another size.
syslogd_SRCS=	usr.sbin/syslogd/syslogd.c
syslogd_FLAGS=	-stack 12288
logger_SRCS=	ucb/logger.c
dmesg_SRCS=	sbin/dmesg/dmesg.c
CRON=		misc.c entry.c env.c compat.c
cron_SRCS=	$(addprefix usr.sbin/cron/,cron.c database.c user.c job.c \
		do_command.c popen.c $(CRON))
cron_FLAGS=	-I$(SRC)/usr.sbin/cron
crontab_SRCS=	$(addprefix usr.sbin/cron/,crontab.c $(CRON))
crontab_FLAGS=	-I$(SRC)/usr.sbin/cron
ATPROGS=	at atq atrm atrun
$(foreach p,$(ATPROGS),$(eval $(p)_SRCS = usr.bin/at/$(p).c))
accton_SRCS=	usr.sbin/accton/accton.c
acctd_SRCS=	libexec/acctd/acctd.c
lastcomm_SRCS=	ucb/lastcomm.c
sa_SRCS=	usr.sbin/sa/sa.c
find_SRCS=	usr.bin/find/find.c
LOGPROGS=	syslogd logger dmesg cron crontab $(ATPROGS) accton acctd \
		lastcomm sa find

# What /usr/adm/weekly runs to make find's list of the system's files,
# which find looks a name up in when given only the name: bigram and
# code, which updatedb, a script, runs.  And the time zones: zic, which
# makes the files in /usr/share/zoneinfo that localtime reads, and
# zdump, which shows them.
bigram_SRCS=	usr.bin/find/bigram.c
code_SRCS=	usr.bin/find/code.c
zic_SRCS=	$(addprefix share/zoneinfo/,zic.c scheck.c ialloc.c)
zdump_SRCS=	$(addprefix share/zoneinfo/,zdump.c ialloc.c)
ADMPROGS=	bigram code zic zdump

# Backups: dump, which copies a file system, or what has changed in it
# since a dump of a lower level, to a tape, and here to a file or a pipe;
# rdump, the same to another machine's file or tape, through rmt there,
# which serves it; dumpdir, which lists the files in a dump; and restor,
# which puts a dump back on a file system, or takes files out of it.
# And the rest of the administrator's: chflags, which sets the flags of
# a file that the kernel keeps; la, which shows the load averages; gcore,
# which writes a running process's core; and window, which shares a
# terminal among windows, each with its own shell, on a pty.
DUMP=		dumpitime.c dumpmain.c dumpoptr.c dumptape.c dumptraverse.c \
		unctime.c
dump_SRCS=	$(addprefix sbin/dump/,$(DUMP))
rdump_SRCS=	$(addprefix sbin/dump/,$(DUMP) dumprmt.c)
rdump_FLAGS=	-DRDUMP
dumpdir_SRCS=	sbin/dump/dumpdir.c
restor_SRCS=	sbin/restor/restor.c
rmt_SRCS=	usr.sbin/rmt/rmt.c
chflags_SRCS=	bin/chflags/chflags.c bin/ls/stat_flags.c
la_SRCS=	new/la/la.c
gcore_SRCS=	ucb/gcore.c
WINDOW=		char.c cmd.c cmd1.c cmd2.c cmd3.c cmd4.c cmd5.c cmd6.c cmd7.c \
		context.c error.c lcmd.c lcmd1.c lcmd2.c main.c mloop.c \
		parser1.c parser2.c parser3.c parser4.c parser5.c scanner.c \
		startup.c string.c ttf100.c ttgeneric.c tth19.c tth29.c \
		ttinit.c ttoutput.c tttermcap.c tttvi925.c var.c win.c wwadd.c \
		wwalloc.c wwbox.c wwchild.c wwclose.c wwclreol.c wwclreos.c \
		wwcursor.c wwdata.c wwdelchar.c wwdelete.c wwdelline.c \
		wwdump.c wwend.c wwenviron.c wwerror.c wwflush.c wwframe.c \
		wwgets.c wwinit.c wwinschar.c wwinsline.c wwiomux.c wwlabel.c \
		wwmisc.c wwmove.c wwopen.c wwprintf.c wwpty.c wwputc.c \
		wwputs.c wwredraw.c wwredrawwin.c wwrint.c wwscroll.c \
		wwsize.c wwspawn.c wwsuspend.c wwtty.c wwunframe.c wwupdate.c \
		wwwrite.c
window_SRCS=	$(addprefix ucb/window/,$(WINDOW))
window_LIBS=	-ltermcap
# The large model: its copies of the screen and of each window's
# contents are more than a data segment holds on a terminal of any
# size.
window_MODEL=	l
BACKPROGS=	dump rdump dumpdir restor rmt chflags la gcore window

# Those with a grammar for yacc, and yacc and lex themselves.  The
# grammars are made into C by the tree's own yacc and lex, built for the
# machine the system is built on (below) in obj/host, and the C is in
# obj/gen.
H=		$(TOP)/obj/host
G=		$(TOP)/obj/gen
YSRC=		$(SRC)/usr.bin/yacc
LSRC=		$(SRC)/usr.bin/lex
expr_SRCS=	$(G)/expr.c
yacc_SRCS=	$(addprefix usr.bin/yacc/,y1.c y2.c y3.c y4.c)
lex_SRCS=	$(addprefix usr.bin/lex/,lmain.c sub1.c sub2.c header.c) \
		$(G)/lex.c
lex_FLAGS=	-Dunix -I$(LSRC)
# awk's are its grammar and its scanner, in obj/gen/awk with what its
# Makefile makes besides: awk.h, the tokens' numbers as yacc gives them;
# token.c with the table of their names made from that, as the ex script
# there makes it; and proctab.c, the function that runs each token,
# which a program of awk's own, proc, writes.
AWKG=		$(G)/awk
awk_SRCS=	$(addprefix $(AWKG)/,awk.g.c awk.lx.c token.c proctab.c) \
		$(addprefix bin/awk/,b.c main.c tran.c lib.c run.c parse.c \
		freeze.c)
awk_FLAGS=	-I$(AWKG) -I$(SRC)/bin/awk
awk_LIBS=	-lm
egrep_SRCS=	$(G)/egrep.c
bc_SRCS=	$(G)/bc.c
dc_SRCS=	usr.bin/dc/dc.c
make_SRCS=	$(addprefix bin/make/,ident.c main.c doname.c misc.c files.c \
		dosys.c) $(G)/make.c
make_FLAGS=	-I$(SRC)/bin/make -I$(SRC)/bin/ar
YACCPROGS=	expr yacc lex awk egrep bc dc make

# Those that work on text and files and need nothing but themselves:
# of one file in usr.bin or ucb, or in a directory of their own.  clear,
# ul, more and tset read the termcap; crypt has makekey make its key;
# install is xinstall here as it is in the tree, and takes ls's flags.
USRBINSTD= basename cal cb checkeq col comm crypt deroff fgrep file join \
	look mesg ptx rev sort spline split sum tabs tr tsort tty uniq units
$(foreach p,$(USRBINSTD),$(eval $(p)_SRCS = usr.bin/$(p).c))
UCBSTD=	apply checknr clear colcrt colrm ctags expand fold head last leave \
	printenv script soelim strings tail ul unexpand unifdef users what \
	wc xstr yes
$(foreach p,$(UCBSTD),$(eval $(p)_SRCS = ucb/$(p).c))
clear_LIBS=	-ltermcap
ul_LIBS=	-ltermcap
USRBINDIR= id printf renice touch uname whereis xargs
$(foreach p,$(USRBINDIR),$(eval $(p)_SRCS = usr.bin/$(p)/$(p).c))
diff3_SRCS=	usr.bin/diff3/diff3.c
m4_SRCS=	$(addprefix usr.bin/m4/,eval.c expr.c look.c main.c misc.c)
m4_FLAGS=	-DEXTENDED -I$(SRC)/usr.bin/m4
patch_SRCS=	$(addprefix usr.bin/patch/,patch.c pch.c inp.c util.c \
		version.c)
shar_SRCS=	usr.bin/shar/shar.c usr.bin/shar/traverse.c
traverse_SRCS=	usr.bin/shar/traverse.c
traverse_FLAGS=	-DSTANDALONE
xinstall_SRCS=	usr.bin/xinstall/xinstall.c bin/ls/stat_flags.c
compress_SRCS=	ucb/compress/compress.c
lock_SRCS=	ucb/lock/lock.c
mkstr_SRCS=	ucb/mkstr/mkstr.c
more_SRCS=	ucb/more/more.c
more_LIBS=	-ltermcap
tset_SRCS=	ucb/tset/tset.c
tset_LIBS=	-ltermcap
su_SRCS=	bin/su.c
tar_SRCS=	bin/tar.c
sed_SRCS=	bin/sed/sed0.c bin/sed/sed1.c
diff_SRCS=	$(addprefix bin/diff/,diff.c diffdir.c diffreg.c)
diff_FLAGS=	'-DDIFF="/bin/diff"' '-DDIFFH="/usr/libexec/diffh"' \
		'-DPR="/bin/pr"'
diffh_SRCS=	bin/diff/diffh.c
strip_SRCS=	bin/strip.c
makekey_SRCS=	libexec/makekey/makekey.c
chroot_SRCS=	usr.sbin/chroot/chroot.c
ac_SRCS=	usr.sbin/ac/ac.c
shutdown_SRCS=	sbin/shutdown/shutdown.c
TEXTPROGS=	$(USRBINSTD) $(UCBSTD) $(USRBINDIR) diff3 m4 patch shar \
		traverse xinstall compress lock mkstr more tset su tar sed \
		diff diffh strip makekey chroot ac shutdown

# The network's other programs.  The remote login, shell and copy, and
# their daemons, with rexecd; ftp, tftp, finger and talk, and theirs;
# whois, nc and traceroute; rwhod, which tells the network who is logged
# in, for rwho and ruptime; routed, timed and ntpd, which keep the
# routes and the clock with other machines.  rshd, rexecd and ftpd hold
# a command line or a pattern's expansion on the stack, and ftp the
# latter.  ftpd's commands are a grammar.  timed's checksum is in a
# directory for the machine, as the PDP-11's is.
rlogin_SRCS=	ucb/rlogin/rlogin.c
rsh_SRCS=	ucb/rsh.c
rcp_SRCS=	bin/rcp.c
rlogind_SRCS=	libexec/rlogind/rlogind.c
rlogind_LIBS=	-lutil
rshd_SRCS=	libexec/rshd/rshd.c
rshd_FLAGS=	-stack 16384
rexecd_SRCS=	libexec/rexecd/rexecd.c
rexecd_FLAGS=	-stack 16384
ftp_SRCS=	$(addprefix ucb/ftp/,cmds.c cmdtab.c ftp.c glob.c main.c \
		ruserpass.c domacro.c)
# -Dunix: ftp asks the server what it is only if this is UNIX with bytes
# of 8 bits (cmds.c), and takes files as they are, not as text, from one
# that says it is too.  Without it every file came as text, a character
# at a time, 29 K/s where 130 K/s is had, and less its CRs before LFs.
ftp_FLAGS=	-I$(SRC)/ucb/ftp -Dunix -stack 16384
ftpd_SRCS=	$(addprefix libexec/ftpd/,ftpd.c glob.c logwtmp.c popen.c \
		vers.c) $(G)/ftpcmd.c
ftpd_FLAGS=	-DSETPROCTITLE -Dunix -stack 16384
tftp_SRCS=	$(addprefix ucb/tftp/,main.c tftp.c tftpsubs.c)
tftpd_SRCS=	libexec/tftpd/tftpd.c libexec/tftpd/tftpsubs.c
finger_SRCS=	$(addprefix ucb/finger/,finger.c lprint.c net.c sprint.c \
		util.c)
finger_FLAGS=	-DSECSPERDAY=86400 -DDAYSPERNYEAR=365
fingerd_SRCS=	libexec/fingerd/fingerd.c
talk_SRCS=	$(addprefix ucb/talk/,talk.c get_names.c display.c io.c \
		ctl.c init_disp.c msgs.c get_addrs.c ctl_transact.c \
		invite.c look_up.c)
talk_LIBS=	-lcurses -ltermcap
talkd_SRCS=	$(addprefix libexec/talkd/,announce.c print.c process.c \
		table.c talkd.c)
whois_SRCS=	ucb/whois.c
rwho_SRCS=	ucb/rwho.c
ruptime_SRCS=	ucb/ruptime.c
rwhod_SRCS=	usr.sbin/rwhod/rwhod.c
nc_SRCS=	usr.sbin/nc/netcat.c usr.sbin/nc/inet_aton.c
nc_FLAGS=	-DGENERIC
traceroute_SRCS= usr.sbin/traceroute/traceroute.c
routed_SRCS=	$(addprefix sbin/routed/,af.c if.c input.c main.c output.c \
		startup.c tables.c timer.c trace.c inet.c)
TIMEDCOMM=	byteorder.c measure.c i286/cksum.c
timed_SRCS=	$(addprefix usr.sbin/timed/,acksend.c candidate.c correct.c \
		master.c networkdelta.c readmsg.c slave.c timed.c \
		$(TIMEDCOMM))
timed_FLAGS=	-DMEASURE
timedc_SRCS=	$(addprefix usr.sbin/timed/,cmds.c cmdtab.c timedc.c \
		$(TIMEDCOMM))
timedc_FLAGS=	-DMEASURE
NTPFLAGS=	-DGENERIC_UNS_BUG -DSUN_FLT_BUG -DDEBUG -DLOG_NTP=LOG_LOCAL3
ntp_SRCS=	usr.sbin/ntp/ntp.c usr.sbin/ntp/ntpsubs.c
ntp_FLAGS=	$(NTPFLAGS)
ntpd_SRCS=	$(addprefix usr.sbin/ntp/,ntpd.c ntpsubs.c ntp_proto.c \
		ntp_sock.c ntp_adjust.c read_local.c read_psti.c)
ntpd_FLAGS=	$(NTPFLAGS)
ntpdc_SRCS=	usr.sbin/ntp/ntpdc.c
ntpdc_FLAGS=	$(NTPFLAGS)
# ntest is ntp's check of its arithmetic, which its Makefile runs when
# it has built the rest (tests/network runs it).
ntest_SRCS=	usr.sbin/ntp/test.c usr.sbin/ntp/ntpsubs.c
ntest_FLAGS=	$(NTPFLAGS)
# tcpd stands between inetd and a daemon, and lets a connection through
# or not by /etc/hosts.allow and /etc/hosts.deny; what it and its
# companions share is a library, libwrap, as its Makefile has it, from
# which each takes what it does not define itself.  A rule's options
# and a banner's text are on the stack.
WRAPFLAGS=	-DFACILITY=LOG_DAEMON -DHOSTS_ACCESS -DPARANOID \
		-DDAEMON_UMASK=022 '-DREAL_DAEMON_DIR="/usr/libexec"' \
		-DKILL_IP_OPTIONS -DSEVERITY=LOG_INFO -DRFC931_TIMEOUT=10 \
		'-DHOSTS_DENY="/etc/hosts.deny"' \
		'-DHOSTS_ALLOW="/etc/hosts.allow"' -DALWAYS_HOSTNAME
WRAPPROG=	$(WRAPFLAGS) -I$(SRC)/libexec/tcpd -stack 16384
tcpd_SRCS=	libexec/tcpd/tcpd.c
tcpd_FLAGS=	$(WRAPPROG)
tcpd_LIBS=	-lwrap
tcpdmatch_SRCS=	$(addprefix libexec/tcpd/,tcpdmatch.c fakelog.c inetcf.c \
		scaffold.c)
tcpdmatch_FLAGS= $(WRAPPROG)
tcpdmatch_LIBS=	-lwrap
tcpdchk_SRCS=	$(addprefix libexec/tcpd/,tcpdchk.c fakelog.c inetcf.c \
		scaffold.c)
tcpdchk_FLAGS=	$(WRAPPROG)
tcpdchk_LIBS=	-lwrap
try-from_SRCS=	libexec/tcpd/try-from.c libexec/tcpd/fakelog.c
try-from_FLAGS=	$(WRAPPROG)
try-from_LIBS=	-lwrap
safe_finger_SRCS= libexec/tcpd/safe_finger.c
safe_finger_FLAGS= $(WRAPFLAGS)
NET2PROGS=	rlogin rsh rcp rlogind rshd rexecd ftp ftpd tftp tftpd \
		finger fingerd talk talkd whois rwho ruptime rwhod nc \
		traceroute routed timed timedc ntp ntpd ntpdc ntest tcpd \
		tcpdmatch tcpdchk try-from safe_finger

# Mail.  sendmail takes a letter from whoever sends one, here or over
# the network, and hands it to what delivers it: /bin/mail for a user
# here, which also reads a mailbox the old way, or SMTP for another
# machine.  Mail is what a user reads and sends mail with, and fmt
# fills its lines.  sendmail had three overlays on the PDP-11 and is
# medium-model here.  There it was linked with libstubs, which has
# ctimed, another process, do localtime and the like to save room in
# its code; it is linked without here, and has the library's own.  Its
# aliases are a dbm database, through libdbm.  Each holds a message's
# header on the stack, and Mail its commands' arguments.
SENDMAIL=	conf.c main.c collect.c parseaddr.c alias.c deliver.c \
		savemail.c err.c readcf.c stab.c headers.c recipient.c \
		stats.c daemon.c usersmtp.c srvrsmtp.c queue.c macro.c util.c \
		clock.c trace.c envelope.c sysexits.c arpadate.c convtime.c \
		Version.c
sendmail_SRCS=	$(addprefix usr.sbin/sendmail/src/,$(SENDMAIL))
sendmail_FLAGS=	-I$(SRC)/usr.sbin/sendmail/include -DVMUNIX -DNOSCCS \
		-stack 16384
sendmail_LIBS=	-ldbm
sendmail_MODEL=	m
mail_SRCS=	bin/mail.c
Mail_SRCS=	$(addprefix ucb/Mail/,version.c aux.c cmd1.c cmd2.c cmd3.c \
		cmdtab.c collect.c edit.c fio.c getname.c head.c v7.local.c \
		lex.c list.c main.c names.c popen.c quit.c send.c strings.c \
		temp.c tty.c vars.c)
Mail_FLAGS=	-stack 16384
fmt_SRCS=	ucb/Mail/fmt.c ucb/Mail/head.c
# What waited for mail: comsat, which tells a user logged in that mail
# has come, if biff has said to; from, which lists who it is from; msgs,
# the system's messages, which mail to msgs adds to; vacation, which
# answers mail while its user is away, and keeps whom it has answered in
# a dbm database; and popper, which lets another machine fetch a user's
# mail by POP3.
comsat_SRCS=	libexec/comsat/comsat.c
biff_SRCS=	ucb/biff.c
from_SRCS=	ucb/from.c
msgs_SRCS=	ucb/msgs/msgs.c
msgs_LIBS=	-ltermcap
vacation_SRCS=	ucb/vacation.c
vacation_LIBS=	-ldbm
popper_SRCS=	$(addprefix libexec/popper/,pop_dele.c pop_dropcopy.c \
		pop_dropinfo.c pop_get_command.c pop_get_subcommand.c \
		pop_init.c pop_last.c pop_list.c pop_log.c pop_lower.c \
		pop_msg.c pop_parse.c pop_pass.c pop_quit.c pop_rset.c \
		pop_send.c pop_stat.c pop_updt.c pop_user.c pop_xtnd.c \
		pop_xmit.c popper.c)
popper_FLAGS=	-DBIND43
MAILPROGS=	sendmail mail Mail fmt comsat biff from msgs vacation popper

# nroff, which formats text for a terminal or a printer, by the macros
# in /usr/share/tmac and a printer's table in /usr/share/term (below);
# man, which shows a page of the manual, formatted already (the manual,
# below); and apropos, also whatis, which finds pages by what their
# names say.  nroff had separate I and D on the PDP-11, and is small here.
# Upstream's preprocessor says __BSD2_11__, which is where nroff finds
# its paths; this compiler is told.
NROFF=		$(addprefix common_source/,ni.c nii.c n1.c n2.c n3.c n4.c \
		n5.c) nroff/n6.c $(addprefix common_source/,n7.c n8.c n9.c) \
		nroff/n10.c common_source/hytab.c nroff/ntab.c \
		common_source/suftab.c
nroff_SRCS=	$(addprefix usr.bin/roff/,$(NROFF))
nroff_FLAGS=	-DNROFF -D__BSD2_11__ -I$(SRC)/usr.bin/roff/common_source
man_SRCS=	ucb/man/man.c
apropos_SRCS=	ucb/man/apropos.c
DOCPROGS=	nroff man apropos

# What documents go through on their way to nroff, and what reads their
# words: tbl, which lays out tables; eqn and neqn, which set mathematics
# for a typesetter and for nroff, both of one grammar (below); refer,
# which finds references and cites them, with what it runs in
# /usr/libexec/refer, and addbib, lookbib and sortbib; spell, in
# /usr/libexec, and spellin and spellout, which make and read its lists
# of words; style and diction, which judge a document's sentences and
# phrases, of scanners made by lex (below); and indent, which lays out C.
# refer's hunt is refer.hunt here, beside the game.  tbl and spell's
# three want unix defined, as the PDP-11's compiler has it: tbl keeps its
# handler of SIGPIPE under it, and spell.h takes the Honeywell's table
# without it.  spell and its two have a table of 50000 bytes, and
# diction one of 52000: each has a stack of 4K, half the usual, which
# leaves them room for their arguments and stdio's buffers.  refer reads
# a reference into 4.5K of its stack, under the 1K and more of its
# caller and what it runs of hunt's, and sortbib a record into 9K.
tbl_SRCS=	$(addprefix usr.bin/tbl/,t0.c t1.c t2.c t3.c t4.c t5.c t6.c \
		t7.c t8.c t9.c tb.c tc.c te.c tf.c tg.c ti.c tm.c ts.c tt.c \
		tu.c tv.c)
tbl_FLAGS=	-Dunix
EQN=		$(G)/eqn/e.c $(addprefix usr.bin/eqn/,diacrit.c eqnbox.c \
		font.c fromto.c funny.c glob.c integral.c io.c lex.c lookup.c \
		mark.c matrix.c move.c over.c paren.c pile.c shift.c size.c \
		sqrt.c text.c)
eqn_SRCS=	$(EQN)
eqn_FLAGS=	-I$(G)/eqn -I$(SRC)/usr.bin/eqn
neqn_SRCS=	$(EQN)
neqn_FLAGS=	-DNEQN $(eqn_FLAGS)
RF=		usr.bin/refer
deliv_SRCS=	$(addprefix $(RF)/,deliv1.c deliv2.c)
mkey_SRCS=	$(addprefix $(RF)/,mkey1.c mkey2.c mkey3.c deliv2.c)
inv_SRCS=	$(addprefix $(RF)/,inv1.c inv2.c inv3.c inv5.c inv6.c deliv2.c)
refer.hunt_SRCS= $(addprefix $(RF)/,hunt1.c hunt2.c hunt3.c hunt5.c hunt6.c \
		hunt7.c hunt8.c hunt9.c glue4.c glue5.c refer3.c shell.c \
		deliv2.c tick.c)
refer_SRCS=	$(addprefix $(RF)/,glue1.c glue2.c glue3.c glue4.c glue5.c \
		refer0.c refer1.c refer2.c refer4.c refer5.c refer6.c refer7.c \
		refer8.c hunt2.c hunt3.c hunt5.c hunt6.c hunt7.c hunt8.c \
		hunt9.c mkey3.c shell.c deliv2.c)
refer_FLAGS=	-stack 16384
addbib_SRCS=	$(RF)/addbib.c
lookbib_SRCS=	$(RF)/lookbib.c
sortbib_SRCS=	$(RF)/sortbib.c
sortbib_FLAGS=	-stack 16384
$(foreach p,spell spellin spellout,$(eval $(p)_SRCS = usr.bin/spell/$(p).c) \
		$(eval $(p)_FLAGS = -Dunix -stack 4096))
DI=		$(SRC)/usr.bin/diction
style1_SRCS=	$(G)/diction/wdslex.c
style2_SRCS=	$(G)/diction/endlex.c
style3_SRCS=	$(G)/diction/prtlex.c usr.bin/diction/pscan.c \
		usr.bin/diction/outp.c
$(foreach p,style1 style2 style3,$(eval $(p)_FLAGS = -I$(DI)) \
		$(eval $(p)_LIBS = -lln))
dprog_SRCS=	usr.bin/diction/diction.c
dprog_FLAGS=	'-DDICT="/usr/share/misc/dict.d"' -stack 4096
indent_SRCS=	$(addprefix ucb/indent/,indent.c io.c lexi.c parse.c \
		pr_comment.c args.c)
WRITERPROGS=	tbl eqn neqn deliv mkey inv refer.hunt refer addbib lookbib \
		sortbib spell spellin spellout style1 style2 style3 dprog \
		indent

# The games, in /usr/games, with their data and scores in
# /usr/games/lib, as the games' Makefiles install them (docs/userland.md).
# Those of one file in games, and those in a directory of their own.
GAMESTD=	arithmetic banner bcd cfscores factor fish morse number ppt \
		wump primes canfield rain worm worms
$(foreach p,$(GAMESTD),$(eval $(p)_SRCS = games/$(p).c))
primes_LIBS=	-lm
$(foreach p,canfield rain worm worms,$(eval $(p)_LIBS = -lcurses -ltermcap))
CURSES=		-lcurses -ltermcap
GS=		games
GO=		$(TOP)/obj/games
# adventure's first run, which makes the game on the system
# (i286/adventure/Makefile).
adventure_SRCS=	$(addprefix $(GS)/adventure/,done.c init.c io.c main.c \
		save.c subr.c vocab.c wizard.c)
atc_SRCS=	$(addprefix $(GS)/atc/,extern.c graphics.c input.c list.c \
		log.c main.c tunable.c update.c) $(GO)/atc/grammar.c \
		$(GO)/atc/lex.c
atc_FLAGS=	-DBSD '-DDEST="/usr/games/lib/atc/"' -I$(GO)/atc \
		-I$(SRC)/games/atc
atc_LIBS=	-lln -lm $(CURSES)
BG=		allow.c board.c check.c fancy.c init.c odds.c one.c save.c \
		subs.c table.c
backgammon_SRCS= $(addprefix $(GS)/backgammon/,$(BG) extra.c main.c move.c \
		text.c message.c)
backgammon_FLAGS= -DV7
backgammon_LIBS= -ltermcap
teachgammon_SRCS= $(addprefix $(GS)/backgammon/,$(BG) data.c teach.c \
		ttext1.c ttext2.c tutor.c)
teachgammon_FLAGS= -DV7
teachgammon_LIBS= -ltermcap
battlestar_SRCS= $(addprefix $(GS)/battlestar/,battlestar.c com1.c com2.c \
		com3.c com4.c com5.c com6.c com7.c init.c cypher.c getcom.c \
		parse.c room.c save.c fly.c misc.c dayobjs.c nightobjs.c \
		words.c) $(addprefix $(GO)/battlestar/xx,dayfile.c \
		nightfile.c globals.c)
battlestar_FLAGS= -I$(SRC)/games/battlestar
battlestar_LIBS= $(CURSES)
boggle_SRCS=	$(GS)/boggle/boggle.c
btlgammon_SRCS=	$(GS)/btlgammon/btlgammon.c
cribbage_SRCS=	$(addprefix $(GS)/cribbage/,extern.c crib.c support.c cards.c \
		score.c io.c)
cribbage_LIBS=	$(CURSES)
fortune_SRCS=	$(GS)/fortune/fortune.c
hangman_SRCS=	$(addprefix $(GS)/hangman/,endgame.c extern.c getguess.c \
		getword.c main.c playgame.c prdata.c prman.c prword.c setup.c)
hangman_LIBS=	$(CURSES)
HUNTDEFS=	-I$(SRC)/games/hunt -DBROADCAST -DRANDOM -DREFLECT -DMONITOR \
		-DINTERNET -DOOZE -DFLY -DVOLCANO
hunt_SRCS=	$(addprefix $(GS)/hunt/,hunt.c connect.c playit.c pathname.c)
hunt_FLAGS=	$(HUNTDEFS)
hunt_LIBS=	$(CURSES)
hunt.driver_SRCS= $(addprefix $(GS)/hunt/,answer.c driver.c draw.c execute.c \
		expl.c makemaze.c shots.c terminal.c extern.c pathname.c)
hunt.driver_FLAGS= $(HUNTDEFS)
mille_SRCS=	$(addprefix $(GS)/mille/,comp.c end.c extern.c init.c mille.c \
		misc.c move.c print.c roll.c save.c types.c varpush.c)
mille_LIBS=	$(CURSES)
monop_SRCS=	$(addprefix $(GS)/monop/,monop.c cards.c execute.c getinp.c \
		houses.c jail.c misc.c morg.c print.c prop.c rent.c roll.c \
		spec.c strcmp.c trade.c)
# phantasia's settings are its Makefile's, but for -Dpdp11, which is the
# PDP-11's own.
phantasia_SRCS=	$(addprefix $(GS)/phantasia/,main.c fight.c func0.c func1.c \
		func2.c)
phantasia_FLAGS= -I$(SRC)/games/phantasia \
		'-DPATH="/usr/games/lib/phantasia"' '-DWIZARD="daemon"' \
		-DUID=1 -DRAND=32768.0 -DENEMY
phantasia_LIBS=	-lm $(CURSES)
phantasia_MODEL= m
quiz_SRCS=	$(GS)/quiz/quiz.c
robots_SRCS=	$(addprefix $(GS)/robots/,extern.c init_field.c main.c \
		make_level.c move.c move_robs.c play_level.c query.c rnd_pos.c \
		score.c flush_in.c)
robots_FLAGS=	-DMAX_PER_UID=5
robots_LIBS=	$(CURSES)
rogue_SRCS=	$(addprefix $(GS)/rogue/,curses.c hit.c init.c inventory.c \
		level.c machdep.c main.c message.c monster.c move.c object.c \
		pack.c play.c random.c ring.c room.c save.c score.c spec_hit.c \
		throw.c trap.c use.c zap.c)
rogue_FLAGS=	-DUNIX -DUNIX_BSD4_2
rogue_LIBS=	$(CURSES)
rogue_MODEL=	m
sail_SRCS=	$(addprefix $(GS)/sail/,main.c pl_main.c pl_1.c pl_2.c pl_3.c \
		pl_4.c pl_5.c pl_6.c pl_7.c dr_main.c dr_1.c dr_2.c dr_3.c \
		dr_4.c dr_5.c lo_main.c assorted.c game.c globals.c misc.c \
		parties.c sync.c version.c)
sail_LIBS=	$(CURSES)
snake_SRCS=	$(GS)/snake/snake.c $(GS)/snake/move.c
snake_FLAGS=	-DCHECKBUSY
snake_LIBS=	-lm -ltermcap
snscore_SRCS=	$(GS)/snake/snscore.c
snscore_FLAGS=	-DCHECKBUSY
busy_SRCS=	$(GS)/snake/busy.c
busy_FLAGS=	-DCHECKBUSY
trek_SRCS=	$(addprefix $(GS)/trek/,abandon.c attack.c autover.c \
		capture.c check_out.c checkcond.c compkl.c computer.c damage.c \
		damaged.c dcrept.c destruct.c dock.c dumpgame.c dumpme.c \
		dumpssradio.c events.c externs.c getcodi.c getpar.c help.c \
		impulse.c initquad.c kill.c klmove.c lose.c lrscan.c main.c \
		move.c nova.c out.c phaser.c play.c ram.c ranf.c rest.c \
		schedule.c score.c setup.c setwarp.c shell.c shield.c snova.c \
		srscan.c systemname.c torped.c utility.c visual.c warp.c win.c \
		cgetc.c)
trek_LIBS=	-lm
# hack's sources, each run through the preprocessor and then crypt, a
# program of hack's own built for this machine, which shortens the words
# of the messages in its strings, as hack's Makefile has it.  The
# preprocessor is the compiler's, the system's own with CCOM=1, whose
# hack has its sources in a directory of its own.
HGO=		$(GO)/$(if $(CCOM),nhack,hack)
HACK=		hack.Decl.c hack.apply.c hack.bones.c hack.c hack.cmd.c \
		hack.do.c hack.do_name.c hack.do_wear.c hack.dog.c hack.eat.c \
		hack.end.c hack.engrave.c hack.fight.c hack.invent.c \
		hack.ioctl.c hack.lev.c hack.main.c hack.makemon.c \
		hack.mhitu.c hack.mklev.c hack.mkmaze.c hack.mkobj.c \
		hack.mkshop.c hack.mon.c hack.monst.c hack.o_init.c \
		hack.objnam.c hack.options.c hack.pager.c hack.potion.c \
		hack.pri.c hack.read.c hack.rip.c hack.rumors.c hack.save.c \
		hack.search.c hack.shk.c hack.shknam.c hack.steal.c \
		hack.termcap.c hack.timeout.c hack.topl.c hack.track.c \
		hack.trap.c hack.tty.c hack.unix.c hack.u_init.c hack.vault.c \
		hack.wield.c hack.wizard.c hack.worm.c hack.worn.c hack.zap.c \
		hack.version.c rnd.c alloc.c
hack_SRCS=	$(addprefix $(HGO)/,$(HACK))
hack_LIBS=	-ltermcap
hack_MODEL=	m
warp_SRCS=	$(addprefix $(GS)/warp/,bang.c init.c intrp.c move.c object.c \
		play.c score.c sig.c term.c them.c us.c util.c version.c \
		warp.c weapon.c)
warp_LIBS=	-ltermcap -lm
warp_MODEL=	m
words1_SRCS=	$(GS)/words/words1.c
GAMEPROGS=	$(GAMESTD) adventure atc backgammon teachgammon battlestar boggle \
		btlgammon cribbage fortune hangman hunt hunt.driver mille \
		monop quiz robots rogue sail snake snscore busy trek warp \
		words1 hack phantasia

# This port's own (docs/userland.md), each in a directory here.
adduser_SRCS=	i286/adduser/adduser.c
bench_SRCS=	i286/bench/bench.c
# ve keeps the file it edits in segments it asks the system for
# (segalloc(2)), so it is of the medium model for its code alone.
ve_SRCS=	$(addprefix i286/ve/,main.c term.c screen.c text.c edit.c \
		normal.c insert.c search.c ex.c fmem.s)
ve_MODEL=	m
ve_LIBS=	-ltermcap
OWNPROGS=	adduser bench ve

# What a program is made with on the system (docs/toolchain.md): cc,
# which runs the rest; /lib/cpp; the compiler's two passes and its
# object code improver, /lib/c0, c1 and c2, the second pass and the
# improver being the 286's own (lib/ccom/i286, lib/c2/i286); as and ld,
# which are the 286's own too (bin/as/i286, bin/ld/i286); and ar,
# ranlib, nm and size.  cpp is given the size of a pointer, and keeps
# what it has in memory and not in a file (LIBVMF).  nm and ranlib read
# libraries with ar's header.  The compiler's passes and the improver
# are compiled by the compiler, whichever one compiles the rest
# (NAME_CCOM, below): they are written for it, and the passes are more
# than the cross compiler will take, a function being called there with
# as many arguments as the call has use for.
cc_SRCS=	bin/cc.c
cpp_SRCS=	$(addprefix lib/cpp/,cpp.c cpc.c token.c)
cpp_FLAGS=	-DLIBVMF=0 -DSIZEOF_INT_P=2 -I$(SRC)/lib/cpp
c0_SRCS=	$(addprefix lib/ccom/,c00.c c01.c c02.c c03.c c04.c c05.c \
		ansi.c)
c1_SRCS=	$(addprefix lib/ccom/i286/,c10.c c11.c c12.c c13.c c14.c)
c2_SRCS=	lib/c2/i286/c20.c lib/c2/i286/c21.c
c0_CCOM=	1
c1_CCOM=	1
c2_CCOM=	1
as_SRCS=	$(addprefix bin/as/i286/,as0.c as1.c as2.c as3.c)
ld_SRCS=	bin/ld/i286/ld.c
ar_SRCS=	$(addprefix bin/ar/,append.c ar.c archive.c contents.c \
		delete.c extract.c misc.c move.c print.c replace.c strmode.c)
ar_FLAGS=	-I$(SRC)/bin/ar
ranlib_SRCS=	$(addprefix usr.bin/ranlib/,build.c misc.c ranlib.c touch.c) \
		bin/ar/archive.c
ranlib_FLAGS=	-I$(SRC)/bin/ar
nm_SRCS=	bin/nm/nm.c
nm_FLAGS=	-I$(SRC)/bin/ar
size_SRCS=	bin/size.c
CCPROGS=	cc cpp c0 c1 c2 as ld ar ranlib nm size

# What waited for the compiler: calendar's program, which writes the
# dates that its script has /lib/cpp and egrep look for; and learn,
# whose lessons in C have cc compile what is typed (share/learn, below),
# with the two programs its lessons run.  lint and mkdep are scripts,
# and liby is with the libraries.
calendar_SRCS=	usr.bin/calendar/calendar.c
LEARNFLAGS=	-DBSD4_2 '-DLOGDIR="/var/log/learn"'
learn_SRCS=	$(addprefix usr.bin/learn/,copy.c dounit.c getlesson.c \
		learn.c list.c mem.c makpipe.c maktee.c mysys.c selsub.c \
		selunit.c start.c whatnow.c wrapup.c)
learn_FLAGS=	$(LEARNFLAGS)
lrntee_SRCS=	usr.bin/learn/lrntee.c
lrntee_FLAGS=	$(LEARNFLAGS)
lcount_SRCS=	usr.bin/learn/lcount.c
lcount_FLAGS=	$(LEARNFLAGS)
CCUSEPROGS=	calendar learn lrntee lcount

PROGS=	init getty login sh csh ls mkdir stty test hostname mknod mount \
	umount reboot sleep ex exrecover expreserve tcsh $(BINSTD) \
	$(NETPROGS) $(SYSPROGS) $(DISKPROGS) $(KMEMPROGS) $(LOGPROGS) \
	$(ADMPROGS) $(BACKPROGS) $(YACCPROGS) $(TEXTPROGS) $(NET2PROGS) \
	$(MAILPROGS) $(DOCPROGS) $(WRITERPROGS) $(GAMEPROGS) $(OWNPROGS) \
	$(CCPROGS) $(CCUSEPROGS)

# The libraries the programs use besides the C library: NAME_LIB lists a
# library's sources, and NAME_LIBFLAGS what it is compiled with.  Each is
# made, in obj/lib/M/libNAME.lib, for each of the three models.
util_LIB=	$(addprefix usr.lib/libutil/,login.c logout.c logwtmp.c \
		dkcksum.c)
termcap_LIB=	$(addprefix usr.lib/libtermlib/,termcap.c tgoto.c tputs.c)
termcap_LIBFLAGS= -DCM_N -DCM_GT -DCM_B -DCM_D -DNOSCCS
# libm is the library of mathematical functions that upstream installs
# under that name, libom, the older of its two and the one in C.
m_LIB=		$(addprefix usr.lib/libom/,asin.c atan.c hypot.c jn.c j0.c \
		j1.c pow.c gamma.c log.c sin.c sqrt.c tan.c tanh.c sinh.c \
		erf.c exp.c floor.c)
curses_LIB=	$(addprefix usr.lib/libcurses/,addch.c addstr.c box.c clear.c \
		clrtobot.c clrtoeol.c cr_put.c cr_tty.c curses.c delch.c \
		deleteln.c delwin.c endwin.c erase.c fullname.c getch.c \
		getstr.c idlok.c id_subwins.c initscr.c insch.c insertln.c \
		longname.c move.c mvprintw.c mvscanw.c mvwin.c newwin.c \
		overlay.c overwrite.c printw.c putchar.c refresh.c scanw.c \
		scroll.c toucholap.c standout.c touchwin.c tstp.c unctrl.c)
curses_LIBFLAGS= -DNOSCCS
wrap_LIB=	$(addprefix libexec/tcpd/,hosts_access.c options.c shell_cmd.c \
		rfc931.c eval.c hosts_ctl.c refuse.c percent_x.c \
		clean_exit.c fix_options.c socket.c tli.c workarounds.c \
		update.c misc.c diag.c percent_m.c)
wrap_LIBFLAGS=	$(WRAPFLAGS)
dbm_LIB=	usr.lib/libdbm/dbm.c
dbm_LIBFLAGS=	-DNOSCCS
# lex's library, libln, which upstream also installs as libl: yywrap for
# atc's scanner.
ln_LIB=		$(addprefix usr.lib/libln/,allprint.c main.c reject.c yyless.c \
		yywrap.c)
y_LIB=		usr.lib/liby/libmai.c usr.lib/liby/libzer.c
LIBRARIES=	util termcap m curses wrap dbm ln y

all:	$(addprefix $(O)/,$(PROGS))

# The programs' names, a line each.
list:
	@printf '%s\n' $(PROGS)

# The compiler's programs, a line each: the name, and what cc is given
# to make it, its sources named from $(SRC).  tests/ccom/self.sh has
# the system make them from that.
ccprogs:
	@$(foreach p,$(CCPROGS),echo $(p) \
	    $(subst $(SRC)/,,$($(p)_FLAGS) $($(p)_SRCS));)

.PHONY:	all list ccprogs root syslibs clean FORCE

$(O) $(O)/log $(EXO) $(TCO) $(H) $(G) $(AWKG):
	mkdir -p $@

HDRS:=	$(wildcard $(TOP)/usr/include/*.h $(SRC)/sys/h/*.h $(SRC)/sys/i286/*.h)

# A program's model, the directory of that model's libraries, and its
# sources.  One with NAME_CCOM set is made by the system's own compiler,
# assembler and linker, with the libraries those made, when the others
# are not.
XLIB=	$(TOP)/obj/lib
model=	$(or $($(1)_MODEL),s)
ccom=	$(or $(CCOM),$($(1)_CCOM))
lib=	$(if $(call ccom,$(1)),$(NLIB),$(XLIB))/$(call model,$(1))
lx=	$(if $(call ccom,$(1)),a,lib)
srcs=	$(foreach s,$($(1)_SRCS),$(if $(filter /%,$(s)),$(s),$(SRC)/$(s)))

define prog
$(O)/$(1): $$(call srcs,$(1)) $(HDRS) \
		$(call lib,$(1))/crt0.o $(call lib,$(1))/libc.$(call lx,$(1)) \
		$$(patsubst -l%,$(call lib,$(1))/lib%.$(call lx,$(1)),$$($(1)_LIBS)) | $(O)/log
	@echo cc $(1)
	@$(TOP)/tools/cc $(if $(call ccom,$(1)),-ccom) \
	    -m$(call model,$(1)) $$($(1)_FLAGS) -o $$@ \
	    $$(call srcs,$(1)) $$($(1)_LIBS) \
	    > $(O)/log/$(1) 2>&1 || { cat $(O)/log/$(1); rm -f $$@; exit 1; }
endef
$(foreach p,$(PROGS),$(eval $(call prog,$(p))))

# What the build itself runs, made in obj/host by the C compiler of the
# machine it is run on, HOSTCC: the tree's yacc and lex, so that the
# parsers and scanners in the system's programs are what upstream's yacc
# and lex make of the grammars, whatever the machine has by those names.
# Both are C of before the standard, which the compiler is asked to take
# as it finds it; it is left its own knowledge of the library's
# functions, which is all that a program that calls printf undeclared
# has to go by where such a function is not called as others are, but
# for index, since lex has one of its own.  This yacc has the larger of
# its two sets of tables, its items being twice the size where a pointer
# is twice an int, and reads the tree's yaccpar; this lex keeps a
# pointer or a number in a long, and reads the tree's ncform.  The
# tables they write are the same as those of the system's own
# (tests/system).
HOSTCC=	cc
HOSTCFLAGS= -std=gnu89 -w -fno-strict-aliasing \
	-Wno-error=return-mismatch -Wno-error=int-conversion
YACC=	$(H)/yacc
LEX=	$(H)/lex

$(YACC): $(addprefix $(YSRC)/,y1.c y2.c y3.c y4.c dextern files yaccpar) | $(H)
	@echo hostcc yacc
	@$(HOSTCC) $(HOSTCFLAGS) -DHUGE '-DPARSER="$(YSRC)/yaccpar"' -o $@ \
	    $(filter %.c,$^)

$(LEX): $(addprefix $(LSRC)/,lmain.c sub1.c sub2.c header.c ldefs.c once.c \
		parser.y ncform nrform) $(YACC)
	@echo hostcc lex
	@$(call yacc,$(LSRC)/parser.y,$(H)/lex.y.c)
	@$(HOSTCC) $(HOSTCFLAGS) -fno-builtin-index -Dunix -DINTPTR=long \
	    '-DNCFORM="$(LSRC)/ncform"' '-DNRFORM="$(LSRC)/nrform"' \
	    -I$(LSRC) -o $@ $(addprefix $(LSRC)/,lmain.c sub1.c sub2.c \
	    header.c) $(H)/lex.y.c

# nroff, which formats the manual (below) as the system's would, which
# tests/programs checks.  It is the system's but that it looks for its
# macros in tmac/, and its printers' tables in term/, of the directory
# it is run in, where it is.
NR=	$(SRC)/usr.bin/roff
$(H)/nroff: $(addprefix $(NR)/,$(NROFF)) $(wildcard $(NR)/common_source/*.h) \
		| $(H)
	@echo hostcc nroff
	@$(HOSTCC) $(HOSTCFLAGS) -DNROFF '-D_PATH_TMAC="tmac/"' \
	    '-D_PATH_TERMTAB="term/"' -I$(NR)/common_source -o $@ \
	    $(addprefix $(NR)/,$(NROFF))

# yacc and lex leave their files, under names of their own, in the
# directory they run in: each runs in one made for it.
# $(call yacc,GRAMMAR,C) makes C of GRAMMAR, and with a third argument
# that file of the tokens' numbers as well; $(call lex,SOURCE,C)
# likewise.
yacc=	rm -rf $(2).d && mkdir -p $(2).d && cd $(2).d && \
	$(YACC) $(if $(3),-d) $(1) && mv y.tab.c $(2) && \
	$(if $(3),mv y.tab.h $(3) &&) cd .. && rm -rf $(2).d
lex=	rm -rf $(2).d && mkdir -p $(2).d && cd $(2).d && \
	$(LEX) $(1) && mv lex.yy.c $(2) && cd .. && rm -rf $(2).d

$(G)/expr.c: $(SRC)/bin/expr.y $(YACC) | $(G)
	@echo yacc expr.y
	@$(call yacc,$<,$@)
$(G)/lex.c: $(LSRC)/parser.y $(YACC) | $(G)
	@echo yacc lex/parser.y
	@$(call yacc,$<,$@)

$(G)/egrep.c: $(SRC)/usr.bin/egrep.y $(YACC) | $(G)
	@echo yacc egrep.y
	@$(call yacc,$<,$@)
$(G)/bc.c: $(SRC)/usr.bin/bc.y $(YACC) | $(G)
	@echo yacc bc.y
	@$(call yacc,$<,$@)
$(G)/make.c: $(SRC)/bin/make/gram.y $(YACC) | $(G)
	@echo yacc make/gram.y
	@$(call yacc,$<,$@)
$(G)/ftpcmd.c: $(SRC)/libexec/ftpd/ftpcmd.y $(YACC) | $(G)
	@echo yacc ftpd/ftpcmd.y
	@$(call yacc,$<,$@)

# eqn's grammar, with e.def, the tokens' numbers, which the rest of eqn
# and neqn include.  diction's three scanners, which include its tables of
# words and the functions that look in them, from its directory.
$(G)/eqn $(G)/diction:
	mkdir -p $@
$(G)/eqn/e.c: $(SRC)/usr.bin/eqn/e.y $(YACC) | $(G)/eqn
	@echo yacc eqn/e.y
	@$(call yacc,$<,$@,$(G)/eqn/e.def)
$(G)/eqn/e.def: $(G)/eqn/e.c ;
$(O)/eqn $(O)/neqn: $(G)/eqn/e.def $(SRC)/usr.bin/eqn/e.h
$(G)/diction/wdslex.c: $(DI)/nwords.l $(LEX) | $(G)/diction
	@echo lex diction/nwords.l
	@$(call lex,$<,$@)
$(G)/diction/endlex.c: $(DI)/end.l $(LEX) | $(G)/diction
	@echo lex diction/end.l
	@$(call lex,$<,$@)
$(G)/diction/prtlex.c: $(DI)/part.l $(LEX) | $(G)/diction
	@echo lex diction/part.l
	@$(call lex,$<,$@)
$(O)/style1: $(addprefix $(DI)/,nhash.c dict.c ydict.c abbrev.c names.h)
$(O)/style2: $(addprefix $(DI)/,ehash.c edict.c names.h)
$(O)/style3: $(addprefix $(DI)/,names.h conp.h style.h)
$(O)/tbl: $(SRC)/usr.bin/tbl/t..c
$(O)/spell $(O)/spellin $(O)/spellout: $(SRC)/usr.bin/spell/spell.h

A=	$(SRC)/bin/awk
$(AWKG)/awk.g.c: $(A)/awk.g.y $(YACC) | $(AWKG)
	@echo yacc awk.g.y
	@$(call yacc,$<,$@,$(AWKG)/awk.h)
$(AWKG)/awk.h: $(AWKG)/awk.g.c ;
$(AWKG)/awk.lx.c: $(A)/awk.lx.l $(LEX) | $(AWKG)
	@echo lex awk.lx.l
	@$(call lex,$<,$@)
$(AWKG)/token.c: $(A)/token.c $(AWKG)/awk.h
	@{ sed '/= *{$$/q' $<; \
	  sed -n 's/^# *define *\([^ ]*\) \(.*\)/"\1", \2,/p' $(AWKG)/awk.h; \
	  sed -n '/^};$$/,$$p' $<; } > $@
$(H)/awkproc: $(A)/proc.c $(AWKG)/token.c $(AWKG)/awk.h | $(H)
	@echo hostcc awk/proc
	@$(HOSTCC) $(HOSTCFLAGS) -I$(AWKG) -o $@ $(A)/proc.c $(AWKG)/token.c
$(AWKG)/proctab.c: $(H)/awkproc
	@$(H)/awkproc > $@.new && mv $@.new $@
$(O)/awk: $(A)/awk.def
$(O)/yacc: $(YSRC)/dextern $(YSRC)/files
$(O)/lex: $(LSRC)/ldefs.c $(LSRC)/once.c

# What the games' Makefiles make for them, in obj/games.  atc's grammar
# and scanner, the first with y.tab.h, the tokens' numbers, which the
# second includes.
$(GO)/atc $(GO)/battlestar:
	mkdir -p $@
$(GO)/atc/grammar.c: $(SRC)/games/atc/grammar.y $(YACC) | $(GO)/atc
	@echo yacc atc/grammar.y
	@$(call yacc,$<,$@,$(GO)/atc/y.tab.h)
$(GO)/atc/y.tab.h: $(GO)/atc/grammar.c ;
$(GO)/atc/lex.c: $(SRC)/games/atc/lex.l $(LEX) | $(GO)/atc
	@echo lex atc/lex.l
	@$(call lex,$<,$@)
$(O)/atc: $(GO)/atc/y.tab.h

# battlestar's descriptions of its places and things are in
# /usr/games/lib/battle_strings, which its own mkstr, built for this
# machine, makes of three files as it takes them out: each string that
# is strstr's argument becomes its offset in the file, a long.  The
# files are made again together, the strings added in the order its
# Makefile has make take them.
BS=		$(SRC)/games/battlestar
BSTR=		$(addprefix $(GO)/battlestar/xx,dayfile.c nightfile.c globals.c)
$(H)/bmkstr: $(BS)/mkstr.c | $(H)
	@echo hostcc battlestar/mkstr
	@$(HOSTCC) $(HOSTCFLAGS) -o $@ $<
$(GO)/battlestar/battle_strings: $(addprefix $(BS)/,dayfile.c nightfile.c \
		globals.c) $(H)/bmkstr | $(GO)/battlestar
	@echo mkstr battlestar
	@cd $(BS) && rm -f $@ && for f in globals.c dayfile.c nightfile.c; do \
	    $(H)/bmkstr - $@ $(GO)/battlestar/xx $$f || exit 1; done
$(BSTR): $(GO)/battlestar/battle_strings ;

# hack's sources as they are compiled, and crypt, which is built for this
# machine and reads and writes C, with -Di286 for the system's compiler,
# whose #pragma lines it leaves alone.  hack.onames.h, the names of its
# objects, which its makedefs writes, is in the tree as makedefs writes
# it.
HK=		$(SRC)/games/hack
$(HGO):
	mkdir -p $@
$(H)/hcrypt: $(HK)/crypt.c | $(H)
	@echo hostcc hack/crypt
	@$(HOSTCC) $(HOSTCFLAGS) -Di286 -o $@ $<
$(HGO)/%.c: $(HK)/%.c $(wildcard $(HK)/*.h) $(H)/hcrypt | $(HGO)
	@$(CC) -E -mm -I$(HK) $< > $@.i
	@$(H)/hcrypt < $@.i > $@.new && mv $@.new $@ && rm $@.i

# fortune's text, scene and then obscene after a line "%-", which
# tools/strfile.py makes into the table fortune reads, as strfile -r does.
FT=		$(SRC)/games/fortune
$(GO)/fortunes.dat: $(FT)/scene $(FT)/obscene $(TOP)/tools/strfile.py
	@mkdir -p $(GO)
	@{ cat $(FT)/scene; echo "%-"; cat $(FT)/obscene; } > $(GO)/fortunes
	python3 -I $(TOP)/tools/strfile.py -r $(GO)/fortunes $@

# monop's cards, which tools/initdeck.py packs as initdeck does.
$(GO)/cards.pck: $(SRC)/games/monop/cards.inp $(TOP)/tools/initdeck.py
	@mkdir -p $(GO)
	python3 -I $(TOP)/tools/initdeck.py $< $@

# phantasia's files, which its setfiles makes as it is installed
# (tools/setfiles.py), and its help, which its Makefile formats with tbl
# and then nroff -Ttn300, the table of a printer.  There is no tbl, and
# nroff formats it alone, as the Makefile formats phantasia's page.  The
# host's nroff finds the table in term/, beside it.
PH=		$(GO)/phantasia
$(PH)/characs: $(TOP)/tools/setfiles.py
	python3 -I $(TOP)/tools/setfiles.py $(PH)
$(addprefix $(PH)/,gold motd mess enemy lastdead void): $(PH)/characs ;
$(H)/term/tabtn300.tab: $(TOP)/obj/root/term/tablpr.tab
	@mkdir -p $(H)/term
	cp $< $@
$(PH)/phant.help: $(SRC)/games/phantasia/phant.nr $(H)/nroff \
		$(H)/tmac/tmac.an $(H)/term/tabtn300.tab
	@mkdir -p $(PH)
	cd $(H) && ./nroff -man -Ttn300 < $< > $@

# warp's maps of the stars, each of which its sm, built for this machine,
# makes of a picture of one.
WP=		$(SRC)/games/warp
$(H)/sm: $(WP)/sm.c $(WP)/config.h | $(H)
	@echo hostcc warp/sm
	@$(HOSTCC) $(HOSTCFLAGS) -I$(WP) -o $@ $<
$(GO)/warp/smap.%: $(WP)/smp.% $(H)/sm
	@mkdir -p $(GO)/warp
	$(H)/sm < $< > $@

# cribbage's instructions, formatted by nroff as its Makefile has it.
$(GO)/crib.instr: $(SRC)/games/cribbage/cribbage.n \
		$(SRC)/games/cribbage/macro $(H)/nroff
	@mkdir -p $(GO)
	cd $(SRC)/games/cribbage && $(H)/nroff cribbage.n > $@

# boggle's dictionary, the words of /usr/dict/words it can use, each
# written as how many letters it shares with the one before and then the
# rest, by its comp, built for this machine: it writes text.  The
# Makefile takes /usr/local/lib/w2a instead if there is one; here there
# is none.
BG6=		$(SRC)/games/boggle
$(H)/bcomp: $(BG6)/comp.c | $(H)
	@echo hostcc boggle/comp
	@$(HOSTCC) $(HOSTCFLAGS) -o $@ $<
$(GO)/bogdict: $(TOP)/usr/dict/words $(BG6)/sfile $(H)/bcomp
	@mkdir -p $(GO)
	LC_ALL=C sed -f $(BG6)/sfile $< | uniq | $(H)/bcomp > $@

# spell's lists of words, which its Makefile makes with spellin, built
# here for this machine: hlist of /usr/dict/words; hlista and hlistb of
# that and the words that are American or British, and the local ones;
# and hstop of the words spell stops at before it looks for a stem.
# Each is spell's table of bits as shorts, low byte first, as on the
# PDP-11 too: what the tape has in /usr/dict, byte for byte.
SP=		$(SRC)/usr.bin/spell
SO=		$(TOP)/obj/spell
$(H)/spellin: $(SP)/spellin.c $(SP)/spell.h $(SRC)/sys/h/localopts.h | $(H)
	@echo hostcc spell/spellin
	@mkdir -p $(H)/include/sys
	@cp $(SRC)/sys/h/localopts.h $(H)/include/sys
	@$(HOSTCC) $(HOSTCFLAGS) -Dunix -I$(H)/include -o $@ $<
$(SO)/hlist: $(TOP)/usr/dict/words $(H)/spellin
	@mkdir -p $(SO)
	$(H)/spellin < $< > $@
$(SO)/hlista: $(SP)/american $(SP)/local $(SO)/hlist
	cat $(SP)/american $(SP)/local | $(H)/spellin $(SO)/hlist > $@
$(SO)/hlistb: $(SP)/british $(SP)/local $(SO)/hlist
	cat $(SP)/british $(SP)/local | $(H)/spellin $(SO)/hlist > $@
$(SO)/hstop: $(SP)/stop $(H)/spellin
	@mkdir -p $(SO)
	$(H)/spellin < $< > $@

# The zone files, made by zic from the tables upstream's Makefile
# installs, as the system's own zic makes them (tests/programs): their
# numbers are four bytes, high byte first, on any machine.  This zic is
# built for 32-bit time_t (tools/hostzic.h), whose sums it expects to
# wrap.  zone.proto lists them for root.proto, each name a Link gives
# the zone being another link to its file, as zic makes it.
ZI=		$(SRC)/share/zoneinfo
ZO=		$(TOP)/obj/zoneinfo
ZDATA=		asia australasia europe etcetera northamerica pacificnew \
		systemv
$(H)/zic: $(addprefix $(ZI)/,zic.c scheck.c ialloc.c) \
		$(TOP)/usr/include/tzfile.h $(TOP)/tools/hostzic.h | $(H)
	@echo hostcc zic
	@mkdir -p $(H)/include
	@cp $(TOP)/usr/include/tzfile.h $(H)/include
	@$(HOSTCC) $(HOSTCFLAGS) -fwrapv -include $(TOP)/tools/hostzic.h \
	    -I$(H)/include -o $@ $(filter %.c,$^)
$(ZO)/zone.proto: $(addprefix $(ZI)/,$(ZDATA)) $(H)/zic
	@rm -rf $(ZO) && mkdir -p $(ZO)
	cd $(ZI) && $(H)/zic -d $(ZO) $(ZDATA)
	@cd $(ZI) && awk 'BEGIN { z = "/usr/share/zoneinfo"; \
	    print "# The zone files, made by zic in obj/zoneinfo."; \
	    print "d " z " 755" } \
	    $$1 == "Zone" { if (split($$2, p, "/") > 1 && !(p[1] in d)) { \
		d[p[1]]; print "d " z "/" p[1] " 755" } \
		print "f " z "/" $$2 " 644 " $$2 } \
	    $$1 == "Link" { h = h "h " z "/" $$3 " " z "/" $$2 "\n" } \
	    END { printf "%s", h }' $(ZDATA) > $@

# What root.proto takes of the games' besides their programs.
GAMEDATA=	$(GO)/battlestar/battle_strings $(GO)/fortunes.dat \
		$(GO)/cards.pck $(GO)/crib.instr $(GO)/bogdict \
		$(addprefix $(PH)/,characs phant.help) \
		$(addprefix $(GO)/warp/smap.,0 1 2 3 4 5 6 7)

# ex's sources with its messages taken out, and the file they are in.
$(EXO)/exstrings: $(addprefix $(SRC)/ucb/ex/,$(EX)) \
		$(wildcard $(SRC)/ucb/ex/*.h) $(TOP)/tools/mkstr.py | $(EXO)
	python3 -I $(TOP)/tools/mkstr.py $@ $(EXO) \
	    $(addprefix $(SRC)/ucb/ex/,$(EX))
$(ex_SRCS): $(EXO)/exstrings ;

# tcsh's, likewise, and its headers: sh.err.h has the numbers of its
# errors, from sh.err.c, which has two sets of the flags that go with
# them, the PDP-11's 16-bit ones and others; ed.defns.h the numbers of
# the editor's functions; tc.const.h a declaration of each of its
# constant strings.
T=	$(SRC)/bin/tcsh
$(TCO)/tcsh6.00strgs: $(addprefix $(T)/,$(TCSH)) $(wildcard $(T)/*.h) \
		$(TOP)/tools/mkstr.py | $(TCO)
	python3 -I $(TOP)/tools/mkstr.py -e xprintf=mkprintf -n 5 $@ $(TCO) \
	    $(addprefix $(T)/,$(TCSH))
$(tcsh_SRCS): $(TCO)/tcsh6.00strgs $(TCHDRS) ;
$(TCO)/sh.err.h: $(T)/sh.err.c | $(TCO)
	{ echo '#ifndef _h_sh_err'; echo '#define _h_sh_err'; \
	  sed '/^#if.*pdp11/,/^#else/d' $< | grep '^#define.*ERR_'; \
	  echo '#endif'; } > $@
$(TCO)/ed.defns.h: $(T)/ed.defns.c | $(TCO)
	{ echo '#ifndef _h_ed_defns'; echo '#define _h_ed_defns'; \
	  grep '^#define.*[FV]_' $<; echo '#endif'; } > $@
$(TCO)/tc.const.h: $(T)/tc.const.c | $(TCO)
	grep '^Char STR' $< | \
	    sed 's/Char \([a-zA-Z0-9_]*\).*/extern Char \1[];/' | sort > $@

define library
$(LIB)/$(2)/lib$(1).$(LX): $(addprefix $(SRC)/,$($(1)_LIB)) $(HDRS) \
		| $(LIB)/$(2)/$(1)
	@echo cc lib$(1), $(2)
	@cd $(LIB)/$(2)/$(1) && rm -f *.o && \
	    $(CC) -m$(2) $($(1)_LIBFLAGS) -c \
	    $(addprefix $(SRC)/,$($(1)_LIB)) > log 2>&1 || { cat log; exit 1; }
	@rm -f $$@ && $(if $(CCOM),python3 -I $(TOP)/tools/ar.py $$@ \
	    $(patsubst %.c,$(LIB)/$(2)/$(1)/%.o,$(notdir $($(1)_LIB))),\
	    wlib -q -n -b $$@ \
	    $(patsubst %.c,+$(LIB)/$(2)/$(1)/%.o,$(notdir $($(1)_LIB))))
$(LIB)/$(2)/$(1):
	mkdir -p $$@
endef
$(foreach l,$(LIBRARIES),$(foreach m,s m l,$(eval $(call library,$(l),$(m)))))

# The root file system, obj/root/root.fs, as root.proto lists it.  It
# needs the kernel (../sys/PC) and /boot (../sys/i286stand) made first.
# ROOTDISK is the disk it is to be on, for /etc/fstab: wd0, the first on
# the AT disk controller, unless it is given, as sd0 is for the first
# SCSI disk (make root ROOTDISK=sd0).
#
# The programs go in it without their symbol tables, as upstream's
# install -s leaves them: tools/strip.py makes of each in obj/bin, which
# keeps its own, the copy in obj/root/bin that root.proto names.  The
# kernel loads neither the symbols nor their names, so the copies run the
# same and are an eighth smaller.  make root STRIP=no copies the programs
# as they are, for when the symbols are wanted on the system itself.
#
# ROOTSIZE is its size in blocks of 1K, with room for what a system
# that is used writes there: the tests' disks are of 40 MB
# (tools/mkdisk.py -g 977,5,17), and a machine's own disk is given a
# root as big as it has room for.
R=	$(TOP)/obj/root
ROOTSIZE= 24000
ROOTDISK= wd0
STRIP=	yes
# The kernel that is /unix: the cross compiler's, or with
# KERNEL=$(TOP)/obj/nPC/unix the one the system's own tools made
# (usr/src/sys/PC CCOM=1).  root.proto has it from kernel.proto, which
# is written again only when it changes.
KERNEL=	$(TOP)/obj/PC/unix

# The system has its compiler, and so the headers and the libraries
# that a program is compiled and linked with there.  The libraries are
# of the system's own objects, which the cross compiler's are not:
# they are made, for each model, by the system's own compiler,
# assembler and linker as built for this machine (CCOM=1, above), in
# obj/nlib, whichever compiler the programs are made by.  That is done
# first, by a make of its own, and then the file system is made;
# cc.proto lists them and /usr/include for root.proto
# (tools/ccproto.py).  The library's Makefile is told where to put
# each model's, since an O given to this one is given to every make it
# runs, and that one has an O of its own.
SYSLIBS= $(foreach m,s m l,$(NLIB)/$(m)/crt0.o $(NLIB)/$(m)/libc.a \
	$(foreach l,$(LIBRARIES),$(NLIB)/$(m)/lib$(l).a))

root:	syslibs
	@$(MAKE) --no-print-directory $(R)/strip $(R)/root.fs

syslibs:
	@$(MAKE) --no-print-directory -C $(SRC)/lib/ccom/i286 host
	@$(MAKE) --no-print-directory -C $(SRC)/lib/libc/i286 CCOM=1 \
	    MODEL=s O=$(NLIB)/s
	@$(MAKE) --no-print-directory -C $(SRC)/lib/libc/i286 CCOM=1 \
	    MODEL=m O=$(NLIB)/m
	@$(MAKE) --no-print-directory -C $(SRC)/lib/libc/i286 CCOM=1 \
	    MODEL=l O=$(NLIB)/l
	@$(MAKE) --no-print-directory CCOM=1 $(foreach m,s m l,\
	    $(foreach l,$(LIBRARIES),$(NLIB)/$(m)/lib$(l).a))

# The compiler's passes want the C library of those tools before the
# file system does: if it is not there it is made, and it is made again
# with the rest when the file system is.
$(NLIB)/s/libc.a:
	@$(MAKE) --no-print-directory -C $(SRC)/lib/ccom/i286 host
	@$(MAKE) --no-print-directory -C $(SRC)/lib/libc/i286 CCOM=1 \
	    MODEL=s O=$(NLIB)/s
$(NLIB)/s/crt0.o: $(NLIB)/s/libc.a ;

$(R)/cc.proto: $(SYSLIBS) $(TOP)/tools/ccproto.py \
		$(wildcard $(TOP)/usr/include/*.h $(TOP)/usr/include/*/*.h) | $(R)
	python3 -I $(TOP)/tools/ccproto.py $(TOP) $(LIBRARIES) > $@

$(R) $(R)/bin:
	mkdir -p $@

# learn's lessons, as share/learn's Makefile installs them: a directory
# for each subject in /usr/share/learn, with its Init to be run; the
# two objects that the lessons in C have a program linked with, which
# the system's compiler makes here (tools/ncc); and for each subject a
# file in /var/log/learn that anyone may write.  learn.proto lists them
# for root.proto.
LEARN=	$(SRC)/share/learn
LESSONS= C editor eqn files macros morefiles vi
$(R)/getline.o $(R)/getnum.o: $(R)/%.o: $(LEARN)/C/%.c $(HDRS) | $(R)
	python3 -I $(TOP)/tools/ncc -O -o $@ $<
$(R)/learn.proto: $(wildcard $(LEARN)/*/*) $(R)/getline.o $(R)/getnum.o
	@{ echo "d /usr/share/learn 755"; echo "d /var/log/learn 755"; \
	  for f in Linfo Xinfo; do \
	    echo "f /usr/share/learn/$$f 644 $(LEARN)/$$f"; \
	  done; \
	  for d in $(LESSONS); do \
	    echo "d /usr/share/learn/$$d 755"; \
	    echo "f /var/log/learn/$$d 666 $(R)/empty"; \
	    for f in $(LEARN)/$$d/*; do \
	      case $$f in */Init) m=555;; *) m=644;; esac; \
	      echo "f /usr/share/learn/$$d/$${f##*/} $$m $$f"; \
	    done; \
	  done; \
	  for f in getline getnum; do \
	    echo "f /usr/share/learn/C/$$f.o 644 $(R)/$$f.o"; \
	  done; } > $@

# Whether they are stripped, and where they are from.  When STRIP is
# other than it was, or O is, as when tests/ccom/programs.sh has the
# programs that the system's own compiler made put in, the copies and
# the file system are removed, before make has looked at either, and so
# made again whatever their times: a file made in the same second as one
# it depends on passes for up to date.
$(R)/strip: FORCE | $(R)
	@echo $(STRIP) $(O) > $@.new
	@cmp -s $@.new $@ && rm $@.new || \
	    { rm -rf $(R)/bin $(R)/root.fs && mv $@.new $@; }
$(R)/bin/%: $(O)/% $(TOP)/tools/strip.py | $(R)/bin
	@if [ "$(STRIP)" = no ]; then cp $< $@; \
	    else python3 -I $(TOP)/tools/strip.py $< $@; fi

# /etc/passwd, which is master.passwd without the passwords and the
# fields only the master file has, and the database that programs look
# users up in, passwd.dir and passwd.pag: what mkpasswd -p makes.
$(R)/passwd: $(SRC)/etc/master.passwd $(TOP)/tools/mkpasswd.py | $(R)
	python3 -I $(TOP)/tools/mkpasswd.py $< $(R)
$(R)/passwd.dir $(R)/passwd.pag: $(R)/passwd ;

# fstab for ROOTDISK.  It is made each time and replaced only if it is
# different, so that the file system is made again when ROOTDISK is.
$(R)/fstab: etc/fstab FORCE | $(R)
	sed 's,^/dev/wd0,/dev/$(ROOTDISK),' $< > $@.new
	cmp -s $@.new $@ && rm $@.new || mv $@.new $@
FORCE:

$(R)/syserrlst: $(SRC)/usr.lib/liberrlst/errlst.c $(TOP)/tools/mkerrlst.py | $(R)
	python3 -I $(TOP)/tools/mkerrlst.py $< $@

# The database of /etc/aliases, aliases.dir and aliases.pag, which
# newaliases makes on the system: without it every sendmail waits for
# someone to make it.
$(R)/aliases.dir: $(SRC)/etc/aliases $(TOP)/tools/mkaliases.py \
		$(TOP)/tools/mkpasswd.py | $(R)
	python3 -I $(TOP)/tools/mkaliases.py $< $(R)
$(R)/aliases.pag: $(R)/aliases.dir ;

$(R)/empty: | $(R)
	: > $@

# The time atrun last ran, for atq: never.
$(R)/zero: | $(R)
	echo 0 > $@

$(R)/termcap: $(wildcard $(SRC)/share/termcap/*) $(TOP)/tools/mktermcap.py \
		| $(R)
	python3 -I $(TOP)/tools/mktermcap.py $(SRC)/share/termcap $@

# nroff's macros, as upstream installs them in /usr/share/tmac: most as
# they are, and mdoc's, which is doc and the files it reads, and -me's,
# which go in /usr/share/me but for tmac.e, without their comments, by
# strip.sed (the -me Makefile has the same sed in its own lines).
TM=	$(SRC)/share/tmac
MDOC=	doc doc-ditroff doc-common doc-nroff doc-syms
MEFILES= tmac.e acm.me chars.me deltext.me eqn.me float.me footnote.me \
	index.me local.me null.me refer.me sh.me tbl.me thesis.me

$(R)/tmac $(R)/me $(R)/term $(H)/tmac:
	mkdir -p $@
$(addprefix $(R)/tmac/tmac.,$(MDOC)): $(R)/tmac/tmac.%: $(TM)/% \
		$(TM)/strip.sed | $(R)/tmac
	LC_ALL=C sed -f $(TM)/strip.sed $< > $@
$(addprefix $(R)/me/,$(MEFILES)): $(R)/me/%: $(SRC)/share/me/% \
		$(TM)/strip.sed | $(R)/me
	LC_ALL=C sed -f $(TM)/strip.sed $< > $@

# The tables of the printers nroff drives besides a terminal, for -T:
# each is a table in C, which mktab, built with it for this machine,
# writes out as the text nroff reads.
NT=	$(SRC)/usr.bin/roff/nroff_term
TERMS=	37 lpr 300 300-12 302 302-12 382 382-12 450 450-12 833 833-12 epson \
	itoh itoh12 nec nec12 nec-t nec25-t qume qume12 xerox xerox12 x-ecs \
	x-ecs12
$(R)/term/tab%.tab: $(NT)/tab%.c $(NT)/mktab.c $(NR)/common_source/tw.h \
		| $(R)/term $(H)
	@echo mktab tab$*
	@$(HOSTCC) $(HOSTCFLAGS) -I$(NR)/common_source -o $(H)/mktab$* \
	    $(NT)/mktab.c $<
	@$(H)/mktab$* > $@

# The manual: the pages man.list names, formatted by the host's nroff
# with the macros the system has, but that the copies of those it reads
# look for each other in tmac/, beside it, where the system's say
# /usr/share/tmac/; and the links among them and /usr/man/whatis.  What
# tools/mkman.py makes is in obj/man, with man.proto, which root.proto
# includes.
MANO=	$(TOP)/obj/man
MANLIST:= $(shell awk '$$1 ~ /^cat/ { for (i = 2; i <= NF; i++) \
	{ sub(/:.*/, "", $$i); print $$i } }' man.list)
MANSRCS:= $(addprefix $(SRC)/,$(MANLIST)) $(wildcard $(SRC)/man/man*/* \
	$(addprefix $(SRC)/,$(addsuffix Makefile,$(dir $(MANLIST)))))
HTMAC=	$(addprefix $(H)/tmac/tmac.,an an.new an6n an6t $(MDOC))
$(filter-out $(addprefix %.,$(MDOC)),$(HTMAC)): $(H)/tmac/%: $(TM)/% \
		| $(H)/tmac
	LC_ALL=C sed 's,/usr/share/tmac/,tmac/,' $< > $@
$(filter $(addprefix %.,$(MDOC)),$(HTMAC)): $(H)/tmac/%: $(R)/tmac/% \
		| $(H)/tmac
	LC_ALL=C sed 's,/usr/share/tmac/,tmac/,' $< > $@
$(MANO)/man.proto: man.list $(MANSRCS) $(SRC)/man/makewhatis.sed $(HDRS) \
		$(H)/nroff $(HTMAC) $(TOP)/tools/mkman.py
	@python3 -I $(TOP)/tools/mkman.py $(H)/nroff $(SRC) man.list $(MANO)

$(R)/root.fs: root.proto $(wildcard etc/* root/* skel/*) adventure/Makefile \
		$(addprefix $(R)/bin/,$(PROGS)) \
		$(R)/passwd $(R)/passwd.dir $(R)/passwd.pag $(R)/fstab \
		$(R)/syserrlst $(R)/aliases.dir $(R)/aliases.pag $(R)/empty \
		$(R)/zero $(R)/termcap $(addprefix $(R)/tmac/tmac.,$(MDOC)) \
		$(addprefix $(R)/me/,$(MEFILES)) \
		$(patsubst %,$(R)/term/tab%.tab,$(TERMS)) $(MANO)/man.proto \
		$(GAMEDATA) $(addprefix $(SO)/,hlista hlistb hstop) \
		$(ZO)/zone.proto $(R)/cc.proto $(SRC)/usr.bin/lorder.sh \
		$(R)/learn.proto $(SRC)/usr.bin/calendar/calendar.sh \
		$(SRC)/usr.bin/lint.sh $(SRC)/usr.bin/mkdep.sh \
		$(R)/kernel.proto $(KERNEL) \
		$(TOP)/obj/stand/boot $(TOP)/obj/stand/hdboot.bin
	python3 -I $(TOP)/tools/mkfs.py -s $(ROOTSIZE) root.proto $@

$(R)/kernel.proto: FORCE
	@mkdir -p $(R)
	@echo "f /unix 644 $(KERNEL)" > $@.new
	@if cmp -s $@.new $@; then rm $@.new; else mv $@.new $@; fi

clean:
	rm -rf $(O) $(EXO) $(TCO) $(H) $(G) $(R) $(MANO) $(GO) $(SO) $(ZO) \
	    $(foreach l,$(LIBRARIES),$(LIB)/[sml]/$(l) $(LIB)/[sml]/lib$(l).$(LX))
