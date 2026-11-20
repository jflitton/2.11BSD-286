.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)a.t	3.0 (2.11BSD) 2026/11/09
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 7
.nr H2 0
.ds RH "Appendix A \- configuring and building a new kernel
.ds CF \*(DY
.bp
.LG
.B
.ce
APPENDIX A \- CONFIGURING AND BUILDING A NEW KERNEL
.sp 2
.R
.NL
.PP
The \*(2B kernel has a number of parameters and options that can be
used to tailor the kernel to site specific needs.
This appendix lists the parameters and options used in the kernel.  The
parameters have numeric values, usually table sizes.  The options flags
are either defined or undefined (via the values YES or NO respectively.)
.PP
Once you have identified all of your desired options, this appendix describes 
how to build, install, and test your new kernel.
.NH 2
Configuring the number of mountable filesystems (NMOUNT)
.PP
Because of time constraints the
.B NMOUNT
constant was not moved into the kernel configuration file where it belongs.
.B NMOUNT
is used to configure the number of mountable filesystems in \*(2B.  Since
each slot in the kernel mount table takes up close to a half KB of
valuable kernel data space, the distribution kernel comes configured
with
.B NMOUNT
set to 6.
This may be too small for systems using multiple disk drives and should be 
increased to the number of filesystems you expect to mount.
.PP
.B NMOUNT
is defined in
.IR /sys/h/param.h .
If you change its value, you must recompile the kernel (obviously) and the
following applications:
.IR mount ,
.IR quotaon ,
.IR edquota ,
.IR umount ,
and
.IR df .
.NH 2
Configuring automatic boot and restart
.PP
\fBNOTE:\fR The following only applies to the distribution tape - the "canned" 
(pre-built images) have automatic boot and restart configured.
.PP
The distribution tape contains a configuration which will wait for user input 
(normally a carriage return) before loading the kernel, in order to allow a 
different kernel to be specified. Likewise, after the system is in single user 
mode a Control-D (^D) is needed to switch into multi user mode. There are two 
settings that control this behavior.
.NH 3
Setting BOOTDEV
.PP
The first is the kernel configuration setting "BOOTDEV". As shipped, it is set 
to "NONE". To enable automatic restart and switching to multi user mode after a 
"shutdown -r" command, this should be set to match the type of disk you are 
using. For example, "rm" if you are booting from an RM or RP disk.
.NH 3
Configuring AUTOMULTIUSER
.PP
The second setting controls what happens when the system is "cold booted". As 
shipped, this causes the system to wait for the user to either press the return
key (for the default \fI/unix\fR kernel) or to enter the name of an alternate 
kernel and then press the return key.
.PP
You can override this behavior and instead cause a five-second countdown 
before the default kernel is loaded. At any time during those five seconds, 
the user can either press the return key to boot the default kernel, or any 
other key to be prompted to enter the name of an alternate kernel. To enable 
this option, perform the following steps:
.DS
\fB#\fP cd /sys/pdpstand
[edit the file boot.c and change the definition of AUTOMULTIUSER from "0" to "1" and save the file.
\fB#\fP make boot (ignore some harmless warning messages)
\fB#\fP install -s -m 744 boot /
\fB#\fP make clean
.DE
.PP
After this, a "cold boot" will display a prompt as shown below:
.DS
Press <CR> to boot, or any other key to abort: 5 4 3 2 1 0
.DE
.NH 2
Kernel organization
.PP
On the distribution tape, the kernel source is in a 
separate tar image. On "canned" (prebuilt) disk images, 
the kernel source has already been loaded into the disk
image. When installing from tape, the source may be physically
located anywhere within any filesystem so long as
a symbolic link to the location is created for the
file /sys
(many files in /usr/include are normally symbolic links
relative to /sys).  In further discussions of the
system source all path names will be given relative to
/sys.
.PP
The directory /sys/sys
contains the mainline machine independent
operating system code.
Files within this directory are conventionally
named with the following prefixes:
.DS
.TS
lw(1.0i) l.
init_	system initialization
kern_	kernel (authentication, process management, etc.)
quota_	kernel portion of disk quota system
subr_	misc. subroutines used throughout the kernel
sys_	system calls and the like
tty_	terminal handling
ufs_	filesystem
uipc_	interprocess communication
vm_	memory management
.TE
.DE
.PP
The remaining directories are organized as follows:
.DS
.TS
lw(1.0i) l.
/sys/h	machine independent include files
/sys/conf	site configuration files and basic templates
/sys/net	network independent, but network related code
/sys/netinet	DARPA Internet code
/sys/netimp	IMP support code
/sys/netns	Xerox NS support code
/sys/pdp	PDP-11 specific mainline code
/sys/pdpif	PDP-11 network interface code
/sys/pdpuba	PDP-11 UNIBUS, MASSBUS and QBUS device drivers
.TE
.DE
.PP
Many of these directories are referenced through /usr/include with
symbolic links.  For example, /usr/include/sys is a symbolic
link to /sys/h.  The system code, as distributed, is mostly
independent of the include files in /usr/include.  Unfortunately, 
not all references to /usr/include have been eradicated, so
compiling the system requires the /usr filesystem to be mounted.
.NH 2
Devices and device drivers
.PP
Devices supported by UNIX are implemented in the kernel
by drivers whose source is in /sys/pdp and /sys/pdpuba. 
These drivers are compiled
into the kernel when included in a CPU-specific configuration file
kept in the conf directory.
.PP
\fBNOTE:\fR If you are using a "canned" (prebuilt) disk image, you can safely 
skip the sections on /dev, /etc/dtab, and /etc/fstab below, as they have been 
set up correctly for the image you are using. If you add any devices to a 
"canned" image, you may need to make changes to those files as described below.
.NH 3
Populating the /dev tree
.PP
Devices are accessed through special
files in the filesystem, made by the
.IR mknod (8)
program and normally kept in the /dev directory.
For all the devices supported by the distribution system, the
files in /dev are created by the /dev/MAKEDEV
shell script.
.PP
Determine the set of devices your kernel will support, then
create a new directory
/newdev, copy /dev/MAKEDEV into it, edit the file MAKEDEV.local if needed 
to provide an entry for local needs,
and run /newdev/MAKEDEV to populate the /newdev directory.
For instance, if your machine has a single DZ11, a single
DH11, an RM03 disk, an EMULEX UNIBUS SMD disk controller with an
AMPEX 9300 disk, and a TE16 tape drive you would do:
.DS
\fB#\fP cd /
\fB#\fP mkdir newdev
\fB#\fP cp dev/MAKEDEV newdev/MAKEDEV
\fB#\fP cd newdev
\fB#\fP ./MAKEDEV dz0 dh0 xp0 xp1 ht0 std LOCAL (LOCAL only if needed)
.DE
.PP
Note that the ``std'' argument causes standard devices
such as \fI/dev/console\fP (the machine console), \fI/dev/null\fP,
\fI/dev/tty\fP, \fI/dev/klog\fP, etc.
to be created.
.PP
You can then do
.DS
\fB#\fP cd /
\fB#\fP mv dev olddev ; mv newdev dev
\fB#\fP sync
.DE
.PP
to install the new device directory.\(dg
.FS
.IP \(dg
You must reboot your system before you can remove the /olddev directory.
.FE
As distributed almost all of
the device nodes are already present and you may wish to remove
unused entries from /dev to speed up scanning of the directory.
The terminal nodes are almost certainly incorrect for your site and
will need to be deleted and recreated.  Directly connected terminals
should have the softcarrier bit on in their minor device numbers.  Since
MAKEDEV by default creates terminal (dh, dz, etc) nodes with the softcarrier
bit off you will have to delete those nodes and recreate those terminal
nodes which are directly connected:
.DS
\fB#\fP cd /dev
\fB#\fP rm ttyh0
\fB#\fP mknod ttyh0 c 3 128
.DE
.PP
You can create the device nodes (if needed) before or after building a new 
kernel.
.PP
\fBNOTE:\fR The need to use MAKEDEV and mknod to make device nodes 
for most devices will likely come as a surprise to users of modern Unix-like 
systems, which automatically populate the /dev directory. This is because 
2.11BSD would need additional initialization code to create and populate the 
/dev directory, taking up scarce memory for a function that is only rarely 
needed.
.NH 3
Updating the /etc/dtab file
.PP
In addition to the device nodes in the /dev directory, the kernel uses a file 
called /etc/dtab to attach device drivers to their specific controller 
address.
.PP
The PDP-11 has a complex system of addresses (sometimes called CSRs) and vectors 
that are set on each device so the rest of the system knows where to find that 
device. To complicate matters further, some devices use a "fixed" (unchanging) 
address and vector, others use "floating" addresses and vectors, and yet others
use a fixed address and vector for the first device of that type and a floating 
address and vector for additional devices of that type. Some (relatively) newer
devices allow the operating system to specify their vector; those devices do 
not have a vector that is configured in hardware. Additionally, if you later 
install a device with a floating address, it may require changing the addresses
of other devices. This probably seems very bizarre to modern users accustomed 
to plugging an add-on card into a PC and having it "just work". Bear in mind 
that the first PDP-11 model was introduced in 1970.
.PP
If you are running on an emulator, the emulator likely has a command to display
this information. For example, on SIMH (with entries for disabled devices not 
shown):
.nf
.in 0
.cs CW 20
.ft CW

sim> show dev
PDP-11 simulator configuration

CPU     11/70, FPP, autoconfiguration enabled, idle enabled
SYSTEM  PDP-11 model options
RHA     address=17776700-17776753, vector=254, BR5, RH70
RHB     address=17772440-17772477, vector=224, BR5, RH70
CLK     60Hz, address=17777546-17777547, vector=100, BR6
PTR     address=17777550-17777553, vector=70, BR4
PTP     address=17777554-17777557, vector=74, BR4
TTI     7p, address=17777560-17777563, vector=60, BR4
TTO     7p, address=17777564-17777567, vector=64, BR4
LPT     address=17777514-17777517, vector=200, BR4
VH      address=17760440-17760457*, vector=300-304*, BR4, lines=16
RP      Massbus adapter 0 (RHA), 8 units
TU      Massbus adapter 1 (RHB), FORMATTER, TM03, 8 units
XU      address=17774510-17774517, vector=120, BR5, MAC=08:00:2B:A8:02:0B
ROM     4 units
.fi
.PP
If you are running on an actual PDP-11, the commands differ by model, and to 
make things more complicated, some models (like the PDP-11/70) are "open it up 
and look", at least until you have an operating system running. Newer models 
(you can tell these by their "chatty" console messages) normally have a command
like "map" which will display the contents of the I/O address space.
.PP
The /etc/dtab file needs to be edited to match the actual hardware 
configuration. Here is an /etc/dtab file that matches the above configuration 
(most lines that are commented out are not shown):
.nf
.in 0
.cs CW 20
.ft CW

# Device Configuration Table

#Name Unit# Addr Vector Br      Handler(s)      # Comments

du      ? 160440 300    5       dhurint dhuxint # dhu11 terminal mux
ht      ? 172440 224    5       htintr          # tu 16 massbus tape
lp      ? 177514 200    4       lpintr          # lp-11 line printer
xp      ? 176700 254    5       xpintr          # xp driver
.fi
.PP
You will need to uncomment lines for your devices and specify their address and 
vector. You don't need to worry about the rest of that device line. It is a 
good idea to comment out lines for any devices you do not have configured in 
your kernel. There are a few devices that appear in the SIMH device list but 
not in the /etc/dtab file. CLK/TTI/TTO are the system clock and console serial 
port - the kernel knows where these are because their addresses and vectors 
are the same on all PDP-11 models. PTR/PTP are the high-speed paper tape 
reader and punch, respectively. The hardware is there, but this kernel does 
not include support for them. Lastly, networking devices such as XU (SIMH)/de 
(2,11BSD) device are not enabled in the /etc/dtab file. They appear as 
commented-out devices to provide a reference for the expected addresses and 
vectors.
.PP
\fBNOTE:\fR Device naming is not consistent - the PDP-11 console or emulator 
will refer to them with one set of names, while 2.11BSD may use different 
names. This is true of other PDP-11 operating systems as well. It's just the 
way things were back then.
.PP
You may be wondering why the system has both an /etc/dtab file and a /dev
directory, because they seem to be talking about the same type of thing. The 
answer is that /etc/dtab is used by the kernel to find hardware and attach the 
correct device drivers, while the files in /dev are used by programs to talk to
the devices via the standard I/O library and kernel.
.NH 3
Updating /etc/fstab (if needed)
.PP
If you added additional disk devices to your configuration, you will need to 
add them to the /etc/fstab file so they are automatically mounted during system
startup. \fBNOTE:\fR Refer to Section 7.1 above regarding adjusting NMOUNT (if 
needed).
You can only mount as many filesystems as NMOUNT allows. For example, consider 
the following /etc/fstab:
.nf
.in 0
.cs CW 20
.ft CW

/dev/xp0a       /       ufs     rw              1       1
/dev/xp0b       none    swap    sw              0       0
/dev/xp0g       /usr    ufs     rw              1       2
.fi
.PP
This is a bit of a "trick question". /dev/xp0b is used as a swap device,
so it doesn't consume a mount point, while /dev/xp0a and /dev/xp0g do. In this 
case, you have four more available mount points with the default NMOUNT setting
of 6.
.NH 2
Building a new kernel
.PP
The kernel configuration of each UNIX system is described by
a single configuration file, stored in the \fI/sys/conf\fP directory.
The format of this file is very simple consisting of lines starting with
an \fIidentifier\fP followed by a \fIvalue\fP.  Blank lines and anything
past a ``#'' (including the #) are comments.  This file is processed by the
shell script \fIconfig\fP in the same directory.
The manual pages in Section 4 of the UNIX man pages specify the configuration
lines necessary for various devices.
.PP
The configuration file \fIGENERIC\fP in the conf directory was used to
build the generic distribution kernel.  To build a local configuration
file, copy \fIGENERIC\fR to a new file \fIMYSYSTEM\fP, edit \fIMYSYSTEM\fP 
for your desired system configuration, and then type "./config \fIMYSYSTEM\fP". 
This will create the directory \fI../MYSYSTEM\fP and copy specially edited 
files into based on the definitions in \fIMYSYSTEM\fP.  Change directory into 
the new system directory and type \fImake all\fR.\(dg
.FS
.IP \(dg
If your system does not have a floating-point processor (this only applies to 
the PDP-11/44 and /70) you will need to enable the floating-point emulator via 
the FPSIM kernel configuration option, as only the GENERIC kernel has floating 
point emulation installed. If you do not enable FPSIM, your new kernel will 
hang after printing the memory sizes. This restriction will likely be lifted in
a future Patch Level.
.FE
.PP
Prototypes for all configurable options can be found in the generic
kernel configuration file \fI/sys/conf/GENERIC\fP. You can treat the items 
copied from GENERIC as a ``grocery list'', checking off those options you want,
crossing out those you don't and setting numeric parameters to reasonable
values. A An additional kernel configuration file, /sys/conf/EXAMPLE is also 
provided, showing the changes needed to enable networking support. The exact 
steps are shown below:
.DS
\fB#\fP cp GENERIC \fIMYSYSTEM\fP
\fB#\fP TERM=\fIterminal_type\fP; export TERM
\fB#\fP vi \fIMYSYSTEM\fP (or whatever your favorite editor is)
\fB#\fP ./config \fIMYSYSTEM\fP
\fB#\fP cd ../\fIMYSYSTEM\fP
\fB#\fP make
.DE
.PP
Note that the overlay scheme in the Makefile copied into the new system
directory may fail because either the \fIbase segment\fP is too small, too
large or one or more \fIoverlay segments\fP are too large.  If this
happens the system object files will have to be rearranged in the \fIbase\fP
and \fIoverlay\fP segments.  The comments in the Makefile should make it
fairly clear what the restrictions on object placement are in the system.
.PP
The following is an example of the display the Makefile produces when 
reporting the size of the base and overlay segments:
.PP
.nf
.in +0.5i
.cs CW 20
.ft CW
size unix
text    data    bss     dec     hex
57088   5844    34182   97114   17b5a   total text: 108928
        overlays: 7808,7488,8128,7296,2240,7744,5440,5696
.fi
.PP
Each of the overlay segments must be kept to under 8KB in size and you cannot 
have a zero-size overlay between two occupied overlays. You can use
the \fIsize\fR command on a file (use the "text" number returned by that 
command) to see how much space a given object file would add to or remove 
from an overlay when editing the Makefile. Be careful and try to move object 
files between overlays, rather than moving things from the base to an 
overlay - many object files in the base need to remain there. \fBNOTE:\fR 
Each time you use the \fIconfig MYSYSTEM\fR command, the Makefile will be 
overwritten. If your kernel required editing the Makefile to rearrange the 
overlays, you should keep track of those changes, as you will need to re-apply 
them to the new Makefile.
.PP
The configured system image ``\fIunix\fR''\(dd
.FS
.IP \(dd
On networked systems there are two images, \fIunix\fP and \fInetnix\fP.
.FE
should be
copied to the root, and then booted to try it out.
It is best to first save the old kernel file(s) to a known name so as not to 
break the working system until you're sure the new one does work.  It is a
\fBbetter\fP idea to also always keep the generic non network kernel (renamed 
to \fI/genunix\fR) on the system. On "canned" (prebuilt) disk images, the
\fI/genunix\fR kernel has already been installed for you.
.PP
For kernels built without networking, the steps would be:
.DS
\fB#\fP cp /unix /oldunix
\fB#\fP make install
\fB#\fP sync
.DE
.PP
For kernels built with networking, the steps would be:
.DS
\fB#\fP cp /unix /oldunix
\fB#\fP cp /netnix /oldnetnix
\fB#\fP make install
\fB#\fP sync
.DE
.PP
\fBIMPORTANT NOTE: \fR\fI/unix\fR and \fI/netnix\fR are a "matched set" and 
should be treated as such. A mismatch will lead to a crash at system boot 
time, as will a \fI/unix\fR built with networking, but missing its matched 
\fI/netnix\fR. For these reasons, we suggest 
always keeping the GENERIC distribution kernel as \fI/genunix\fR - it will 
work on most hardware and does not depend on networking. After you boot 
\fI/genunix\fR, you can either fall back to your previous kernel (by copying 
the \fI/old*nix\fR files back to the expected names) or perform other recovery 
tasks as may be needed.
.PP
\fBNOTE:\fR When booting from a kernel that is not 
named \fI/unix\fR (for example, \fI/genunix\fR) you will see a pair of console 
messages like these:
.DS
autoconfig: /unix is not the running version.
init: configuration setup error
.DE
.PP
This is just to warn you that you are in a recovery environment and is 
otherwise harmless.
.PP
It is advisable to reboot the system relatively soon after installing a new
kernel, otherwise various utilities that read kernel memory will be using an
incorrect kernel symbol table and can produce error messages or other 
unexpected results:
.DS
\fB#\fP shutdown -r \fItime\fR
.DE
.PP
If you are the only user on the system and have no other software running, you 
can use 'now' for \fItime\fR. If there are other users on the system, it is 
polite to give them some time to save their work and log off before rebooting.
.NH 2
GENERIC kernel configuration
.PP
The GENERIC kernel supports the following devices:
.TS
l n.
Device	Number
_
RK05	8
RK06/07	2
MSCP (RA) Controllers	2
MSCP (RA) Disks	3
RL01/02 Drives	4
SMD (XP) Controllers	1
SMD (XP) Disks	2
TE16, TU45, TU77 (HT) Tape drives	2
TM11 (TM) Tape drives	2
TS11 (TS) Tape drives	2
TK50/TU81 (TMSCP) Tape drives	2
.TE
.PP
The generic kernel adapts automatically to the booted device.  The `a'
partition on the booted device is automatically made the root filesystem
and the `b' partition the swap area. The size of the swap partition is 
determined at run time, the kernel queries the driver for the number of blocks 
in the `b' partition.  \fBNOTE:\fP If the swap partition is not labeled as 
being of type \fIswap\fP the kernel will panic.
.NH 3
GENERIC kernel configuration file
.PP
.nf
.in 0
.cs CW 20
.ft CW
.so GENERIC.out
.fi
.NH 2
EXAMPLE kernel configuration file with networking
.PP
This is an example kernel configuration, showing the necessary changes to
customize the GENERIC kernel for specific hardware, add networking, and
so on.
.NH 3
EXAMPLE kernel configuration file with networking
.PP
.nf
.in 0
.cs CW 20
.ft CW
.so EXAMPLE.out
.fi
.cs R
