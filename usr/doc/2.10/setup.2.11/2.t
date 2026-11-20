.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)2.t	3.0 (2.11BSD) 2026/11/08
.\"
.ds lq ``
.ds rq ''
.ds LH "Installing/Operating \*(2B
.ds RH Installation
.ds CF \*(DY
.bp
.nr H1 2
.nr H2 0
.bp
.LG
.B
.ce
2. INSTALLATION PROCEDURE
.sp 2
.R
.NL
.PP
This section explains the bootstrap procedure that can be used
to get the kernel supplied with this distribution running on your PDP-11, 
regardless of whether it is an emulator or an actual PDP-11 system.
.PP
If you are installing this distribution on a system running the SIMH emulator,
the supplied simh.ini should be enough to get you started. If you are running 
on another emulator, you will need to attach the tape (it is in SIMH TAP 
format) and one or more destination disk container files. If you are installing
this on a real, working, PDP-11 system, you will need to write the supplied 
tape image to an actual tape which is compatible with the tape drive in your 
PDP-11 system.
.PP
.NH 2
Booting from tape
.PP
The tape bootstrap procedure used to create a
working system involves the following major
steps:
.\" This has manual line breaks (I HATE *roff). Might be ugly in non-PDF form.
.DS
1) Load the tape bootstrap monitor.
2) Create the partition tables on the disk using \fIdisklabel\fP. This is described in detail in Appendix B.
3) Create a UNIX ``root'' filesystem system on disk using \fImkfs\fP\|(8).
4) Restore the full root filesystem using \fIrestor\fP\|(8).
5) Boot the UNIX system on the new root filesystem and copy the appropriate \fIsector 0 boot block\fP
     to your boot device.
6) Build and restore the /usr filesystem from tape with \fItar\fP\|(1).
7) Restore the kernel sources from tape.
8) Restore the remaining sources from tape.
9) Tailor a version of UNIX to your specific hardware. Refer to Appendix A for detailed informa-
     tion.
