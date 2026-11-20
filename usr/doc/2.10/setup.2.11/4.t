.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)4.t	3.0.1 (2.11BSD) 2026/11/10
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 4
.nr H2 0
.ds CF \*(DY
.ds RH "System setup
.bp
.LG
.B
.ce
4. SYSTEM SETUP
.sp 2
.R
.NL
.PP
At this point you should have a system which can successfully boot from disk 
into multi-user mode. If it does not, return to the setup steps in Chapter 2 
before proceeding.
.PP
This section describes procedures used to set up a PDP-11 UNIX system.
These procedures are used when a system is first installed
or when the system configuration changes.  Procedures for normal
system operation are described in the next section.
.NH 2
First, a little housekeeping
.PP
There are a number of tasks that you should perform after first installation, 
as well as in the obvious cases where these steps might need to be repeated.
.NH 3
Saving a backup copy of the GENERIC kernel
.PP
You can get yourself into a situation where a custom kernel does not boot, or 
other things have gone wrong. Having a copy of the GENERIC kernel around can be
the difference between a simple recovery and needing to do a re-install.
By convention, this saved copy of the GENERIC kernel is called /genunix and is 
created as follows:
.DS
su
cd /
cp -p /unix /genunix
.DE
.PP
\fBNOTES:\fR 1) You should perform this \fIbefore\fR replacing /unix with a 
customized kernel. 2) Some Patch Levels may make major changes to some part of 
the system. In such cases, it is advisable to build a new GENERIC kernel and 
replaced the saved /genunix with the new kernel. Those Patch Levels will 
include a suggestion to rebuild the GENERIC kernel when necessary.
.PP 
The following shows the steps to use the /genunix kernel by specifying it on 
the command line during the system boot process.
.PP
.nf 
.in +0.5i
.cs CW 20
.ft CW
70Boot from xp(0,0,0) at 0176700
: genunix
Boot: bootdev=05000 bootcsr=0176700

2.11 BSD UNIX #157: Sat Oct 24 16:44:40 CDT 2026 
    sms@vernon.2bsd.com:/usr/src/sys/GENERIC
