.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)c.t	3.0 (2.11BSD) 2026/11/08
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Keeping your \*(2B system up-to-date
.nr H1 9
.nr H2 0
.ds RH "Appendix C \- keeping your 2.11BSD system up-to-date
.ds CF \*(DY
.bp
.LG
.B
.ce
APPENDIX C \- KEEPING YOUR 2.11BSD SYSTEM UP-TO-DATE
.sp 2
.R
.NL
.NH 2
PL499 and earlier
.PP
It is recommended (particularly if you are using an emulator, as hardware
is free) that you either perform a new install of the Patch Level 500 tape or 
install one of the "canned" (pre-built) Patch Level 500 disk images and then 
merge or copy your local changes.
.NH 3
Intentional breaking changes
.PP
In the 35 years since 2.11BSD was first released, there have been a number of 
"breaking changes", such as the introduction of longer file names, the 
conversion of the C compiler and system sources to ANSI C, and so on. Even if 
you have a 2.11BSD system correctly at an earlier patch level, the effort to
apply multiple patches (many of which have additional manual steps as 
described in the specific patch, so you can't just queue up a dozen or so patch
installations and come back to an updated system) likely does not justify 
incremental patching.
.NH 3
Incomplete or entirely missing patches
.PP
There have been a number of unofficial releases, both as distribution tapes as 
well as bootable disk images, where some patches were missed completely or the 
patch's post-installation tasks were partially or completely skipped.
.PP
This can be particularly annoying because (for example) an old file that 
should have been removed in a post-installation task may cause a subsequent 
patch to fail. It might not be the next patch, either - you could successfully 
install the next dozen or so patches, then run into problems as a new patch 
attempts to patch a file, but fails because a previous patch left the file in 
an incorrect state. You will likely spend more time trying to figure out what 
went wrong than you would have if you just installed Patch Level 500 from tape 
or one of the "canned" PL500 disk images.
.NH 3
C library routine prototypes
.PP
Many C library routines that were previously missing prototype declarations 
are now properly prototyped. This means that existing C source files that 
compiled on earlier 2.11BSD systems may generate new errors. These errors are 
usually caused by either using a variable name which conflicts with a function 
in the C library, or where the source file declared a C library function with 
a different prototype than the new C library declaration.
.NH 3
Recent changes to the system startup scripts
.PP
Patch Level 500 can leave you with different system startup scripts depending 
on how you got to PL500.
.PP
If you successfully upgraded to PL500 via patching (and not a fresh install) 
the system will leave the existing startup scripts unchanged. This is because 
these are among the most frequently customized parts of the system and 
automated conversion to the new style would likely be impossible.
.PP
If you performed a new install of Patch Level 500, you will find the startup 
settings have been centralized in \fI/etc/rc.conf\fR, more closely following 
what the other *BSD flavors do.
.PP
While both old-style and new-style startup scripts are supported on PL500,
future patch levels will likely only provide changes to the new-style startup. 
Despite this, if you are determined to manually patch your way to PL500, you 
can follow the instructions for PL500 and later, found below.   
.NH 2
PL500 and later
.PP
Once your system is at PL500, keeping it up-to-date should be much less work 
for you. The following provides a general overview of the installation of a new
Patch Level. Depending on what is changed in the new Patch Level, you will 
usually need to execute a post-installation script and/or type individual 
commands yourself.
.NH 3
Notification of new Patch Levels
.PP
New Patch Levels are often announced in a number of places. These announcements
are made by users, so they should be treated as "unofficial". The definitive 
way to check for new patches is at any of the following sites:
.DS
ftp://ftp.dfupdate.se/pub/pdp11/2.11BSD
https://www.tuhs.org/Archive/Distributions/UCB/2BSD/2.11BSD
ftp://ftp.2bsd.com/2.11BSD
http://www.2bsd.com/2.11BSD
.DE
.PP
Unofficial announcements may appear at:
.DS
The HECnet mailing list
The Vintage Computer Federation's DEC forum
The Google Groups PiDP-11 list
.DE
.NH 3
Determining the current Patch Level
.PP
While logged in to your 2.11BSD system, you can issue the command: \fIhead -2 
/VERSION\fP, which will report the current Patch Level and date. As of this 
release, that command would produce this output:
.DS
Current Patch Level: 500
Date: October 11, 2026
.DE
.PP
Each time a new Patch Level is installed, this file is updated. You should also
check the Patch Log using the command \fIcat /PATCH-LOG\fP. It will report the 
date each patch was installed, as well as an optional comment describing the 
patch. Note that this file contains the history of all patching on the system, 
showing the date a particular patch was installed (not the patch release date).
.PP
It is \fBimportant\fP to carefully examine this list to make sure there are no 
missing Patch Levels, and that they were all applied in the proper order. If 
you encounter a missing Patch Level, download the patch and look it over. 
Sometimes the missing one(s) can simply be applied out of sequence with no ill 
effects. In other cases, installing a missing one may fail due to system files 
not having the expected contents. The converse can happen as well, where an 
earlier missed Patch Level causes a more recent one to fail. Recovering from 
either of these situations will require careful examination of each missed or 
out of sequence one. This will likely require you to have detailed knowledge of
the internals of 2.11BSD. Or you can ask for assistance on any of the various 
forums or mailing lists where 2.11BSD is discussed.
.PP
A current \fI/PATCH-LOG\fP file will look like this:
.DS
Patch 500 applied Thu Sep 17 18:47:38 CDT 2026
System now at 2.11BSD 35th Anniversary Edition (PL500) Thu Sep 17 19:47:38 CDT 2026
.DE
.PP
A hypothetical example of a more complex file will look like this (note that 
this is a made-up example for this document - as of the writing of this 
document, Patch Level 501 has not been released).
.DS
Patch 500 applied Thu Sep 17 18:47:38 CDT 2026
System now at 2.11BSD 35th Anniversary Edition (PL500) Thu Sep 17 19:47:38 CDT 2026
Patch 501 applied Thu Apr  1 22:33:59 EST 2027
.DE
.NH 2
Installing a new Patch Level
.PP
Installing a new Patch Level involves determining which is the most recent one 
installed and making sure there are no missing ones, as discussed in the 
previous section. Then, download any Patch Level(s) newer than the one your 
system is currently at, and examine and install each one in order. They are 
distributed as human-readable files (the filename will contain the particular 
Patch Level number), and you can view them using the \fImore\fR paging utility. 
The following describes the patching process. \fBIMPORTANT:\fR This is an 
example only. If you are starting with a PL500 system, all patches through 
PL500 have already been applied.
.NH 3
Back up your existing system (optional, but strongly recommended)
.PP
Installing patches can involve "sawing off the branch you're standing on" if 
you make a typo, miss a step, or execute a step out of order. Therefore, we 
strongly recommend that you back up your 2.11BSD disk(s) before installing any 
patches. If you are running 2.11BSD under an emulator, this can be as easy as 
shutting down the emulated PDP-11 system and then making copies of the disk 
container file(s). We suggest that you include the current date or other 
descriptive information in the backup file name(s), then restart the 
emulator and install the patch(es). If you are running on an actual PDP-11, 
backup procedures could involve physical disk-to-disk copy processes 
(mirroring the procedure when using an emulator), backing up to a tape drive, 
and so on. Due to the large number of possible PDP-11 configurations, it is 
impossible to document all possible cases here. However, if you are lucky 
enough to have an actual PDP-11 system, you are likely familiar with backup 
procedures already.
.NH 3
Viewing the patch description
.PP
As explained above, each Patch Level file is distributed as a plain text file. 
Each file begins with a Subject: line which explains what the file fixes or 
improves, and includes the Patch Level on that line. Next is an Index: line 
which (unless you are interested in 2.11BSD internals), you can ignore.
.PP
Next is a Description: section which provides additional details on what the 
file addresses (expanding on the brief Subject: line mentioned above). That is 
followed by the Repeat-By: section which discusses a way to reproduce the 
problem. The Repeat-By: section will probably also be unimportant to most 
users.
.PP
This is followed by the Fix: section which provides actual instructions to 
install the file. This will normally include the statement "Cut where indicated
and save to a file...". This points out the "---cut here---" line, which 
separates the instructions from the actual patch.
.PP
Some patch files are small, and you can just copy or screenshot the 
instructions. Other patches are quite large and we suggest that you use the
following commands to extract the instruction part of the patch from the file.
\fIxxx\fR should be replaced with the actual patch number (for example, 501) 
which should have already been placed in the \fI/usr/tmp\fR directory:
.PP
.nf
.in 0
.cs CW 20
.ft CW
# Optional: To output the text up to (but not including the cut here line)
# for easy viewing or printing:
#
cd /usr/tmp
sed -e '/^---*cut here---.*/,$d' < xxx > xxx.txt
.fi
.NH 3
Installing the patch
.PP
At the end of the Fix: section, you'll find a line that ends with Then:,
followed by the instructions actually needed to install the patch. You should 
follow the instructions \fBexactly\fR, except in cases where an instruction 
says something like "MYSYSTEM (replace with your actual kernel's name)". 
These instructions generally include using a utility to extract the main
patch contents, followed by steps you need to take after that. A Patch Level
is not successfully installed until \fBall\fR of the listed steps have been 
performed. \fBCAUTION: \fRMissing one or more steps, or performing them out
of sequence can leave your system in (at a minimum) a state where future Patch 
Levels will not apply properly due to errors made when installing a prior patch.
.NH 3 
Sample patch file
.PP
The following is an example of a patch file. Most of the material after the 
"---cut here---" line has been replaced with "[snip]" to keep this example 
brief.
.PP
.nf
.in 0
.cs CW 20
.ft CW
.so 494-example
.fi
