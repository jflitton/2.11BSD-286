.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)0.t	3.0 (2.11BSD) 2026/11/08
.\"
.EH 'setup.2.11 - %''Installing and Operating 2.11BSD on the PDP-11'
.OH 'Installing and Operating 2.11BSD on the PDP-11''Setup.2.11 - %'
.ds Ps 2.10BSD
.ds 1B 2.10.1BSD
.ds 2B 2.11BSD
.bd S B 3
.TL
Installing and Operating \*(2B on the PDP-11
.br
35th Anniversary Release (Patch Level 500)
.br
November 13, 2026
.AU
Steven Schultz
.AI
sms@2bsd.com
.br
 
.br

With additional contributions by:
.AU
Johnny Billquist
.AI
bqt@softjar.se
.AU
Terri Kennedy
.AI
terri-decus@glaver.org
.de IR
\\fI\\$1\|\\fP\\$2
..
.de UX
UNIX\\$1
..
.AB
.PP
\" DEC VAX, IDC and SBI are never referenced again; removed
\" The rest are abandoned per USPTO 8/2026
\" UNIX verified USPTO 8/2026
.FS
.IP \(dg
DEC, PDP-11, UNIBUS and MASSBUS were formerly trademarks (now abandoned) of
Digital Equipment Corporation.
.FE
.FS
.IP \(dd
UNIX is a trademark of The Open Group Limited
.FE
This document contains instructions for the
installation and operation of the \*(2B PDP-11\(dg
.UX \(dd
system.
.PP
It discusses procedures for installing \*(2B UNIX on a PDP-11, including
explanations of how to allocate space for filesystems on one or more disks, 
how to set up terminal lines and user accounts, how to do system-specific 
tailoring, and how to install and configure the networking facilities.  
Finally, the document details system operation procedures: shutdown and 
startup, hardware error reporting and diagnosis, filesystem backup procedures,
resource control, performance monitoring, and procedures for recompiling
and reinstalling system software.
.PP
The ``bugs'' address supplied with this release
will work for some unknown period of time; make sure
the ``Index:'' line of the bug report indicates that the release is
``\*(2B''.  See the \fIsendbug\fP(8) program for more details.
.PP
Updates are available at the following locations in the Patches subdirectory:
.DS
.TS
lw(1.5i) l.
ftp://ftp.dfupdate.se/pub/pdp11/2.11BSD
https://www.tuhs.org/Archive/Distributions/UCB/2BSD/2.11BSD
ftp://ftp.2bsd.com/2.11BSD
http://www.2bsd.com/2.11BSD
.TE
.DE
.PP
You can determine the current patch level of a running \*(2B system with the 
following command: \fIhead -2 /VERSION\fP
.PP
Updating instructions can be found in Appendix C.
.AE