.DE
.PP
Certain of these steps are dependent on your hardware
configuration.  If your disks require formatting, standard DEC
diagnostic utilities will have to be used, they are not supplied on the
\*(2B distribution tape. If you have a system with small-capacity disk 
drives, you may need to perform some or all of steps 6 thru 8 on additional 
disk drives. 
.PP
It is a good idea to locate your disk model in the 
table in Appendix B and use those values when setting up your partitions. 
All of the supported disks are listed in that file. Alternatively, you can
manually calculate the partition layout you will use with the \fIdisklabel\fR 
Calculating partition sizes in cylinders (even if your hardware is MSCP or an 
emulator that doesn't use cylinders) instead of sectors is helpful because 
you're working with smaller numbers and less likely to make a math error. You 
can write this info down or store it in a text file and desk-check your math. 
\fIdisklabel\fR will detect many errors (such as overlapping partitions or 
partitions extending past the end of the disk), but it is not guaranteed to 
catch all possible errors.
.NH 3
Step 1: Loading the tape bootstrap monitor
.PP
To load the tape bootstrap monitor, first
mount the magnetic tape on drive 0 at load point, making
sure that the write ring is not inserted if you are using a physical tape.
Then use the normal bootstrap ROM, console monitor, emulator boot command, or 
other bootstrap to boot from the tape.
.PP
\fBNOTE:\fR The boot blocks expect the CSR of the booting controller in r0
and the unit number in r1.  \fBboot\fP may be booted from any controller
or unit, the earlier restrictions of controller 0 and unit 0 have been
lifted.
.PP
If no other means are available, the following code can be keyed in
and executed at (say) 0100000 to boot from a TM tape drive (the magic number
172526 is the address of the TM-11 current memory address register;
an adjustment may be necessary if your controller is at a nonstandard
address):
.DS
.TS
l l.
012700	(mov $unit, r0)
000000	(normally unit 0)
012701	(mov $172526, r1)
172526
010141	(mov r1, -(r1))
012741	(mov $60003, -(r1))
060003	(if unit 1 use 060403, etc)
000777	(br .)
.TE
.DE
.PP
A toggle-in routine which has been used with a TS tape drive (this should
be entered at 01000):
.DS
.TS
l l.
012700	mov $unit,r0
000000
012701	mov $172522,r1
172522
005011	clr (r1)
105711	1b:tstb (r1)
100376	bpl 1b
012761	mov $setchr,-2(r1)
001040
177776
105711	2b:tstb (r1)
100376	bpl 2b
012761	mov $read,-2(r1)
001060
177776
000000	halt
140004	setchr: TS_ACK|TS_CVC|TS_SETCHR
001050	char
000000	high order address
000010	number of bytes
001070	char: status
000000
000016
000000
140001	read: TS_ACK|TS_CVC|TS_READ
000000	low order of address
000000	high order of address
001000	number of bytes to read
000000	status:
.TE
.DE
.PP
When this is executed, the first block of the tape will be read into memory.
Halt the CPU and restart at location 0.  The register \fBr1\fP \fBMUST\fP
be left pointing at the device \fIcsr\fP.  For the default/first TM or TS
this is 0172522.  The register \fBr0\fP \fBMUST\fP contain the unit number
(usually 0).
.PP
The console should type
.DS
.B
\fInn\fPBoot from \fIxx\fP(\fIctlr\fP,\fIdrive\fP,\fIpart\fP) at \fIcsr\fP
:
.R
.DE
where \fInn\fP is the CPU type on which it believes it is running.
The value will be one of 44, 70, 73, 83, 84, 93
or 94 depending whether a UNIBUS map and other hardware features are detected.
For J11 systems the System Maintenance Register is examined to determine
the CPU type.  At present \*(2B runs on the 44, 53, 70, 73, 83, 84, 93 and 94
\fBonly\fP.  It must be emphasized that \*(2B requires a CPU with separate I/D 
space.
.PP
\fIctlr\fP is the controller number that \fBBoot\fP
was loaded from.  It is 0 unless booting from a non-standard CSR.
.PP
\fIdrive\fP is the drive unit number.
.PP
The \fIpart\fP
number is disk partition or tape file number booted from.  This will always
be 0 for the tape \fBBoot\fP program.
.PP
\fIcsr\fP is an octal number telling the CSR of the controller from which
\fBBoot\fP was loaded.
.PP
You are now talking to the tape bootstrap monitor.
At any point in the following procedure you can return
to this section, reload the tape bootstrap, and restart.
Through the rest of this section,
substitute the correct disk type for \fIdk\fP
and the tape type for \fItp\fP.
.NH 3
Step 2: Creating the disk label
.PP
The standalone \fIdisklabel\fP program is then run:
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,1)	(\fIdisklabel\fP is tape file 1)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
\fBdisklabel\fP
\fBDisk?\fP \fIdk\|\fP(0,0)	(drive 0, partition 0)
d(isplay) D(efault) m(odify) w(rite) q(uit)?
 ...
\fB:\fP	(back at tape boot level)
.TE
.DE
.PP
The \fIdisklabel\fP program is meant to be fairly intuitive.  When prompted
with a line of choices entering the key just before the left parenthesis
selects the entry.  
.PP
If there is an existing label present on \fIdk\fP\|(0,0) it will be 
used as the default.  To have \fIdisklabel\fP create a new default based
on its idea of what the drive is select \fBD\fP.  Then enter \fBm\fP to
modify/edit the label.
.PP
The MSCP driver is quite good at identifying drives because it can query the
controller.  Other drivers (notably the SMD (\fBxp\fP) driver) have to
deal with a much wider range of controllers which do not all have the
same capabilities for drive identification.  When dealing with SMD
drives you must know the geometry of the drive so you can verify  and
correct \fIdisklabel\fP's choices.
.PP
You can however, if using non-DEC SMD controllers, make things easy for
\fIdisklabel\fP to determine what type of drive is being used.  If your
controller offers the choice of RM02 emulation you should select that choice.
The standalone \fBxp\fP driver uses RM02 as the indication that drive 
identification capabilities
not offered by DEC controllers are present.  The driver will be able to
determine the geometry of the drive in this case.   This is \fBoptional\fP
because you can explicitly specify all of the parameters to the standalone
\fIdisklabel\fP program.
.PP
A full description of the standalone \fIdisklabel\fP program is in Appendix
B of this document.
.NH 3
Step 3: Creating a UNIX ``root'' filesystem
.PP
Now create the root filesystem using the following procedure.
.PP
The size of the root ('a') filesystem was assigned in Step 2 (creating the 
disk label).  \fImkfs\fP will not allow a filesystem to be created if there
is not a label present or if the partition size is 0.  \fImkfs\fP 
looks at partition 0 ('a') in the disklabel for the root filesystem size.
.PP
Finally, determine the proper interleaving factors \fIm\fP and \fIn\fP
for your disk.  Extensive testing has demonstrated
that the choice of \fIm\fP is non critical (performance of a filesystem
varying only by 3 to 4% for a wide range of \fIm\fP values).  Values for
\fIm\fP within the range from  2 to 5 give almost identical performance.
Increasing \fIm\fP too much actually causes degraded performance because
the free blocks are too far apart.  Slower processors (such as the 73 and
44) may want to start with a \fIm\fP of 3 or 4, faster processors (such as the
70 and 84) may start with a \fIm\fP of 2 or 3.
On the other
hand, the \fIn\fP value is moderately important.  It should be the number
of filesystem blocks contained by one cylinder of the disk, calculated
by dividing the number of sectors per cylinder by 2, rounding down if
needed.  (This is what \fImkfs\fP does by default, based on the
geometry information in the disk label.)
These numbers determine the layout of the free list that will be constructed;
the proper interleaving will help increase the speed of the filesystem.
.PP
The number of bytes per inode determines how many inodes will be allocated
in the filesystem.  The default of 4096 bytes per inode is normally enough
(resulting in about 2000 inodes for an 8mb root filesystem and 1000 inodes
for the 4mb distribution ``generic'' root filesystem).  If more inodes are
desired then a lower value (perhaps 3072) should be specified when prompted
for the number of bytes per inode.
.PP
Then run the standalone version of the \fImkfs\fP (8) program.
The values in square brackets at the size prompt is the default from 
the disklabel.  Simply hit a return to accept the default.  \fImkfs\fP
will allow you to create a smaller filesystem but you cannot enter a
larger number than the one in brackets.
In the following procedure, substitute the correct types
for \fItp\fP and \fIdk\fP and the size determined above for \fIsize\fP:
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,2)	(\fImkfs\fP is tape file 2)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
\fBMkfs\fP
\fBfilesystem:\fP \fIdk\|\fP(0,0)	(root is the first filesystem on drive 0)
\fBfilesystem size:\fP [NNNN] \fIsize\fP	(count of 1024 byte blocks in root)
\fBbytes per inode:\fP [4096] \fIbytes\fP	(number of bytes per inode)
\fBinterleaving factor (m, 2 default):\fP \fIm\fP	(interleaving, see above)
\fBinterleaving modulus (n, 127 default):\fP \fIn\fP	(interleaving, see above)
\fBisize = XX\fP	(count of inodes in root filesystem)
\fBm/n = \fP\fIm n\fP	(interleave parameters)
\fBExit called\fP
\fInn\fP\fBBoot\fP
\fB:\fP	(back at tape boot level)
.TE
.DE
.PP
The number \fBnnnn\fP is the device number of the device (high byte is the
major device number and the low byte is the unit number).  The \fBmmmmmm\fP
number is the CSR of the device.  This information is mainly used as 
a reminder and diagnostic/testing purposes.
.sp
You now have an empty UNIX root filesystem.
.NH 3
Step 4: Restoring the root filesystem
.PP
To restore the root filesystem onto it, type
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,3)	(\fIrestor\fP is tape file 3)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
\fBRestor\fP
\fBTape?\fP \fItp\|\fP(0,5)	(root \fIdump\fP is tape file 5)
\fBDisk?\fP \fIdk\|\fP(0,0)	(into root filesystem)
\fBLast chance before scribbling on disk.\fP	(type a carriage return to start)
.B
"End of tape"	\fR(appears on same line as message above)\fP
\fInn\fPBoot
\fB:\fR	(back at tape boot level)
.R
.TE
.DE
.PP
If you wish, you may use the \fIicheck\fP program on the tape,
\fItp\|\fP(0,4), to check the consistency of the filesystem you have just
installed.  This has rarely been useful and is mostly for the voyeuristic.
.NH 3
Step 5: Booting UNIX
.PP
You are now ready to boot from disk.
Type:
.DS
.TS
lw(1.5i) l.
\fB:\fP\fIdk\|\fP(0,0)unix	(bring in unix from the root system)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
.TE
.DE
.PP
The standalone boot program will then load unix from
the root filesystem you just created, and the system should boot (the
exact strings will vary of course):
.DS
.B
.\"CHECK
2.11 BSD UNIX #157: Sat Oct 24 16:44:40 CDT 2026 
    sms@vernon.2bsd.com:/usr/src/sys/GENERIC

phys mem  = 4186112
avail mem = 3709504
user mem  = 307200

configure system
\fI\&... information about available devices ...\fP
.R
(Information about various devices will print;
most of them will probably not be found until
the addresses are set below.)
.B
erase=^?, kill=^U, intr=^C
#
.R
.DE
.PP
UNIX itself then runs for the first time and begins by printing out a banner
identifying the release and
version of the system that is in use and the date that it was compiled.  
.PP
Next the
.I mem
messages give the amount of real (physical) memory, the amount of memory
left over after the system has allocated various data structures, and the
amount of memory available to user programs in bytes.
.PP
The information about different devices being attached or not being found
is produced by the \fIautoconfig\fP\|(8) program.  Most of this is not
important for the moment, but later the device table, \fI/etc/dtab\fP,
can be edited to correspond to your hardware. You will likely need to create a 
custom kernel to support your hardware and remove support for hardware you 
don't have. This is discussed in detail in Appendix A.
.PP
The tape drive of the correct type should have been detected and attached.
.PP
The \*(lqerase ...\*(rq message is part of /.profile
that was executed by the root shell when it started.  This message
is present to remind you that the character erase,
line erase, and interrupt characters are set to what is
standard for DEC systems; this ensures that things are
consistent with the DEC console interface characters.
.PP
UNIX is now running single user on the installed root filesystem,
and the `UNIX Programmer's Manual' applies.
The next section tells how to complete
the installation of distributed software on the /usr filesystem.
The `#' is the prompt from the shell,
and lets you know that you are the super-user,
whose login name is \*(lqroot\*(rq.
.PP
The disk with the new root filesystem on it will not be bootable
directly until the block 0 bootstrap program for your disk has been installed.
There are copies of the bootstraps in /mdec.
Use \fIdisklabel\fP\|(1) to install the boot block onto the disk.
.DS
\fB#\fP disklabel -r -B \fIdk\^\fP0 device
.sp
For example:
.sp
 disklabel -r -B xp0 rp06
.sp
will install /mdec/hpuboot into sector 0 of xp0
.sp
disklabel -r -B ra0
.sp
Will install /mdec/rauboot into sector 0 of ra0 (since MSCP disks provide their
device geometry automatically, there is no need to specify a device type as in 
the previous example)
.sp 
.DE
.PP
The block zero bootstraps and the devices they support are:
.DS
.TS
l l.
boot	disk_type
_
hkuboot	rk06/rk07
rauboot	MSCP drives
rkuboot	rk05
rluboot	rl01/rl02
si95uboot	si_eagle
dvhpuboot	diva
hpuboot	rp04/rp05/rp06
rp07uboot	rp07
rm03uboot	rm03
rm05uboot	rm05/cdc9766
si51uboot	si_eagle/2351A
si94uboot	rm2x/fuji160
.TE
.DE
.PP
.B NOTE:
If none of the above are correct (most likely with a SMD drive with differing
geometry) then you will have to use a tape/floppy boot procedure rather than
a sector 0 boot block.  This can be fixed by creating a customized sector 0
boot program once the system sources have been loaded.
.PP
Once this is done, booting from this disk will load and execute the block
0 bootstrap, which will in turn load /boot.  \fB/boot\fP will print
on the console:
.DS
.TS
lw(1.5i) l.
.B
\fInn\fPBoot from \fIdk\fP(\fIctlr\fP,\fIunit\fP,\fIpart\fP) at \fIcsr\fP\fR
:
.R
.TE
.DE
.PP
The boot block automatically loads and runs /\fIboot\fP for you;
if /\fIboot\fP is not found, the system will hang/loop forever.
The block 0 program is very small (has to fit in 512 bytes) and simple 
program, however, and can only
boot the second-stage boot from the first filesystem.
Once /boot is running and prints its ``: '' prompt,
boot unix as above.
.PP
As distributed /\fIboot\fP will load \fIdk\fP(0,0)unix by default if a
carriage return is typed at the \fB:\fP prompt.
.PP
\fBNOTE:\fP NONE of the primary bootstraps have a prompt or alternate program
name capability because of space considerations.  No diagnostic message
results if the file cannot be found.
.NH 3
Step 6: Setting up the /usr filesystem
.PP
First set a shell variable to the name of your disk, so
the commands used later will work regardless of the disk you
have; do one of the following:
.DS
.TS
l l.
\fB#\fP disk=hk	(if you have RK06's or RK07's)
\fB#\fP disk=rl	(if you have RL01's or RL02's)
\fB#\fP disk=ra	(if you have an MSCP drive)
\fB#\fP disk=xp	(if you have an RP04/5/6/7, RM03, RM05, or other SMD drive)
.TE
.DE
.PP
The next thing to do is to extract the rest of the data from the tape.
You might wish to review the disk configuration information in Section
4.2, "Disk configuration" before continuing; you will have
to select a partition to restore the /usr filesystem into which is
at least \fB25\fP Megabytes in size (this is just barely enough for
the system binaries and such and leaves no room for the system
source.)\(dg
.FS
.IP \(dg
Previously a lengthy table of partition names organized by
specific disk type was given.
With the introduction of disk labels this is no longer
necessary (or possible since each site can select whatever partitioning
scheme they desire).  In Step 2 (creating the disklabel) a partition should
have been created for \fI/usr\fP.  If this was not done then it may be easier
to perform step 2 now than to use the more complex \fIdisklabel\fP\|(8)
program and \fIed\fP\|(1).
.FE
.PP
In the command below \fIpart\fP is the partition name (a-h) for the partition
which will hold /usr. For example, if you were going to use the g partition 
for /usr, you would do the following:
.DS
.TS
l l.
\fB#\fP part=g
.DE
.TE
.PP
Now, define the symbol \fIname\fR to refer to complete drive and partition to 
be used for /usr:
.DS
.TS
l l.
\fBname=${disk}0${part}
.TE
.DE
.PP
Next, find the tape you have in the following table and execute the
commands in the right hand portion of the table:
.DS
.TS
l l.
DEC TM02/03, TE16/TU45/TU77	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV ht0; sync
DEC TS11, TK25/TU80/TS05	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV ts0; sync
DEC TM11, TU10/TE10/TS03	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV tm0; sync
DEC TMSCP, TK50/TZ30/TU81	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV tu0; sync
EMULEX TC11	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV tm0; sync
.TE
.DE
.PP
Then execute the following commands:
.br
.ne 5
.sp
.FS
.IP \(dd
The amount of time it will take the \fItar\fR command to extract each archive 
depends on the speed of the real or emulated CPU, tape drive and disk drive. 
.FE
.DS
.TS
lw(2i) l.
\fB#\fP date \fIyymmddhhmm\fP	(set date, see \fIdate\fP\|(1) - note that "date: can't write wtmp file."
\&....	is expected, harmless and can safely be ignored)
\fB#\fP passwd root	(set password for super-user)
\fBNew password:\fP	(password will not echo)
\fBRetype new password:\fP
\fB#\fP hostname \fImysitename\fP	(set your hostname)
\fB#\fP newfs ${name} 	(create empty user filesystem)
(this takes a minute)
\fB#\fP mount /dev/${name} /usr	(mount the usr filesystem)
\fB#\fP umask 22	(set file creation mask)
\fB#\fP cd /usr	(make /usr the current directory)
\fB#\fP mt rew	(rewind the distribution tape to the beginning)
\fB#\fP mt -f /dev/rmt12 fsf 6	(position the distribution tape to the 6th file)
\fB#\fP tar xpbf 20 /dev/rmt12 	(extract all of usr except usr/src\(dd)
(Continued on next page)	
.TE
.DE
.\" This is to force a page break in the middle of a table
.bp
.DS
.TS
lw(2i) l.
\fB#\fP mkdir src	(make directory for source)
\fB#\fP cd src	(make /usr/src the current directory)
\fB#\fP mt -f /dev/rmt12 fsf	(skip to the start of the next tape file)
\fB#\fP tar xpbf 20 /dev/rmt12 	(extract the system source)
\fB#\fP cd /	(back to root)
\fB#\fP chmod 755  /  /usr  /usr/src /usr/src/sys	(set the proper directory permissions)
\fB#\fP umount /dev/${name}	(unmount /usr)
.TE
.DE
.PP
You now have installed a complete base system, along with the ability to 
recompile the kernel. However, the remainder of the source code in the /usr 
filesystem still needs to be loaded, which is described below. it is 
\fBstrongly\fR recommended that you install the remaining source code as it 
will be needed to install future patch levels (refer to Appendix C for 
additional information).
.PP
You can check the consistency of the /usr filesystem by doing
.DS
\fB#\fP fsck /dev/r${name}
.DE
.PP
The output from
.I fsck
should look something like:
.DS
.B
** /dev/r\fIxx\fP0x
filesystem: /usr

NEED SCRATCH FILE (179 BLKS)
ENTER FILENAME:  /tmp/xxx
** Last Mounted on /usr
** Phase 1 - Check Blocks and Sizes
** Phase 2 - Check Pathnames
** Phase 3 - Check Connectivity
** Phase 4 - Check Reference Counts
** Phase 5 - Check Free List
4463 files, 35838 used, 196678 free
.R
.DE
.PP
If there are inconsistencies in the filesystem, you may be prompted
to apply corrective action; see the document describing
.I fsck
for information.
.NH 3
Step 7: extracting the remaining source code
.PP
You should now remount the /usr filesystem and restore the rest of /usr/src as
shown below:
.DS
.TS
lw(2i) l.
\fB#\fP mount /dev/${name} /usr
\fB#\fP cd /usr/src	(make /usr/src the current directory)
\fB#\fP mt rew	(rewind the distribution tape to the beginning)
\fB#\fP mt -f /dev/rmt12 fsf 8	(position the distribution tape to the 8th file)
\fB#\fP tar xpbf 20 /dev/rmt12	(extract the remaining source
.TE
.DE
.NH 3
Step 8: editing the /etc/fstab file
.PP
The /etc/fstab file describes where the filesystems you created in Step 2 above
will be automatically mounted by the kernel. As an example, the following is an 
/etc/fstab file defining three partitions on an RM05 disk. The `a' partition 
holds the root filesystem /, the `b' partition describes the area used for 
swapping, and the `g' partition contains all of /usr, including the kernel and 
other system source files (if you installed them). \fBNOTE:\fR A bootable 
2.11BSD disk pack requires both an `a' and a `b' partition.
.nf
.in 0
.cs CW 20
.ft CW

/dev/xp0a       /       ufs     rw              1       1
/dev/xp0b       none    swap    sw              0       0
/dev/xp0g       /usr    ufs     rw              1       2
.fi
.NH 2
Configure the newly-installed system
.PP
Assuming you are performing a fresh install and not upgrading a system, proceed
to Chapter 4, "System setup.