[...]
.fi
.PP
.NH 3
Setting the correct time zone
.PP
It is quite possible that you are located in a different time zone than US 
Central Time where the GENERIC kernel was built, or where the "canned" 
(prebuilt) disk images were built. The kernel keeps time in GMT by default 
(and should generally not be changed). Instead, the system provides a large 
selection of time zone rules in the directory tree based at 
/usr/share/zoneinfo. Locate the appropriate file and install it as 
/etc/localtime, as shown in the following example (assuming US Eastern Time, 
with automatic switching to Daylight Saving Time and back as needed):
.DS
su
cd /usr/share/zoneinfo
cp -p EST5EDT /etc/localtime
.DE
.PP
\fBNOTES:\fR 1) In addition to the top-level time zone rules, there are 
subdirectories for various countries which have local exceptions. For example, 
the standard US Mountain Time zone rule is in /usr/share/zoneinfo/MST7MDT, but 
most of Arizona does not observe Daylight Saving Time, so it has its own rule 
in /usr/share/zoneinfo/US/Arizona. 2) Governments sometimes change their time 
zone rules, often with little advance notice. Some of the zoneinfo files 
included with 2.11BSD may be out-of-date. Work is planned (as part of a future 
Patch Level) to import the latest IANA tzdata.
.NH 3
Setting the system host name
.PP
All programs that require the site's name, or some similar
characteristic, obtain the information through system calls
or from files located in /etc.  Aside from parts of the
system related to the network, to tailor the system to your
site you must simply select a host name, then edit the file
/etc/rc.conf and change the "rc_configured" line at or about 
line 7 from "NO" to "YES", and then change the "hostname" line 
at or about line 32 to something like:
.DS
hostname=\fImyname.my.domain\fP
.DE
.PP
\fBNOTES:\fR 1) Be sure to remove the leading pound (comment) character as
well as the blank space following it ("# ") in the hostname line or your 
hostname will not be configured. 2) If you are running a system with 
networking enabled, you should instead refer to Chapter 5, "Network Setup".
.PP
Setting the hostname defines the value returned by the 
.IR gethostname (2)
system call.  Your hostname should be a fully qualified 
domain name. Since you are not running a system with networking, you can use a 
name like "mysystem.example.com".  Programs such as
.IR getty (8),
.IR mail (1),
.IR wall (1),
.IR uucp (1),
and
.IR who (1)
use this system call so that the binary images are site
independent.
.NH 3
Setting root's email forwarding
.PP
By default, the system will mail the output of the scheduled daily / weekly / 
monthly reports to the root user. Since it is not good security practice to 
run as root when not needed, you can set root's email forwarding elsewhere, 
either to another user account on the system or (if you have networking 
enabled) to any email address the system can reach. The address to forward 
mail to is saved in root's .forward file (\fI/root/.forward\fR). To add or 
change forwarding, use the one of the following commands, depending on whether 
you are forwarding to a local or remote user:
.DS
\fB#\fP cd /root; echo "\fIusername\fP" > .forward
\fB#\fP cd /root; echo "\fIusername@hostname\fP" > .forward
.DE
.NH 3
Adjusting the line time clock frequency (if needed)
.PP
Both the GENERIC kernel and the "canned" CLASSIC system disk image's 
kernel are built with "LINEHZ 60", while the "canned" PIDP11 system disk 
image's kernel is built with "LINEHZ 50". A late-breaking change during PL500 
development made this setting configurable without needing to build a new 
kernel. At the moment, this involves giving the command 
"sysctl -w kern.linehz=N" where N is either 50 or 60. Ideally, this should be 
done before the system goes into multi-user mode, but it can be made at any 
time. \fBNOTE:\fR If you are running the Network Time protocol (refer to 
Chapter 5, Network Setup for more information) it is important to have the 
correct line time clock frequency set correctly before NTP is enabled.
.NH 2
Disk configuration
.PP
This section describes how to layout filesystems to make use
of the available space and to balance disk load for better system
performance. \fBNOTE:\fR The information in this section was of far greater 
importance on PDP-11 systems with relatively slow peripherals. If you are 
running on an emulator, you can create pretty much any filesystem layout and 
combination of disk drives that you want, without affecting performance. In
this case, you can skip to Section 4.3, "Configuring terminals" if you 
installed from the distribution tape, or to Section 4.4, "Adding and managing 
users and groups" if you are running a "canned" (pre-built) disk image.
.PP
This section remains both for users with actual PDP-11 systems, as well as 
documenting the historic procedures used.
.NH 3
Disk naming and divisions
.PP
Each physical disk drive can be divided into up to 8 partitions;
UNIX typically
uses only 3 or 4 partitions.
For instance, on an RP06
the first partition, xp0a,
is used for a root filesystem, a backup thereof,
or a small filesystem like, /tmp;
the second partition, xp0b,
is used for swapping or a small filesystem; and
a combination of the remaining partitions (xp0d, xp0e, xp0f, xp0g, xp0h)
would hold user filesystems.
.sp
.RS
.PP
\fBWarning:\fP  for disks on which DEC standard 144 bad sector forwarding
is supported, the last track and up to 126 preceding sectors
contain replacement sectors and bad sector lists.
Disk-to-disk copies should be careful to avoid overwriting this information.
See \fIbad144\fP\|(8).
Bad sector forwarding is optional in the \fBhk\fP
and \fBxp\fP drivers.
The partition sizes listed in \fI/etc/disktab\fP that \fInewfs\fP\|(8)
uses automatically reserve the maximum amount of room that may be used
by bad block forwarding on a disk.
.PP
Note also that bad144 style bad block forwarding \fIcannot\fP be used
with SI controllers on the xp driver as the controllers use their own
internal scheme for bad block forwarding, and you can in fact make your
disks unusable on the SI controllers if you write anything in the last
five cylinders.  The partition sizes in \fI/etc/disktab\fP also handle
this constraint automatically.
.PP
The generic distribution kernel does not do bad block forwarding.  There is
unfortunately no way to run bad144 style bad block forwarding on some of your
disks but not others.  As a final bug the hk and xp drivers do not reread
the bad sector forwarding information when disk packs are changed and so
will erroneously use bad block forwarding information from the wrong packs!
.RE
.sp
.PP
The space available on a disk varies, not surprisingly, per device.  
Disk labels make a table giving sizes meaningless since there are no
predefined partition sizes embedded in the kernel any longer.  The root
filesystem (\fBa\fP) must be at least 4MB, preferably 8 to 10MB if possible.
The swap area (almost always the \fBb\fP partition) should be 4MB or a little
more.  It is a rare case where more than
5 or 6MB of swap space is required.
.PP
The system (boot) disk has a swapping area and
a root filesystem.  Other drives may use those partitions for data.
\fBRemember:\fP the \fBa\fP partition must start at sector 0 or else neither
\fIdisklabel\fP\|(8) nor the kernel will be able to read/write the
disk label.
.PP
The distributed system binaries occupy about 34 Megabytes
while the major sources occupy another 36 Megabytes.  Adding in the
miscellaneous sources, a few locate works of art bring the total for
a complete system to about 90 Megabytes.
This overflows RK07, RL02 and RM03 systems,
but fits easily on most other hardware configurations.  \*(2B is quite
happy on RD54 or larger.
.PP
Be aware that the disks have their sizes measured in disk sectors (512
bytes), while the UNIX filesystem blocks are 1024 bytes each.  Thus, if a
disk partition has 10000 sectors (disk blocks), it will have only 5000
UNIX filesystem blocks, and you \fImust\fP divide by 2 to use 5000 when
specifying the size to the \fImkfs\fP command for instance.  The 
\fInewfs\fP\|(8) program performs this calculation automatically.   You
should \fBnever\fP need to run \fImkfs\fP manually.
All user
programs report disk space in kilobytes and, where needed, disk sizes are
always specified in units of sectors.  The /etc/disktab file used in
making filesystems specifies disk partition sizes in sectors; the
default sector size may be overridden with the ``se'' attribute.  \fBNote\fP
that the only sector size currently supported is NBPG as defined in 
\fI/sys/pdp/machparam.h\fP.  This restriction is enforced in several places
in the disk labeling process as a safeguard against specifying a sector
size other than NBPG (512).  Any other sector size would produce strange
results and almost certainly curdled filesystems.
.NH 3
Layout considerations
.PP
There are several considerations in deciding how
to adjust the arrangement of things on your disks.
The most important is making sure that there is adequate space
for what is required; secondarily, throughput should be maximized.
Swap space is an important parameter since it defines the maximum
process image load that may be run.  If, for instance, your swap
area was smaller than the amount of main memory available after
the kernel took its share, some of your memory would never
be used.
.PP
Many common system programs (C, the editor, the assembler etc.)
create intermediate files in the /tmp directory,
so the filesystem where this is stored also should be made
large enough to accommodate
most high-water marks; if you have several disks, it makes
sense to mount this in a ``root'' (i.e. first partition)
filesystem on another disk.
All the programs that create files in /tmp take
care to delete them, but are not immune to rare events
and can leave dregs.
The directory should be examined every so often and the old
files deleted.
.PP
It is critical for good performance on real PDP-11 systems to balance the disk 
load across multiple drives (if available). On an emulator, all drives are 
normally container files in the same directory on the host system, so there is 
much less need to balance the disk load (as it is all accessing the same host 
device). To mimic certain real historic systems, it may be desirable to have 
multiple disk drives, even though there is almost no performance benefit from 
doing so.
.PP
There are at least five components of the disk load that you can
divide between the available disks:
.DS
1. The root filesystem.
2. The /tmp filesystem.
3. The /usr filesystem.
4. The user files.
5. The swapping activity.
.DE
.PP
The following possibilities are ones that have been used at times
when 2, 3 and 4 disks were available:
.TS
center doublebox;
l | c s s
l | lw(5) | lw(5) | lw(5).
	disks
what	2	3	4
_
/	0	0	0
tmp	1	2	3
usr	1	1	1
swapping	0	2	2
users	0	0+2	0+2
archive	x	x	3
.TE
.NH 3
Implementing a layout
.PP
To put a chosen disk layout into effect, you should use the
.IR newfs (8)
command to create each new filesystem.
Each filesystem must also be added to the file
/etc/fstab
so that it will be checked and mounted when the system is bootstrapped.
.PP
As an example, consider a system with RA80's.  On the first RA80, ra0,
we will put the root filesystem in ra0a, and the /usr
filesystem in ra0c, which has enough space to hold it and then some.
The /tmp directory will be part of the root filesystem,
as no filesystem will be mounted on /tmp.
If we had only one RA80, we would put user files
in the ra0c partition with the system source and binaries.
.PP
If we had a second RA80, we would place \fI/usr\fP in ra1c.
We would put user files in ra0c, calling the filesystem /mnt.
We would put swap on \fIra0b\fP.
We would keep a backup copy of the root
filesystem in the \fBra1a\fP disk partition and
put /tmp on \fIra1b\fP.
\fI/etc/fstab\fP would then contain
.DS
/dev/ra0a:/:rw:1:1
/dev/ra0b::sw::
/dev/ra0c:/mnt:rw:1:2
/dev/ra1b:/tmp:rw::
/dev/ra1c:/usr:rw:1:2
.DE
.PP
To make the /mnt filesystem we would do:
.DS
\fB#\fP cd /dev
\fB#\fP ./MAKEDEV ra1
\fB#\fP newfs ra1c ra80
(information about filesystem prints out)
(to specify an alternate \fIm\fP value: newfs -m # ra1c ra80)
(where # is between 1 and 31)
\fB#\fP mkdir /mnt
\fB#\fP mount /dev/ra1c /mnt
.DE
.NH 2
Configuring terminals
.PP
If UNIX is to support simultaneous
access from directly-connected terminals other than the console,
the file \fI/etc/ttys\fP (\fIttys\fP\|(5)) must be edited.
.PP
Terminals connected via DZ11 interfaces are conventionally named \fBttyDD\fP
where DD is a decimal number, the ``minor device'' number.
The lines on dz0 are named /dev/tty00, /dev/tty01, ... /dev/tty07.
By convention, all other terminal names are of the form \fBtty\fPCX, where
C is an alphabetic character according to the type of terminal multiplexor
and its unit number,
and X is a digit for the first ten lines on the interface
and an increasing lower-case letter for the rest of the lines.
C is defined for the number of interfaces of each type listed below.
Since tty structures are approximately 78 bytes each, it is
highly doubtful that more than 3 or 4 terminal interface boards
will ever be attached to a PDP-11 (especially in a BA23 cabinet).
.DS
.TS
center box;
c c c c
c c c c
l c n n.
Interface		Number of lines	Number of
Type	Characters	per board	Interfaces
_
DZ11	see above	8	10
DH11	h-o	16	8
DHU11	S-Z	16	8
pty	p-u	16	6
.TE
.DE
.PP
To add a new terminal device, be sure the device is configured into the system
and that the special files for the device have been made by /dev/MAKEDEV.
Then, enable the appropriate lines of /etc/ttys by setting the ``status''
field to \fBon\fP (or add new lines).
Note that lines in \fI/etc/ttys\fP are one-for-one with entries
in the file of current users (\fI/var/run/utmp\fP),
and therefore it is best to make changes
while running in single-user mode
and to add all of the entries for a new device at once.
.PP
The format of the /etc/ttys file is the same in \*(2B as in \*(Ps and 4.3BSD.
Each line in the file is broken into four tab separated
fields (comments are shown by a `#' character and extend to
the end of the line).  For each terminal line the four fields
are:
the device (without a leading /dev),
the program /etc/init should startup to service the line
(or \fBnone\fP if the line is to be left alone),
the terminal type (found in /etc/termcap),
and optional status information describing if the terminal is
enabled or not and if it is ``secure'' (i.e. the super user should
be allowed to login on the line).  All fields are character strings
with entries requiring embedded white space enclosed in double
quotes.
Thus, a newly added terminal /dev/tty00 could be added as
.DS
tty00 	"/usr/libexec/getty std.9600"	vt100	on secure	# Steve's office
.DE
.PP
The std.9600 parameter provided
to /usr/libexec/getty is used in searching the file /etc/gettytab; it specifies
a terminal's characteristics (such as baud rate).
To make custom terminal types, consult 
.IR gettytab (5)
before modifying /etc/gettytab.
.PP
Dialup terminals should be wired so that carrier is asserted only when the
phone line is dialed up.
For non-dialup terminals from which modem control
is not available, you must either wire back the signals so that
the carrier appears to always be present, or show in the minor device number
that carrier is to be assumed to be present
by adding 128 decimal to the minor device number when creating the
device node.  This differs from 4.3BSD where the softcarrier state is
specified at kernel configuration time.
.PP
For network terminals (i.e. pseudo terminals), no program should
be started up on the lines.  Thus, the normal entry in /etc/ttys
would look like
.DS
ttyp0 	none	network	(Note the fourth field is not needed here.)
.DE
.PP
When the system is running multi-user, all terminals that are listed
in /etc/ttys as \fBon\fP have their line are enabled.
If, during normal operations, it is desired
to disable a terminal line, you can edit the file
/etc/ttys
to change the terminal's status to \fBoff\fP and
then send a hangup signal to the \fIinit\fP process, by doing
.DS
\fB#\fP kill \-1 1
.DE
.PP
Terminals can similarly be enabled by changing the status field
from \fBoff\fP to \fBon\fP and sending a hangup signal to \fIinit\fP.
.PP
Note that if a special file is inaccessible when \fIinit\fP tries
to create a process for it, init will log a message to the
system error logging process (/usr/sbin/syslogd)
and try to reopen the terminal every minute, reprinting the warning
message every 10 minutes.  Messages of this sort are normally
printed on the console, though other actions may occur depending
on the configuration information found in /etc/syslog.conf.
.PP
Note that you should change the names of any dialup
terminals to ttyd?
where ? is in [0-9a-zA-Z], as some programs use this property of the
names to determine if a terminal is a dialup.
Shell commands to do this should be put in the /dev/MAKEDEV.local
script.
.PP
To further complicate things, some emulators (such as SIMH) have the ability to
map network ports on the host system to emulated terminal multiplexor lines on 
the PDP-11 system. The benefit of this is that you can allow network logins to 
your 2.11BSD system without setting up networking under 2.11BSD. The CLASSIC 
"canned" distribution provides this in its classic.ini file, as well as 
examples of mapping physical terminal ports on the host system to emulated 
multiplexor ports on the PDP-11 system.
.NH 2
Adding and managing users and groups
.PP
New users can be added to the system by adding a line to the
password file
/etc/passwd.
The procedure for adding a new user is described in
\fIadduser(8)\fR. This is currently a manual procedure. A future Patch Level 
may provide an \fIadduser\fR command that prompts you for each setting and then 
does the work for you.
.PP
Users are assigned to their primary group by specifying a group number (from 
the file /etc/group). It is important to realize that certain groups listed in 
the /etc/group file have special capabilities and should \fBNOT\fR be normally 
assigned to users. Normally you would use the staff or guest group and avoid 
assigning users to any of the other existing groups, as that would grant those 
users additional permissions which is likely undesirable.
.PP
You can also add additional groups to the /etc/group file. For example, if you 
had a number of users working on a ``widgets'' project, you could add a 
widgets group to the /etc/group file. \fBNOTE:\fR Take care to pick a new, 
unique number for your new group - do not re-use an existing group number.
.PP
In addition to the primary group for each user, users can be granted 
membership in additional groups by listing them (separated with commas if there
is more than one user who should be in the additional group). For example, if 
users ``fred'' and ``barney'' should have the ability to manage some system 
operations, but not have full control over the system, you can add them to the 
"operator" group. For example, to allow ``fred'' and ``barney'' to shut down or 
reboot the system and similar tasks, you would add them to the operator group 
as shown in the following example. Before:
.DS
operator:*:5:
.DE
.PP
After:
.DS
operator:*:5:fred,barney
.DE
.PP
You can edit user entries in the password file using the \fIvipw(8)\fR utility.
You can also use the \fIvipw(8)\fR utility to remove a user by removing their 
entry from the password file. \fBNOTE:\fR When removing a user, it is important
to remove them from the /etc/group file (if they are named there) as well as 
either archiving or deleting their login directory, as appropriate.
.PP
As the system administrator, you can change a user's password for them by using
the \fIpasswd(1)\fR utility. Individual users can use the same utility, 
although normally only the system administrator is allowed to change the 
passwords of other users.
.NH 2
Setting up the mail system
.PP
The mail system consists of the following commands:
.DS
.TS
l l.
/bin/mail	old standard mail program, \fIbinmail\fP\|(1)
/usr/ucb/mail	UCB mail program, described in \fImail\fP\|(1)
/usr/sbin/sendmail	mail routing program
/usr/spool/mail	mail spooling directory
/etc/aliases	mail forwarding information
/usr/bin/newaliases	command to rebuild binary forwarding database
/usr/ucb/biff	mail notification enabler\(dg
/usr/libexec/comsat	mail notification daemon\(dg
.TE
.DE
.PP
Mail is normally sent and received using the
.IR mail (1)
command, which provides a front-end to edit the messages sent
and received, and passes the messages to
.IR sendmail (8)
for routing.
The routing algorithm uses knowledge of the network name syntax,
aliasing and forwarding information, and network topology, as
defined in the configuration file /etc/sendmail.cf, to
process each piece of mail.
Local mail is delivered by giving it to the program /bin/mail
that adds it to the mailboxes in the directory /usr/spool/mail/\fIusername\fP,
using a locking protocol to avoid problems with simultaneous updates.
After the mail is delivered, the local mail delivery daemon /usr/libexec/comsat
is notified, which in turn notifies
users who have issued a ``\fIbiff\fP y'' command that mail has arrived\(dg.
.FS
.IP \(dg
comsat and biff are only available under systems configured
for networking support.
.FE
.PP
To set up the mail facility you should read the instructions in the
file READ_ME in the directory /usr/src/usr.lib/sendmail and then adjust
the necessary configuration files.
Documents describing 
.IR sendmail 's
operation and installation are also included on the distribution tape.
.NH 3
Setting up a UUCP connection
.PP
The version of \fIuucp\fP included in \*(2B is an
enhanced version of the one originally distributed with 32/V\(dd.
.FS
.IP \(dd
The \fIuucp\fP included in this distribution is the result
of work by many people; we gratefully acknowledge their
contributions, but refrain from mentioning names in the 
interest of keeping this document current.
.FE
The enhancements include:
.DS
\(bu Support for many auto call units and dialers in addition to the DEC DN11.
\(bu Breakup of the spooling area into multiple subdirectories
\(bu Addition of an \fIL.cmds\fP file to control the set of commands that may be executed by a remote site
\(bu Enhanced ``expect-send'' sequence capabilities when logging in to a remote site
\(bu New commands to be used in polling sites and obtaining snap shots of \fIuucp\fP activity
\(bu Additional protocols for different communication media
.DE
.PP
This section gives a brief overview of \fIuucp\fP
and points out the most important steps in its installation.
.PP
To connect two UNIX machines with a \fIuucp\fP network link using modems,
one site must have an automatic call unit
and the other must have a dialup port.
It is better if both sites have both.
.PP
You should first read the paper in the UNIX System Manager's Manual:
``UUCP Implementation Description''.
It describes in detail the file formats and conventions,
and will give you a little context.
In addition,
the document ``setup.tblms'',
located in the directory /usr/src/usr.bin/uucp/UUAIDS,
may be of use in tailoring the software to your needs.
.PP
The \fIuucp\fP support is located in three major directories:
/usr/bin,
/etc/uucp,
and /usr/spool/uucp.
User commands are kept in /usr/bin,
operational commands in /etc/uucp,
and /usr/spool/uucp is used as a spooling area.
The commands in /usr/bin are:
.DS
.TS
l l.
/usr/bin/uucp	file-copy command
/usr/bin/uux	remote execution command
/usr/bin/uusend	binary file transfer using mail
/usr/bin/uuencode	binary file encoder (for \fIuusend\fP)
/usr/bin/uudecode	binary file decoder (for \fIuusend\fP)
/usr/bin/uulog	scans session log files
/usr/bin/uusnap	gives a snap-shot of \fIuucp\fP activity
/usr/bin/uupoll	polls remote system until an answer is received
/usr/bin/uuname	prints a list of known uucp hosts
/usr/bin/uuq	gives information about the queue
.TE
.DE
The important files and commands in /etc/uucp are:
.DS
.TS
l l.
/etc/uucp/L-devices	list of dialers and hard-wired lines
/etc/uucp/L-dialcodes	dialcode abbreviations
/etc/uucp/L.aliases	hostname aliases
/etc/uucp/L.cmds	commands remote sites may execute
/etc/uucp/L.sys	systems to communicate with, how to connect, and when
/etc/uucp/SEQF	sequence numbering control file
/etc/uucp/USERFILE	remote site pathname access specifications
/usr/sbin/uucico	\fIuucp\fP protocol daemon
/etc/uucp/uuclean	cleans up garbage files in spool area
/usr/libexec/uuxqt	\fIuucp\fP remote execution server
.TE
.DE
while the spooling area contains the following important files and directories:
.DS
.TS
l l.
/usr/spool/uucp/C.	directory for command, ``C.'' files
/usr/spool/uucp/D.	directory for data, ``D.'', files
/usr/spool/uucp/X.	directory for command execution, ``X.'', files
/usr/spool/uucp/D.\fImachine\fP	directory for local ``D.'' files
/usr/spool/uucp/D.\fImachine\fPX	directory for local ``X.'' files
/usr/spool/uucp/TM.	directory for temporary, ``TM.'', files
/usr/spool/uucp/LOGFILE	log file of \fIuucp\fP activity
/usr/spool/uucp/SYSLOG	log file of \fIuucp\fP file transfers
.TE
.DE
.PP
To install \fIuucp\fP on your system,
start by selecting a site name.
A \fIuucp\fP account must be created in the password file and a password set up.
Then,
create the appropriate spooling directories with mode 755
and owned by user \fIuucp\fP, group \fIdaemon\fP.
.PP
If you have an auto-call unit,
the L.sys, L-dialcodes, and L-devices files should be created.
The L.sys file should contain
the phone numbers and login sequences
required to establish a connection with a \fIuucp\fP daemon on another machine.
For example, my L.sys file looks something like:
.sp
.DS
elisa Any ACU 1200 7064297  "" \\\\r\\\\c ogin-EOT-ogin-\\\\r\\\\c-ogin xelisa assword: XXX
etn-ra Any ACU 1200 8891237  "" \\\\r\\\\c ogin nuucp assword XXX
anagld Never ACU 2400 8894517 name:-EOT-name: uucp assword: XXX
.DE
.sp
The first field is the name of a site,
the second shows when the machine may be called,
the third field specifies how the host is connected
(through an ACU, a hard-wired line, etc.),
then comes the phone number to use in connecting through an auto-call unit,
and finally a login sequence.
The phone number
may contain common abbreviations that are defined in the L-dialcodes file.
The device specification should refer to devices
specified in the L-devices file.
Listing only ACU causes the \fIuucp\fP daemon, \fIuucico\fP,
to search for any available auto-call unit in L-devices.
Refer to the README file in the \fIuucp\fP source directory
for more information about installation.
.PP
As \fIuucp\fP operates it creates (and removes) many small
files in the directories underneath /usr/spool/uucp.
Sometimes files are left undeleted;
these are most easily purged with the \fIuuclean\fP program.
The log files can grow without bound unless trimmed back;
\fIuulog\fP maintains these files.
Many useful aids in maintaining your \fIuucp\fP installation
are included in a subdirectory UUAIDS beneath /usr/src/usr.bin/uucp.
Peruse this directory and read the ``setup'' instructions also located there.
