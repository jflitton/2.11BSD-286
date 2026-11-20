.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)1.t	3.0.1 (2.11BSD) 2026/11/10
.\"
.ds lq ``
.ds rq ''
.ds LH "Installing/Operating \*(2B
.ds RH Introduction
.ds CF \*(DY
.LP
.nr H1 1
.nr H2 0
.bp
.LG
.B
.ce
1. INTRODUCTION
.sp 2
.R
.NL
.\" NOTE below is just something you should be aware of when editing
.\" this manual. FIXME below indicates an open issue which needs to
.\" be resolved, ideally before the release of this manual.
.\"
.\" NOTE When installing this directory on a 2.11BSD system, the files
.\" GENERIC and EXAMPLE should be symlinks to the ones in /sys/conf.
.\"
.\" NOTE When rendering the PDF version of this manual in Adobe Acrobat,
.\" any missing fonts will need to be installed. DO NOT allow Acrobat to
.\" perform font substitution as text will be mis-aligned.
.\"
.\" NOTE If body text in a chapter is changed, all footnotes in that chap-
.\" ter will need to be checked to make sure they are on the same page as
.\" their reference(s) and that the order is intact. The standard order is:
.\" dagger, double dagger, section mark, bullet mark, asterisk and pound. 
.\" These are dg, dd, sc, bu, *, and #.
.\"
.\" NOTE There are explicit page breaks (generally in tables) to prevent
.\" large amounts of blank paper at the bottoms of pages. These may need
.\" to be repositioned if the body text changes and may also render poorly
.\" in nroff text format output.
.\"
.\" FIXME When doing "make nroff" on 2.11BSD, 4 "Zero field width." mes-
.\" sages are generated.
.\"
.\" FIXME Adjust double quotes, italics, manual section references to be
.\" consistent throughout. Will require someone with fresh eyes to look at
.\" it.
.\" 
.\" FIXME Need to add Appendix D for "2.11BSD for modern UNIX users"
.\" 
.\" FIXME Evaluate material from http://mim.softjar.se/pidpbsd.htm and see
.\" if it should be incorporated here.
.\" 
.NH 1 
Overview
.PP
This document explains how to install \*(2B UNIX on your PDP-11
system.  This document has been revised several times since the first
release of \*(2B in 1991. The last significant revision in July 1995 
dealt with the addition of disk labels to the system.  This revision is 
for the 2026 release of the 35th Anniversary Edition of \*(2B, which
also includes preconfigured ("canned") bootable disk images for users of
PDP-11 emulators, described in detail below.
.PP
The procedure for performing a full bootstrap is outlined in Chapter 2.
The process includes copying a root filesystem from
the distribution tape into a new filesystem,
booting that root filesystem, and then reading the remainder of the system
binaries and sources from the archives on the tapes.
.PP
As \*(2B is not compatible at the filesystem level with previous versions
of UNIX on the PDP-11,
any upgrade procedure is essentially a full bootstrap\(dg).  There is a
limited ability to access old filesystems which may be used after
the system partitions have been loaded from a full bootstrap.
It is desirable to recompile most local software after the conversion,
as there are changes and performance improvements in the standard
libraries.
.FS
.IP \(dg
Refer to Appendix C for special considerations when upgrading from earlier 
2.11BSD Patch Levels.
.FE
.PP
.NH 2
A brief history of 2BSD
.PP
.\" Approved by SMS 22-Aug-2026
The original 2BSD (short for "Second Berkeley Software Distribution", which 
is quite a mouthful) was put together by Bill Joy in 1979. Many subsequent 
releases followed, including 2.8BSD in 1982, 2.9BSD in 1983, 2.10BSD in 1987 
and finally 2.11BSD in 1991. Each release added more parts of 4BSD (the BSD 
release for DEC VAX systems), with 2.11BSD adding full TCP/IP networking 
support.
.PP
Steven Schultz did nearly all of the work for the 2.11BSD release, including 
relocating the TCP/IP networking support from the kernel to supervisor mode. 
Rather than creating additional version numbers every time something major 
changed in 2.11BSD, the concept of "Patch Levels" was introduced. Some Patch 
Levels were minor changes, while others were massive changes (for example, 
updating the C compiler and library to modern ANSI standards). Many Patch 
Levels were partially or nearly entirely contributed by other volunteers, too 
numerous to credit here individually. This 35th Anniversary Release of 2.11BSD 
is Patch Level 500. There will almost certainly be additional Patch Levels 
released in the future.
.PP
None of us who ran 2.11BSD "back in the day" envisioned the operating system 
still being under active development 35 years later, but here we are. In 
addition to some systems still being used in production, "retrocomputing" 
enthusiasts and museums have kindled new interest in 2BSD, as well as other 
older operating systems. Some enthusiasts and museums run 2.11BSD on actual 
hardware (one of the authors of this document knows of enthusiasts who have 
multiple complete PDP-11/70 systems in their homes) as well as on PDP-11 
emulators such as SIMH. Emulators have the advantage of using \fBfar\fR less
electricity and space than actual hardware, as well as making additional 
hardware "free". For example, in 1984 adding an additional RA81 disk drive cost
about $19,000, while under an emulator it is a one-line change to a 
configuration file. And you don't have to worry about your emulated hardware 
breaking! 
.PP
2.11BSD represents (in the opinion of one of the authors of this document) the 
pinnacle of what can be achieved in a UNIX environment on a PDP-11. Newcomers 
may wonder why some feature they're used to from modern UNIX-like systems (for 
example, Perl) isn't present in 2.11BSD. The short answer is there's only so 
much you can squeeze into a system with a 64-kilobyte address space and a 
maximum of 4 megabytes of total system memory. 
.\" A more detailed explanation of what you can and can't expect to find in 
.\" 2.11BSD, compared to more modern systems, can be found in Appendix D. 
.PP
.NH 2
Supported hardware
.PP
This distribution can be booted on a PDP-11 system equipped with 1MB or
more of main memory, with separate I/D space and supervisor mode. Floating 
point (if not standard in your CPU model) is strongly recommended\(dg, and 
booting from any of the following disks:
.FS
.IP \(dg
In practice, this includes the PDP-11/44, PDP-11/70, and J11-based models 
(PDP-11/53, /73, /83, /84, /93 and /94), as well as a number of third-party 
CPUs compatible with one of the above models.
.FE
.DS
.TS
lw(1.5i) l.
RL02
RK06, RK07
RM02, RM03, RM05
RP04, RP05, RP06
SMD disks (using controllers with RM/RP emulation)
SMD disks (using controllers with MSCP emulation)
SMD disks (using System Industries 9500 controller)
Most MSCP disks (RAxx, RDxx models, exceptions below)
.TE
.DE
.PP
Many companies have produced aftermarket controllers or entire disk 
subsystems. If these faithfully emulate one of the above models, they
should work as well.
.PP
It is important to note that many controllers that
emulate DEC RM or RP disk systems can be configured to exactly match the
capacity of one or more DEC models, or to other configurations that use
the entire disk capacity. If your system has RM or RP disks configured
for non-standard sizes, you will need to manually define the disk
during the standalone disklabel phase of installation, and optionally add a 
definition of your custom disks to the \fI/etc/disktab\fR file. In cases like 
this, editing \fI/etc/disktab\fR is just a potential convenience if you have 
additional drives of the same size that need labels.
.PP
Controllers that emulate MSCP disks (RA and RD types) report their exact 
drive capacity and other information to the operating system. However, if 
your MSCP disk does not exactly match the capacity of any of the DEC MSCP
drives, you should start by using the default size as reported by the 
standalone disklabel program and define your partitions as desired. You can
optionally add a new disk definition as described in the previous paragraph.
.PP
Non-standard sizes of other disk devices (for example, the RK07) are not
supported by 2.11BSD as the drivers have fixed assumptions about capacity
and geometry.
.PP
Other disks are supported (RX23, RX33, RX50, RD51, RK05, RL01, RX02) but are 
not large enough to hold a root filesystem plus a swap partition.  It is 
possible to create a bootable 2.11BSD system on a single RL02 disk drive, but 
such a system will be severely limited.
.PP
Disks which are too small to hold even a root filesystem (floppies for 
example) may be used as data disks or as standalone boot media, but are not 
useable for loading the distribution.  Others, while listed above, are not very
well suited to loading the distribution.  The RK06/07 and RL02 drives are hard 
pressed to even hold the system binaries, much less the sources.
.PP
If you are 
using a PDP-11 emulator such as SIMH, hardware is essentially free, so you 
should choose one of the larger disk models (RM05, RP06, or an MSCP drive).
.PP
The tape drives supported by this distribution are:
.DS
.TS
lw(1.5i) l.
TS11, TU80, TK25
TM11 (including AVIV 6250/1600)
TE16, TU45, TU77
TK50, TU81, TU81+, TZ30
.TE
.DE
.PP
Third-party tape controllers and drives that emulate any of the above models
are also supported. Since tapes do not have a known capacity, the size issues 
mentioned in the above section on disks do not happen with tapes.
.NH 2
Distribution format ("canned" bootable disk images)
.PP
As most users will be running \*(2B under a PDP-11 emulator such as SIMH,
two types of preconfigured (canned) bootable disk images are provided. 
Although this is a great convenience for emulator users, to get the full \*(2B
experience, we suggest that (once you are familiar with the system) you
do a "from scratch" installation of \*(2B from tape. This will allow more
customization (for example, disk layout) and give you a deeper understanding of
\*(2B. After all, emulated hardware is free, and you can't break it - if
something doesn't work out, you can always start over.
.PP
There are two available canned images, depending on what sort of environment 
you desire. Each comes with a .ini file for the SIMH emulator as a starting 
point.
.PP
.NH 3
Classic PDP-11/70 system with MASSBUS
.PP
The first image is for a PDP-11/70 system with classic MASSBUS peripherals,
upgraded with an Ethernet controller and a modern terminal multiplexor. It 
supports the following hardware:
.DS
.TS
lw(1.5i) l.
PDP-11/70 CPU with 4MB memory and floating-point processor
60Hz line clock (can be reconfigured for 50Hz)
DL11 console terminal
RH70 with RM05 disk drives (maximum quantity 4)
RH70 with TU77 tape drive
DELUA Ethernet controller
DHU11 16-port terminal multiplexor
LP11 line printer
TCP/IP networking
.TE
.DE 
.PP
It is supplied and maintained by Terri Kennedy, \fIterri-decus@glaver.org\fP and
can be downloaded from: 
https://www.glaver.org/PiDP-11/tk2bsd/CLASSIC-211BSD.tgz (or .zip)
.PP
.NH 3
Modern PDP-11/70\(dg system and peripherals
.PP
The second image is for a PDP-11/70\(dg system with a variety of modern 
peripherals, including an Ethernet controller and a terminal multiplexor. It
supports the following hardware:
.FS
.IP \(dg
This image should actually work on any system with an 11/70-class UNIBUS 
processor, as well as on QBUS systems based on the J11 processor, as long as 
the system has at least 1MB of memory.
.FE
.DS
.TS
lw(1.5i) l.
PDP-11/70 CPU supporting an optional floating-point processor and at least 1MB of memory
50Hz line clock (can be reconfigured for 60Hz)
DL11 console terminal
UDA50/KDA50 with MSCP disk drives (maximum quantity 4)
RL11/RLV11 with RL02 disk drives (maximum quantity 4)
RH70/RH11 with RM/RP-series disk drives (maximum quantity 4)
KLESI with TMSCP tape drive
DEUNA/DEQNA Ethernet controllers
DZ11 8-port terminal multiplexor
TCP/IP networking
.TE
.DE 
.PP
It is supplied and maintained by Johnny Billquist, \fIbqt@softjar.se\fR and can 
be downloaded from: http://mim.softjar.se/pidpbsd.htm
.NH 3
Steps needed when using any canned system
.PP
Regardless of which canned disk image you use, you will still need to perform 
some setup steps to customize the system for use. You can skip ahead to Section
4.4, "Adding and managing users and groups" if you are using a canned image.
.NH 3
Additional documentation
.PP
Additionally, PDF versions of this manual as well as the USENIX 4.3BSD Manual 
Set are available from:
https://www.glaver.org/PiDP-11/tk2bsddocs/2bsddocs.tgz (or .zip)
.PP
Many of the 4.3BSD manuals also apply to \*(2B, while some do not. The PDF 
versions are offered as a convenience, however the man pages included with 
\*(2B should be considered authoritative.
.PP
\fBNote:\fP This may not be available at the time of the 35th Anniversary 
Release, but will be released in the future.
.PP
.NH 2
Distribution format (installation from tape)
.PP
The basic distribution contains the following items:
.DS
(1)\0\0 magnetic tape image (in SIMH TAP format)
(1)\0\0 template SIMH config file (simh.ini)
(1)\0\0 Soft copy of this document
.DE
.PP
\fBThe distribution does not fit on several standard PDP-11 configurations
that contain only small disks\fP.  If your hardware configuration does not
provide at \fBleast 75\fP Megabytes of disk space you can still install the
distribution, but you will probably have to operate without source for the
user level commands and, possibly, the source for the operating system. If
you are using an emulator, you can just choose a large disk type (RM05, RP06, 
or MSCP).
.PP
The root filesystem now occupies \fBa minimum of 4MB\fP.  Creating a larger, 
8 to 10MB root filesystem, is highly recommended when using the standalone
.B disklabel
program\(dg.
.FS
.IP \(dg
The root filesystem cannot be arbitrarily large. The exact size varies, but 10MB
should always be a safe upper limit. The reasons for this are discussed in more
detail in Appendix B.
.FE
.PP
The basic bootstrap material is present in six
short files at the beginning of the tape.
The first file on the tape contains the primary bootstrap and program.
This is followed by several standalone
utilities (\fIdisklabel\fP, \fImkfs\fP\|(8), \fIrestor\fP\|(8), and 
\fIicheck\fP\|(8)\(dd)
.FS
.IP \(dd
References of the form X(Y) mean the subsection named
X in section Y of the UNIX Programmer's Manual.
.FE
followed by a full dump of a root filesystem (see \fIdump\fP\|(8)).
Following the root filesystem dump is a \fItar\fP archive of \fB/usr\fP
except for \fB/usr/src\fP.  Next, a \fItar\fP archive
of the kernel source.  Finally, a \fItar\fP archive of
the remaining system sources.
.PP
.DS
TAPE:
.TS
n n n l.
Tape file	Record size	Records\(sc	Contents
_
0	512	1	primary tape boot block
	512	1	boot block (some tape boot ROMs go for this copy)
	512	69	standalone \fBboot\fP program
1	1024	38	standalone \fBdisklabel\fP
2	1024	34	standalone \fBmkfs\fP(8)
3	1024	36	standalone \fBrestor\fP(8)
4	1024	34	standalone \fBicheck\fP(8)
5	10240	321	\fIdump\fP of ``root'' filesystem
6	10240	3254	\fItar\fP dump of /usr, excepting /usr/src
7	10240	293	\fItar\fP dump of /usr/src/sys
8	10240	3190	\fItar\fP dump of /usr/src excepting sys
.TE
.DE
.FS
.IP \(sc
The number of records in each tape file are approximate and do not necessarily 
correspond to the actual number on the distribution tape image.
.FE
.NH 2
UNIX device naming
.PP
UNIX has a set of names for devices which are different from the DEC names 
(of which there may be several for each device), which are also different 
from the names the SIMH emulator uses. The disk and tape names used by the 
2.11BSD bootstrap and system are:
.DS
.TS
l l.
RK05	rk
RK06, RK07 disks	hk
RL01, RL02 disks	rl
MSCP disks/diskettes	ra
RM02/03/05	xp
RP04/05/06	xp
RX02 diskettes (RX01 are not supported)	rx
SMD disks (using RM/RP emulation)	xp
SMD disks (using MSCP emulation)	ra
System Industries 9500 controller, SMD drive(s)	si
TM02/03, TE16, TU45, TU77 tapes	ht
TE10/TM11 tapes	tm
TS11/TU80/TK25 tapes	ts
TK50/TU81/TU81+/TZ30 tapes	tms
Other TMSCP tapes	tms
.TE
.DE
.PP
The generic SMD disk driver, \fIxp\fP, will handle most types of SMD disks on 
one or more controllers (even different disk types on the same controller).
The \fIxp\fP driver handles RM02/03/05, RP04/05/06 disks on DEC or third-party
QBUS, UNIBUS or MASSBUS controllers. Non-standard disk sizes (not matching 
any DEC drive size) are supported, but the user must provide the capacity and
geometry information during system installation and configuration.
.PP
MSCP disks and TMSCP tapes include SCSI drives attached to the 
RQZX1 controller on suported QBUS CPU models.  MSCP disks and TMSCP tapes also 
include SCSI drives attached to third-party UNIBUS and QBUS controllers, as 
long as those controllers emulate a DEC MSCP/TMSCP controller.
.PP
The standalone system used to bootstrap the full UNIX system
uses device names of the form:
.DS
\fIxx\|\fP(\fIc\fP,\fIy\fP,\fIz\fP)
.DE
where \fIxx\fP is one of device types in the above disk and tape tables.
The value \fIc\fP specifies the controller number (0-3).  This value is
usually not explicitly given.  The default is 0 if booting from the standard
(first) CSR of a device.
.PP
Example: if there are two MSCP controllers in
the system addressed as 0172150 and 0172154 respectively booting from the
controller at 172154 requires that \fIc\fP be given as 1.  Booting from
the standard CSR of 0172150 would be done by specifying \fIc\fP as 0 or
omitting the \fIc\fP parameter.
\fBboot\fP automatically detects if the 
first (standard) CSR is being used.   All future references will ignore
the \fIc\fP parameter by assuming the default value.
.PP
The \fIy\fP value specifies the device or drive unit to use.
The \fIz\fP value is interpreted differently for tapes and disks:
for disks, \fIz\fP is a partition number (0 thru 7) corresponding to partitions 
\'a\' thru \'h\' respectively, and should always be zero. For tapes, \fIz\fP 
is a file number on the tape.
.PP
In all simple cases, a drive with unit number 0 (determined either by
a unit plug on the front of the drive, or jumper settings on the drive
or controller) will be called unit 0 in its UNIX file name.
If there are multiple controllers, the drive unit numbers
will normally be counted within each controller.  Different controller models 
support different numbers of devices. On a controller that supports 8 drives, 
drives on the first controller are numbered 0 thru 7 and drives on the 
second controller are also numbered 0 thru 7.
Returning to the discussion of the standalone system, recall
that tapes also took two integer parameters.  In the case of a
TM03 tape controller with a TU77 as drive 0, the
files on the tape have names ``ht(0,0)'', ``ht(0,1)'', etc.
Here ``file'' means a tape file containing a single data stream
separated by a single tape mark.
.PP
.\" bp here to force the footnote onto a single page
.bp
Each UNIX physical disk is divided into 8 logical disk partitions,
each of which may occupy any consecutive cylinder\(dg range on the
.FS
.IP \(dg
We refer to "cylinders" as most supported physical disk drives use CHS 
(cylinder/head/sector) geometry. Newer physical disks such as MSCP and 
SCSI don't have traditional CHS geometry, it is still convenient (actually,
necessary) to refer to them with a synthetic CHS geometry.
.FE
physical device. The \fI/etc/disktab\fR file provides tested, standard
partition layouts.  While overlapping partitions are allowed, they are 
strongly discouraged, being an accident waiting to happen, as making one 
filesystem will destroy the other overlapping filesystem(s).
.PP
The cylinders occupied by the 8 partitions for each drive type are specified
by the disk label read from the disk.
.PP
If no label exists the disk will not be bootable or mountable as a filesystem, 
and while the kernel attempts not to damage unlabeled disks (by swapping to or 
doing a crash dump on a live filesystem) there is a chance that filesystem 
damage will result if a kernel tries to write to an unlabeled disk. For 
physical (as opposed to emulated) disk drives, we recommend using the Write 
Protect button (if available on the drive) or simply not "spinning up" disks 
that will not be accessed by the kernel to guarantee that data on those disks 
will not be corrupted. This is particularly important on systems that alternate 
between running 2.11BSD and some other operating system. 
.PP
The standalone \fBdisklabel\fP program is used to define the partition tables.
Each partition may be used
either as a raw data area (such as a swapping area) or to store a
UNIX filesystem.
It is mandatory for the first partition on a disk to start at sector offset 0
because the 'a' partition is used to read and write the label (which is at
the beginning of the disk).
If the drive is to be used 
to bootstrap a UNIX
system, the 'a' partition must be of type \fB2.11BSD\fP (FS_V71K in
\fIdisklabel.h\fP) and at least 4MB in size, with 8 to 10MB being strongly 
suggested.  A 'b' partition for swapping is also needed. The 'b' partition 
must be at least 0.5MB larger than the size of main memory so that in the 
event of a system crash the contents of main memory can be written to the 'b' 
partition. Since most emulated systems will be running with a full 4MB of 
memory configured, this works out to a recommended size of 4.5 to 5MB for 
the 'b' partition. The extra space is so that the system can be booted and 
begin running without overwriting the crash dump. If a drive is being used 
solely for data, then that drive need not have a 'b' (swap) partition but 
partition 'a' must still span the first part of the disk. The second ('b') 
partition is traditionally used as a swapping area, and the rest of the disk 
is divided into space (as described below) for additional ``mounted 
filesystems'' by use of one or more additional partitions.
.PP
The third ('c') logical partition of each physical disk also has a
conventional usage: it allows access to the entire physical device,
including the BAD144 bad sector forwarding information recorded at the end of
the disk (one track plus 126 sectors).  It is occasionally used to store
a single large filesystem or to access the entire pack when making a
copy of it on another.  Care must be taken when using this partition not
to overwrite the last few tracks and thereby destroying the bad sector
information.
.PP
On disks that support bad sector forwarding, the eighth ('h') partition 
traditionally spans the entire physical disk \fBexcept\fR for the last 
cylinder. It is generally preferable to use the 'h' partition (if it exists) 
instead of the 'c' partition when copying entire disks using bad sector 
forwarding for the reasons described in the previous paragraph.
.PP
On other disks, partition 'h' is either not defined at all, is defined as an
additional partition for user data (generally on larger-capacity disks), or 
is defined as in the above paragraph. This inconsistency is a result of new 
disk types added since the original release of 2.11BSD 35 years ago, the change
to on-disk labels instead of in-kernel labels, and so forth.
.PP
One reason to spread things out across multiple partitions (particularly on 
larger drives) is the 64K limit on the number of files (inodes). If you expect 
to have a large number of small files, you can end up with megabytes of 'free' 
space, but not have an available inode to create a file. 
.NH 2
UNIX devices: block and raw
.PP
UNIX makes a distinction between ``block'' and ``raw'' (character)
devices.  Each disk has a block device interface where
the system makes the device byte addressable and you can write
a single byte in the middle of the disk.  The system will read
out the data from the disk sector, insert the byte you gave it
and put the modified data back.  The disks with the names
``/dev/xx0a'', etc are block devices.
There are also raw devices available.
These have names like ``/dev/rxx0a'', the
``r'' here standing for ``raw''.
Raw devices bypass the buffer cache and use DMA directly to/from
the program's I/O buffers;
they are normally restricted to full-sector transfers.
In the bootstrap procedures we
will often suggest using the raw devices, because these tend
to work faster.
Raw devices are used when making new filesystems,
when checking unmounted filesystems,
or for copying quiescent filesystems.
The block devices are used to mount filesystems,
or when operating on a mounted filesystem such as the root.
.PP
You should be aware that it is sometimes important whether to use
the character device (for efficiency) or not (because it wouldn't
work, e.g. to write a single byte in the middle of a sector).
Don't change the instructions by using the wrong type of device
indiscriminately.
.PP
The standalone \fBdisklabel\fP program must be used to alter the 'a' and 'b'
partitions of a drive being used for a bootable system.  This is because
the kernel will not permit an open partition to change size or offset.
The root and swap partitions are \fBalways\fP open when the kernel is
running.
