\.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)3.t	3.0 (2.11BSD) 2026/11/08
.\"
.ds lq ``
.ds rq ''
.ds LH "Installing/Operating \*(2B
.ds RH "Upgrading a PDP-11 UNIX System
.ds CF \*(DY
.LP
.nr H1 3
.nr H2 0
.bp
.LG
.B
.ce
3. UPGRADING AN EXISTING SYSTEM
.sp 2
.R
.NL
.PP
If you are upgrading a recent patch level of 2.11BSD, skip this section in
its entirety and proceed to Appendix C.
.PP
The general advice is "you don't want to do this". Depending on what patch
level you are on if you are already running 2.11BSD, substantial work can 
be required to get from that patch level to the current (as of this writing) 
Patch Level 500. If you are running 2.10BSD or some other Unix version on your 
existing system, things get even more complicated.
.PP
If you are going to attempt an in-place upgrade, it is \fBstrongly\fR suggested
that you back up your existing system, both in its preferred backup format 
(such as \fIdump\fR/\fIrestore\fR) in case you need to revert to that system, 
as well as in a portable format like \fItar\fR. 
.PP
The following sections describe the procedure for an upgrade from a 
substantially older 2.11BSD or other Unix operating system to Patch Level 500 
of 2.11BSD. These procedures have not been tested in at least a decade and 
success will depend on your familiarity with both the existing system and this 
35th Anniversary Release of 2.11BSD. Here there be dragons!
.NH 2
Files to save
.PP
The following list enumerates the standard set of files you will want to
save and suggests directories in which site-specific files should be
present.  Note that because \*(Ps changed so radically from previous
versions of UNIX on the PDP-11, many of these files may not exist on your
system, and will almost certainly require extensive changes for \*(2B,
but it's still handy to have them around as you're configuring \*(2B.
This list will likely be augmented with non-standard files you have added
to your system.
.PP
You should create a \fItar\fP image of
(at a minimum) the following files before the new filesystems are created.
In addition, you should do a full dump before rebuilding the filesystem
to guard against missing something the first time around.  The \*(2B
\fIrestor\fP\|(8) program can read and convert old \fIdump\fP\|(8) tapes.
.DS
.TS
l c l.
/.cshrc	\(dg	root csh startup script
/.login	\(dg	root csh login script
/.profile	\(dg	root sh startup script
/.rhosts	\(dg	for trusted machines and users
/dev/MAKEDEV	\(dd	in case you added anything here
/dev/MAKEDEV.local	*	for making local devices
/etc/disktab	*	in case you changed disk partition sizes
/etc/dtab	\(dd	table of devices to attach at boot time
/etc/fstab	\(dg	disk configuration data
/etc/ftpusers	\(dg	for local additions
/etc/gateways	\(dg	routing daemon database
/etc/gettytab	\(dg	getty database
/etc/group	\(dg	group data base
/etc/hosts	\(dg	for local host information
/etc/hosts.dir	*	must be rebuilt with mkhosts
/etc/hosts.pag	*	must be rebuilt with mkhosts
/etc/hosts.equiv	\(dg	for local host equivalence information
/etc/networks	\(dg	for local network information
/etc/netstart	*	site dependent network startup script
/etc/passwd	*	must be converted to shadow password file format
/etc/passwd.dir	*	must be rebuilt with mkpasswd
/etc/passwd.pag	*	must be rebuilt with mkpasswd
/etc/printcap	\(dg	line printer database
/etc/protocols	\(dd	in case you added any local protocols
.TE
.DE
.\" This is to force a page break in the middle of a table
.bp
.DS
.TS
l c l.
/etc/rc	\(sc	for any local additions
/etc/rc.local	\(sc	site specific system startup commands
/etc/remote	\(dg	auto-dialer configuration
/etc/services	\(dd	for local additions
/etc/syslog.conf	\(dg	system logger configuration
/etc/securettys	\(sc	for restricted list of ttys where root can log in
/etc/ttys	\(dg	terminal line configuration data
/etc/ttytype	\(sc	terminal line to terminal type mapping data
/etc/termcap	\(dd	for any local entries that may have been added
/lib	\(dd	for any locally developed language processors
/usr/dict/*	\(dd	for local additions to words and papers
/usr/hosts/MAKEHOSTS	\(dg	for local changes
/usr/include/*	\(dd	for local additions
/etc/aliases	\(dg	mail forwarding data base
/etc/crontab	\(dg	cron daemon data base
/usr/share/font/*	\(dd	for locally developed font libraries
/usr/lib/lib*.a	\(dg	for local libraries
/usr/share/lint/*	\(dd	for locally developed lint libraries
/etc/sendmail.cf	\(dg	sendmail configuration
/usr/share/tabset/*	\(dd	for locally developed tab setting files
/usr/share/term/*	\(dd	for locally developed nroff drive tables
/usr/share/tmac/*	\(dd	for locally developed troff/nroff macros
/etc/uucp/*	\(dg	for local uucp configuration files
/usr/man/manl	\(sc	for manual pages for locally developed programs
/usr/msgs	\(dg	for current msgs
/usr/spool/*	\(dg	for current mail, news, uucp files, etc.
/usr/src/local	\(dg	for source for locally developed programs
/sys/conf/HOST	\(dg	configuration file for your machine
/sys/conf/files.HOST	\(dg	list of special files in your kernel
/*/quotas	\(sc	filesystem quota files
.TE
.sp
\(dg\|Files that can be used from \*(Ps without change.
\(dd\|Files that need local modifications merged into \*(2B files.
\(sc\|Files that require special work to merge and are discussed below.
.TE
.DE
.NH 3
Installing \*(2B
.PP
The next step is to build a working \*(2B system.
This can be done by following the steps in Chapter 2 of
this document for extracting the root and /usr filesystems
from the distribution tape onto unused disk partitions.
.PP
Once you have extracted the \*(2B system and booted from it,
you will have to build a kernel customized for your configuration.
If you have any local device drivers,
they will have to be incorporated into the new kernel.
See Appendix A for instructions on configuring, building, and installing a new
kernel.
.PP
With the introduction of disk labels the disk partitions in \*(2B the 
/etc/disktab file has changed dramatically.  There is a detailed description
later in this chapter about the changes.  If you have modified the 
partition tables in previous versions of \*(2B you will need to create
a new disktab entry or modify an existing one.
.NH 2
Merging your files from earlier PDP-11 UNIX systems into \*(2B
.PP
When your system is booting reliably and you have the \*(2B
root and /usr filesystems fully installed you will be ready
to continue with the next step in the conversion process,
merging your old files into the new system.
.PP
If you saved the files on a \fItar\fP tape, extract them
into a scratch directory, say /usr/convert:
.DS
\fB#\fP mkdir /usr/convert
\fB#\fP cd /usr/convert
\fB#\fP tar x
.DE
.PP
For sites running a pre-\*(Ps UNIX, there is very little that can be
said here as the variety of previous versions of PDP-11 UNIX systems and how
they were administered is large.  As an example, most previous versions
of PDP-11 UNIX systems used the files \fI/etc/ttys\fP and \fI/etc/ttytype\fP
to administer which terminals should have login processes attached to
them and what the types of terminals those were.  Under \*(2B
/etc/ttytype has disappeared entirely, its functions subsumed by
/etc/ttys along with several new functions.  In general, you will simply
have to use your previous configuration files as references as you
configure \*(2B to your site needs.  Familiarity with 4.3BSD
configuration is very helpful at this point since
\*(2B is nearly identical in most of the files listed in the previous
section.
.PP
If you have any home-grown device drivers
that use major device numbers reserved by the system you
will have to modify the commands used to create the devices or alter
the system device configuration tables in /sys/pdp/conf.c.
Note that almost all \*(2B major device numbers are different from
those in previous PDP-11 UNIX systems except \*(1B.  A couple more device
numbers were added since the release of \*(1B for the kernel logging
facility (/dev/klog) and a (new) TK50/TU81 driver.
.PP
System security changes require adding several new ``well-known'' groups 
to /etc/group.
The groups that are needed by the system as distributed are:
.DS
.TS
l c.
name	number
_
wheel	0
daemon	1
kmem	2
sys	3
tty	4
operator	5
staff	10
bin	20
.TE
.DE
.PP
Only users in the ``wheel'' group are permitted to \fIsu\fP to ``root''.
Most programs that manage directories in /usr/spool
now run set-group-id to ``daemon'' so that users cannot
directly access the files in the spool directories.
The special files that access kernel memory, \fI/dev/kmem\fP
and \fI/dev/mem\fP, are made readable only by group ``kmem''.
Standard system programs that require this access are
made set-group-id to that group.
The group ``sys'' is intended to control access to system sources,
and other sources belong to group ``staff.''
Rather than make user's terminals writable by all users,
they are now placed in group ``tty'' and made only group writable.
Programs that should legitimately have access to write on user's terminals
such as \fItalk\fP and \fIwrite\fP now run set-group-id to ``tty''.
The ``operator'' group controls access to disks.
By default, disks are readable by group ``operator'',
so that programs such as \fIdf\fP can access the filesystem
information without being set-user-id to ``root''.
.PP
Several new users have also been added to the group of ``well-known'' users 
in /etc/passwd.
The current list is:
.DS
.TS
l c.
name	number
_
root	0
daemon	1
operator	2
uucp	66
nobody	32767
.TE
.DE
.PP
The ``daemon'' user is used for daemon processes that
do not need root privileges.
The ``operator'' user-id is used as an account for dumpers
so that they can log in without having the root password.
By placing them in the ``operator'' group, 
they can get read access to the disks.
The ``uucp'' login has existed long before \*(2B,
and is noted here just to provide a common user-id.
The password entry ``nobody'' has been added to specify
the user with least privilege.
.PP
After restoring your old password file from tape/backups, a conversion is
required to create the shadow password file.  Only the steps
to convert /etc/passwd are given here, see the various man pages
for \fIchpass\fP\|(1), \fIvipw\fP\|(8), \fImkpasswd\fP\|(8), etc.
.DS
\fB#\fP awk -f /etc/awk.script < /etc/passwd >/etc/junk
\fB#\fP mkpasswd -p /etc/junk
\fB#\fP mv /etc/junk.orig /etc/passwd
\fB#\fP mv /etc/junk.pag /etc/passwd.pag
\fB#\fP mv /etc/junk.dir /etc/passwd.dir
\fB#\fP mv /etc/junk /etc/master.passwd
\fB#\fP chown root /etc/passwd* /etc/master.passwd
\fB#\fP chmod 0600 /etc/master.passwd
.DE
.PP
The format of the cron table, /etc/crontab, is the same as that
of \*(1B.
.PP
Some of the commands previously in /etc/rc.local have been 
moved to /etc/rc;
several new functions are now handled by /etc/rc.local.
You should look closely at the prototype version of /etc/rc.local
and read the manual pages for the commands contained in it
before trying to merge your local copy.
Note in particular that \fIifconfig\fP has had many changes,
and that host names are now fully specified as domain-style names
(e.g, boris.Oswego.EDU).
.PP
The C library and system binaries are compiled with versions of
\fIgethostbyname\fP and \fIgethostbyaddr\fP which use the name server resolver 
routines and NOT host tables. 
If you want to build the C library and binaries to use host tables 
you must edit the file /usr/src/lib/libc/Makefile and change the macro 
HOSTLOOKUP=named to be HOSTLOOKUP=hosttable.  Then recompile the entire 
system (see /usr/src/Makefile for instructions).
.PP
There is a very old version of \fInamed\fP\|(8) in the system source tree.  
Don't even think about using it.
.PP
The format of /etc/ttys is the same as it was under \*(Ps.
It includes the terminal type and security options that were previously
in /etc/ttytype and /etc/securettys.
.PP
\fIsyslog\fP is the 4.4BSD-Lite version now.
See \fIsyslog\fP\|(3) and \fIsyslogd\fP\|(8) for details.
They are used by many of the system daemons
to monitor system problems more closely, for example
network routing changes.
.PP
Again, it must be emphasized that the nameserver is not robust under
\*(2B, and if the \fIhosts\fP files are not desired then the best
alternative is to use the \fIresolver\fP\|(5) routines and use
the nameserver on a remote larger machine.  The \fIresolver\fP\|(5)
routines are known to work.
.PP
The spooling directories saved on tape may be restored in their
eventual resting places without too much concern.  Be sure to
use the ``p'' option to \fItar\fP so that files are recreated with the
same file modes:
.DS
\fB#\fP cd /usr
\fB#\fP tar xp msgs spool/mail spool/uucp spool/uucppublic spool/news
.DE
.PP
The ownership and modes of two of these directories
needs to be changed, because
\fIat\fP now runs set-user-id ``daemon'' instead of root.
Also, the uucp directory no longer needs to be publicly writable,
as \fItip\fP reverts to privileged status to remove its lock files.
After copying your version of /usr/spool, you should do the following:
.DS
\fB#\fP chown \-R daemon /usr/spool/at
\fB#\fP chown \-R root /usr/spool/uucp
\fB#\fP chgrp \-R daemon /usr/spool/uucp
\fB#\fP chmod \-R o\-w /usr/spool/uucp
.DE
.PP
Whatever else is left is likely to be site specific or require
careful scrutiny before placing in its eventual resting place.
Refer to the documentation and source code 
before arbitrarily overwriting a file.
.NH 2
Additional conversion information
.PP
After setting up the new \*(2B filesystems, you may restore the user
files that were saved on tape before beginning the conversion.  Note that
the \*(2B \fIrestor\fP program does its work by accessing the raw file
system device and depositing inodes in the appropriate locations on
disk.  This means that filesystem dumps might not restore correctly if
the characteristics of the filesystem have changed (e.g. if you're
restoring a dump of a filesystem into a filesystem smaller than the
original.)  To restore a dump tape for, say, the /u filesystem something
like the following would be used:
.DS
\fB#\fP restor r /dev/rxp1e
.DE
.PP
If \fItar\fP images were written instead of doing a dump, you should
be sure to use the `p' option when reading the files back.
No matter how you restore a filesystem, be sure and check its
integrity with \fIfsck\fP when the job is complete.
.PP
\fItar\fP tapes are preferred (when possible) because the inode
allocation is performed by the kernel rather than the \fIrestor\fP\|(8)
program.  This has the benefit of allocating inodes sequentially starting
from the beginning of the inode portion of the filesystem rather than
preserving the fragmented/randomized order of the old filesystem.
.NH 2
Hints on converting from previous PDP-11 UNIX systems to \*(2B
.PP
This section summarizes some of the significant changes in \*(2B
from \*(1B.  The installation guide for \*(1B is included in the
distribution as /usr/doc/2.10/setup.2.10 and should be read if
you are not presently running \*(Ps or \*(1B.
It does not include changes in the network;
see Chapter 5 for information on setting up the network.
.PP
Old core files will not be intelligible by the current debuggers
because of numerous changes to the user structure.
Also removed from the user structure are the members u_offset, u_count,
u_base, u_segflg, the 4.3BSD uio/iovec/rdwri kernel I/O model having
been put in place.  The 4.3BSD \fInamei\fP argument encapsulation 
technique has been ported, which adds the u_nd member to the user
structure.
.PP
Note, once your system is installed and running, you
should make sure that you recompile and reinstall the directory
\fI/usr/src/share/zoneinfo\fP.  Read through the Makefile first, if you're
not located on the West Coast you will have to change it.  This directory
is an addition since 4.3BSD, and is intended to solve the Daylight
Savings Time problems once and for all.
.PP
The in-memory inode structure has had the i_id member added as part
of implementing the 4.3BSD namei cache.  The di_addr member of the
on disk inode structure is now an array of type \fBdaddr_t\fP instead 
of \fBchar\fP.  The old 3-byte packed block number is obsolete at last.
.PP
The on-disk directory structure is that of 4.3BSD with the difference
that the inode number is an unsigned short instead of a long.  This was
done to reduce the amount of long arithmetic in the kernel and to maintain
compatibility with earlier versions with regard to the maximum number of
inodes per filesystem.  Given the typical size of disks used with \*(2B
the limit on the number of inodes per filesystem will not be a problem.
.PP
And again, \*(2B is \fBnot\fR filesystem compatible with any previous PDP-11 
UNIX system\fP.
.PP
If you want to use \fIps\fP after booting a new kernel,
and before going multiuser, you must initialize its name list
database by running \fIps \-U\fP.
.NH 2
Hints on possible problems upgrading from \*(1B
.NH 3
New utmp UT_NAMESIZE.
.PP
.B UT_NAMESIZE
in <utmp.h>
was changed from 8 to 15.  This won't affect correctly written programs
(those which do not hard code the constant 8) at the source
level but does cause changes in various databases.  This means that old
binaries won't be able to cope with new databases (passwd, aliases, etc)
and vice versa.
.PP
This change was necessary since the systems available for \*(2B development
had to be shared with systems in which UT_NAMESIZE was set at 15.  If this
change/incompatibility is not desired, then utmp.h and wtmp.h will have to
be modified and the system libraries and applications rebuilt before 
proceeding to load local software.
.PP
The simplest way to deal with this incompatibility is simply to rebuild
all your databases from the source data.  In particular, you should be sure
you rebuild
.IR /etc/passwd ,
.IR /etc/hosts ,
and
.I /etc/aliases
databases via the commands:
.IR "mkpasswd /etc/passwd" ,
.IR "mkhosts /etc/hosts" ,
and
.IR /usr/ucb/newaliases.
.NH 3
man system
.PP
The manual system continues to track the changes
going on in 4BSD.  I'm not convinced the new setup is better, but it does
seem to be the method of the moment.
The setup is essentially the same as that in the
.B 4.3BSD-TAHOE
distribution with the manual source in /usr/src/man.
.NH 3
NMOUNT lowered
.PP
The value of
.B NMOUNT
in
.I /sys/h/param.h
is set to 6 in the distribution system.  This may be too small for systems
using multiple disk drives.  Since each mount table entry costs about
440 bytes of valuable kernel data space this number should be chosen
with care.  See Appendix A for an explanation of how to
reconfigure
.B NMOUNT.
.NH 3
Shadow passwords
.PP
The May 1989 release of the 4.3BSD shadow password file has been ported
to \*(2B.  Password aging is also implemented.
.NH 3
New /etc/rc startup scripts
.PP
\fI/etc/rc\fR, \fI/etc/rc.local\fR and \fI/etc/netstart\fR have been modified 
extensively and \fI/etc/rc.conf\fR has been added.  \fIrc.conf\fR is a file 
which centralizes information previously scattered across several files.
.NH 3
mkfs
.PP
\fImkfs\fP\|(8) is no longer able to populate a filesystem with files.  The
4.3BSD version of \fImkfs\fP\|(8) was ported to \*(2B.
.NH 3
mkproto and mklost+found
.PP
\fImkproto\fP\|(8) and \fImklost+found\fP\|(8) have been removed from the
system. \fImkproto\fP\{(8) hasn't been used since it was written and has so many
limitations as to render the program useless.  \fImklost+found\fP\|(8) is not 
needed as \fIfsck\fP\|(8) has been capable of automatically extending 
lost+found for quite some time.
.NH 3
ANSI C compiler
.PP
The C compiler (and C preprocessor) is ANSI compliant and most (but not all)
C library functions are properly prototyped. This may cause compilation 
problems with some program sources.
.NH 3
/etc/disktab
.PP
The format of /etc/disktab
is now the same as 4.3BSD-Reno and 4.4BSD.  Previously to describe a drive
(an RM03 for example) the /etc/disktab
file had entries of the form:
.DS
:ty=removable:ns#32:nt#5:nc#823:sf:
:b0=/mdec/rm03uboot:
:pa#9600:ba#1024:fa#1024:
:pb#9600:bb#1024:fb#1024:
:pc#131520:bc#1024:fc#1024:
:pf#121920:bf#1024:ff#1024:
:pg#112320:bg#1024:fg#1024:
:ph#131520:bh#1024:fh#1024:
.DE
.PP
Note that there is no information at all about which cylinder a partition
starts at or which partitions overlap and may not be used simultaneously.
That information was kept in tables in the driver.  If you modified 
/etc/disktab it would have no effect without also changing the driver and
recompiling the kernel.
.PP
The new /etc/disktab file looks like this:
.DS
# RM02/03
#	
rm02|RM02|rm03|RM03|DEC RM02/03:\\
    :ty=removable:ns#32:nt#5:nc#823:sf:\\
    :b0=/mdec/rm03uboot:\\
    :pa#14080:oa#0:ba#1024:fa#1024:ta=2.11BSD:\\
    :pb#8960:ob#14080:bb#1024:fb#1024:tb=swap:\\
    :pc#131680:oc#0:bc#1024:fc#1024:\\
    :pg#108480:og#23040:bg#1024:fg#1024:tg=2.11BSD:\\
    :ph#131520:oh#0:bh#1024:fh#1024
.DE
.PP
There are two new fields per partition, the 'o' (oa, ob, etc.) field
specifies the offset in sectors that the partition begins at.  The 't'
field specifies the partition type.  Only those partitions which are
\fB2.11BSD\fP will be recognized by \fInewfs\fP\|(8) and the kernel as 
filesystems.
The kernel also will not swap or place a crash dump on a partition that
is not of type \fBswap\fP.
.PP
The two examples above are equivalent and provide an example of a 
translating an old style disktab entry into a new style entry.  To translate
a customized disktab entries you will need:  1) a copy of your
current partition tables from the device driver, 2) a copy of the 
old disktab entry, 3) your current /etc/fstab file.  In new disktab entries
you should only place those partitions you actually use.  There is no
need to declare (as was done in the examples above) all of the possible
partitions.
.PP
If you
have changed the disk partition sizes, be sure to make the necessary
/etc/disktab changes and label your disks
BEFORE trying to access any of
your old filesystems!
There are two ways to label your disks.  The standalone disklabel program 
is one way.  It is also possible to label disks using \fIdisklabel\fP\|(8) 
with the \-r option \- this works even when running on a kernel which does
not support labels (\-r reads and writes the raw disk, thus it is possible
to label disks on an older kernel as long as the \fIdisklabel\fP\|(8) 
program is present).
