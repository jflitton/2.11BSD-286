.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)6.t	3.0 (2.11BSD) 2026/11/09
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 6
.nr H2 0
.ds RH "System Operation
.ds CF \*(DY
.bp
.LG
.B
.ce
6. SYSTEM OPERATION
.sp 2
.R
.NL
.PP
This chapter describes procedures used to operate a system running 2.11BSD.
Procedures described here are used periodically, to reboot the system,
analyze error messages from devices, do disk backups, monitor
system performance, recompile system software and control local changes.
.NH 2
Bootstrap and shutdown procedures
.PP
In a normal reboot, the system checks the disks and comes up multi-user
without intervention at the console.
Such a reboot
can be stopped (after it prints the date) with a ^C (interrupt).
This will leave the system in single-user mode, with only the console
terminal active.
It is also possible to allow the filesystem checks to complete
and then to return to single-user mode by signaling \fIfsck\fP
with a QUIT signal (^\\).
.PP
When cold booting via the system console is needed, you will need to determine 
the correct command(s) or other procedures appropriate to your CPU model or 
emulator type. This information (along with a list of disk type to device name 
mappings) is generally found in the documentation for your CPU model or 
emulator.
.PP
Once you have succeeded in cold booting your system to the bootstrap prompt 
(:), pressing the carriage return key will cause the default kernel image 
(either as distributed or as compiled as described in Appendix A) to be booted.
.PP
Depending on the configuration options as described in Section 7.2, 
"Configuring automatic boot and restart" the system will either come up in 
single-user mode or automatically attempt to enter multi-user mode.
.PP
To bring the system up to multi-user operation from single-user mode, 
all you have to do is press Control-D (^D) on the console terminal. The system
will then execute /etc/rc,
a multi-user restart script (and /etc/rc.local),
and come up on the terminals listed as
active in the file /etc/ttys.
See
\fIinit\fP\|(8)
and
\fIttys\fP\|(5).
.PP
\fBIMPORTANT:\fR If you have the system configured to come up in single-user 
mode and have any reason to suspect the system was not cleanly shut down (power
failure, system crash, etc.), you should run ``fsck'' to check the integrity of
your filesystem(s). A system configured to boot directly to multi-user mode 
will try to detect an unclean shutdown and run ``fsck'' for you. This may 
involve one or more automatic reboots. It is possible for ``fsck'' to "give up"
and request manual intervention if it cannot automatically repair any detected 
filesystem inconsistencies.
.PP
To take the system down to a single user state you can use
.DS
\fB#\fP kill 1
.DE
.PP
or use the
\fIshutdown\fP\|(8)
command (which is much more polite, if there are other users logged in.)
when you are up multi-user.
Either command will kill all processes and give you a shell on the console,
as if you had just booted.  Filesystems remain mounted after the
system is taken single-user.  If you wish to come up multi-user again, you
should do this by:
.DS
\fB#\fP cd /
\fB#\fP umount -a
\fB#\fP ^D
.DE
.PP
Each system shutdown, crash, processor halt and reboot
is recorded in the file /usr/adm/shutdownlog
with the cause.
.NH 2
Device errors and diagnostics
.PP
When serious errors occur on peripherals or in the system, the system
prints a warning diagnostic on the console.
These messages are written to the kernel logger where they are retrieved
by \fIsyslogd\fP\|(8) via \fI/dev/klog\fP - \fIdmesg\fP\|(8) is now
obsolete.  \fIdmesg\fP\|(8) is present in the distribution but no longer
used.  The message buffer is now 4kb in size and external to the kernel.
.PP
Error messages printed by the devices in the system are described with the
drivers for the devices in Section 4 of the Programmer's Manual.
Some drivers have been modified to use the kernel logger; others still
simply do printf statements.
If errors occur suggesting hardware problems, you should contact
your hardware support group or field service.  It is a good idea to
examine the error log files regularly
(e.g. with ``tail \-r \fI/usr/adm/messages\fP'').
.NH 2
Filesystem checks, backups and disaster recovery
.PP
Periodically (say every week or so in the absence of any problems)
and always (usually automatically) after a crash,
all the filesystems should be checked for consistency by
\fIfsck\fP\|(8).
The procedures of
\fIreboot\fP\|(8)
should be used to get the system to a state where a filesystem
check can be performed manually or automatically.
.PP
Dumping of the filesystems should be done on a regular schedule,
since once the system is going it is easy to
become complacent.
Complete and incremental dumps are easily done with
\fIdump\fP\|(8).
.PP
Dumping of files by name is best done by
\fItar\fP\|(1)
but the amount of data that can be moved in this way is limited
to a single tape. Finally, if there are enough drives entire
disks can be copied with
\fIdd\fP\|(1)
using the raw special files and an appropriate
blocking factor; the number of sectors per track is usually
a good value to use, consult \fI/etc/disktab\fP.
.PP
A full backup of the system\(dg in ``bootable'' format,
.FS
.IP \(dg
This excludes any filesystems and directory trees not on a normal distribution
tape. If you have configured any additional filesystems or created new 
directories under / or /usr, you should back those up as well, since they will 
not be on the bootable recovery tape.
.FE
including the standalone programs mentioned in Chapter 2, "Installation 
procedure" (boot, disklabel, mkfs, restor and icheck) plus system 
binaries and sources is made by changing directories to /sys/pdpstand, 
editing the Makefile line with "DISK=   xp0a" and changing the "xp0a" part 
to your root partition name\(dd, making sure a writable physical or emulated 
tape is attached, and then doing:
.DS
.TS
lw(1.5i) l.
\fB#\fP make dump	(creates a dump of the root filesystem)
\fB#\fP make tmscptape	(creates the recovery tape\(dd)
\fB#\fP make clean	(optional, removes the standalone binaries just built)
\fB#\fP rm /usr/root.dump	(optional, removes the root filesystem dump)
.TE
.DE
.FS
.IP \(dd
This procedure uses the same utility that created the original installation 
tape. Some harmless warning messages will be displayed during the procedure. A 
future Patch Level will likely remove the need for editing the Makefile, 
eliminate the warning messages, and so on.
.FE
.PP
If the root and/or /usr filesystems are damaged by a hardware or software 
failure you can rebuild a workable disk by doing a restore in the same way 
that the initial installation was performed, as described in Chapter 2, 
"Installation procedure". The advantage of having a recovery tape (and updating 
it as needed) is that an install from your recovery tape will contain your 
customizations (patches, custom kernel, added users, etc.)
.PP
Exhaustion of user filesystem space is certain to occur on an active system 
with multiple users every now and then and can be managed with a combination 
of disk quotas (the 4.3BSD disk quota system is available as a kernel 
configuration option), threatening messages of the day, and personal letters. 
.PP
If you are running 2.11BSD under an emulator, another possibility is to create 
a new, large disk drive and attach it. You can then move some or all users to 
the new drive using the \fItar(8)\fR command as described in the next section. 
In this case, \fItar(8)\fR is preferred over \fIdump(8)\fR as you can select 
particular directories or files, while \fIdump(8)\fR is limited to entire 
filesystems.
.NH 2
Moving filesystem data
.PP
If you have a tape drive (real or emulated), one way to move a filesystem is 
to dump it to tape using
\fIdump\fP\|(8),
use
\fInewfs\fP\|(8)
to create the new filesystem, and restore the tape using \fIrestor\fP\|(8).
If you don't want to or can't use tape, \fIdump\fP accepts an argument telling 
where to put the dump.  The argument may be a filename or the special 
name ''-'' meaning standard output.  A filesystem can be migrated using a
pipeline.  For example, to migrate the root filesystem (/), where rXXXa is 
the name of the destination:
.DS
\fB#\fP newfs /dev/r\fIXXX\fRa
\fB#\fP dump 0f - / | restor rf - /dev/r\fIXXX\fRa
.DE
.PP
The \fIrestor\fP program accesses the raw device, restoring inodes with their 
original inode number as recorded by dump, while disk blocks are allocated 
from the freelist of the destination filesystem. Defragmenting a filesystem is 
performed using the \fIdump\fP and \fIrestor\fP programs. Unless you have a 
spare disk partition to restore to, you will need to use those commands 
individually, either with tape or to a work file on a filesystem that is 
\fBNOT\fR otherwise used by either command.
.PP
If the destination filesystem has fewer inodes than needed to restore the dump,
you will need to reinitialize the destination filesystem, using the \fI-i\fR 
argument to \fInewfs(8)\fR to increase the number of inodes on the destination 
filesystem. \fBNOTE:\fR lowering the value of \fI-i\fR produces more inodes, 
not less, as the argument following \fI-i\fR specifies the number of bytes per 
inode, not the number of inodes directly. Care must therefore be taken when 
restoring a dump into a filesystem smaller than the original filesystem.
.PP
Filesystems may also be moved by piping the output of a \fItar\fP\|(1)
to another \fItar\fP:
.DS
\fB#\fP tar -cf - / | cd /dev/r\fIXXX\fRa; tar -xpf -
.DE
.PP
The main difference between the two commands is that the restore command (when 
used with the r argument) should only be used on a newly-initialized (empty) 
filesystem, while using tar to extract files can extract them into a 
filesystem with existing data. When using tar, you should make sure you are not
unintentionally overwriting or commingling existing data when restoring from a 
tar file.
.PP
\fBNOTE:\fR If you plan on moving filesystem data between a 2.11BSD system and 
another Unix-like operating system, you should use tar as it is more compatible
across different operating systems. 
.NH 2
Recompiling and reinstalling system software
.PP
It is easy to recompile the system, and it is a good
idea to try rebuilding pieces of the system to build confidence
in the procedures.
The system consists of two major parts:
the kernel itself (/sys) and the user programs
(/usr/src and subdirectories).
The major part of this is /usr/src.
.PP
The three major libraries are the C library in /usr/src/lib/libc
and the \s-2FORTRAN\s0 libraries /usr/src/usr.lib/libI77 and
/usr/src/usr.lib/libF77.  In each
case the library is remade by changing into the corresponding directory
and doing
.DS
\fB#\fP make
.DE
.PP
and then installed by
.DS
\fB#\fP make install
.DE
.PP
Similar to the system,
.DS
\fB#\fP make clean
.DE
.PP
cleans up.
.PP
The source for all other libraries is kept in subdirectories of
/usr/src/usr.lib; each has a Makefile and can be recompiled by the above
recipe.
.PP
If you look at /usr/src/Makefile, you will see that
you can recompile the entire system source with one command.
To recompile a specific program, find
out where the source resides with the \fIwhereis\fP\|(1)
command, then change to that directory and remake it
with the Makefile present in the directory.
For instance, to recompile ``date'', 
all one has to do is:
.DS
\fB#\fP whereis date
\fBdate: /usr/src/bin/date.c /bin/date /usr/man/man1/date.1\fP
\fB#\fP cd /usr/src/bin
\fB#\fP make date
.DE
.PP
This will create an unstripped version of the binary of ``date''
in the current directory.  To install the binary image, use the
install command as in
.DS
\fB#\fP install \-s date /bin/date
.DE
.PP
The \-s option will ensure the installed version of date has
its symbol table stripped.  The install command should be used
instead of mv or cp as it understands how to install programs
even when the program is currently in use.
.PP
If you wish to recompile and install all programs in a particular
target area you can override the default target by doing:
.DS
\fB#\fP make
\fB#\fP make install
.DE
.PP
To recompile all of the system from source, you can do
.DS
\fB#\fP cd /usr/src
\fB#\fP make
.DE
.PP
If you modify the C library, say to change a system call,
and want to rebuild and install everything from scratch you
have to be a little careful.
You must ensure that the libraries are installed before the
remainder of the source, otherwise the loaded images will not
contain the new routine from the library.  The following
sequence will accomplish this.
.DS
\fB#\fP cd /usr/src
\fB#\fP make clean
\fB#\fP make build
\fB#\fP make installsrc
.DE
.PP
The first \fImake\fP removes any existing binaries in the source trees
to ensure that everything is reloaded.
The next \fImake\fP compiles and installs the libraries and compilers,
then compiles the remainder of the sources.
The final line installs all of the commands not installed in the first phase.
This will take about 12 hours on a reasonably configured 11/44.
.NH 2
Making local modifications
.PP
/usr/new is used by default for the programs that constitute
the contributed software portion of the distribution but which may not
have man pages installed.
Locally written commands that aren't distributed (or whose man pages are
not up to date) are kept in /usr/src/local and their binaries are 
kept in /usr/local.  This allows /usr/bin, /usr/ucb,
and /bin to correspond to the distribution tape
People using /usr/local commands are made
aware that the programs may not be in the base system yet.
.NH 2
Accounting
.PP
UNIX optionally records two kinds of accounting information:
connect time accounting and process resource accounting.  The connect
time accounting information is stored in the file \fI/usr/adm/wtmp\fP, which
is summarized by the program
.IR ac (8).
The process time accounting information is stored in the file
\fI/usr/adm/acct\fP after it is enabled by
.IR accton (8),
and is analyzed and summarized by the program
.IR sa (8).
.PP
If you need to recharge for computing time, you can develop
procedures based on the information provided by these commands.
A convenient way to do this is to give commands to the clock daemon
.I cron
to be executed every day at a specified time.  This is done by adding
lines to \fI/usr/adm/crontab\fP; see
.IR cron (8)
for details.
.NH 2
Resource control
.PP
Resource control in \*(2B is more
elaborate than in previous PDP-11 UNIX systems.
The resources consumed
by any single process can be limited by the mechanisms of
\fIsetrlimit\fP\|(2).  As distributed, the mechanism
is voluntary, though sites may choose to modify the login
mechanism to impose limits.  Csh now has the \fIlimits\fP
built-in command enabled.  Another available option
is the 4.3BSD disk quota system.
.NH 2
Files that need periodic attention
.PP
The discussion of system operations is concluded by listing
the files that require periodic attention or are system specific
.de BP
.IP \fB\\$1\fP
.br
..
.TS
center;
lb a.
/etc/fstab	how disk partitions are used
/etc/disktab	disk partition sizes
/etc/printcap	printer data base
/etc/gettytab	terminal type definitions
/etc/remote	names and phone numbers of remote machines for \fItip\fP(1)
/etc/group	group memberships
/etc/motd	message of the day
/etc/master.passwd	password file; each account has a line
/etc/rc.local	local system restart script; runs reboot; starts daemons
/etc/inetd.conf	local internet servers
/etc/hosts	host name data base
/etc/networks	network name data base
/etc/netstart	Startup file to configure network
/etc/services	network services data base
/etc/hosts.equiv	hosts under same administrative control
/etc/syslog.conf	error log configuration for \fIsyslogd\fP\|(8)
/etc/ttys	enables/disables ports
/etc/crontab	commands that are run periodically
/etc/aliases	mail forwarding and distribution groups
/usr/adm/acct	raw process account data
/usr/adm/messages	system error log
/usr/adm/shutdownlog	log of system reboots
/usr/adm/wtmp	login session accounting
.TE
