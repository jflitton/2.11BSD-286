.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)5.t 3.0 (2.11BSD) 2026/11/09
.\"
.ds lq ``
.ds rq ''
.ds LH "Installing/Operating \*(2B
.ds RH Network setup
.ds CF \*(DY
.LP
.nr H1 5
.nr H2 0
.bp
.LG
.B
.ce
5. NETWORK SETUP
.sp 2
.R
.NL
.PP
The networking in \*(2B runs in supervisor
mode separate from the main kernel.  There is room without overlaying
to hold both a SL/IP and an Ethernet driver.
The networking portion of the kernel resides in ``/netnix'', and is
loaded after the kernel is running.  Since the kernel only looks for the
file ``/netnix'', it will panic if it is unable to load ``/netnix''.
\fBNOTE\fP:  The ``/unix'' and ``/netnix'' imagines must
have been created at the same time - do not attempt to use mismatched
images or the system will panic.
.PP
\*(2B provides support for the DARPA standard Internet
protocols IP, ICMP, TCP, and UDP.
Network services are split between the
kernel (communication protocols) and user programs (user
services such as TELNET and FTP).  This section describes
how to configure your system to use the Internet networking support.
.PP
The ``correct'' use of the networking in \*(2B is almost always with the 
local host name and addresses in the \fI/etc/hosts\fP file, your local 
domain name and a set of nameservers in the \fI/etc/resolv.conf\fP file, 
and a default route to a gateway for all other network traffic. This will be 
explained in detail below. In particular, do not run \fIrouted(8)\fR or 
\fInamed(8)\fR unless you're extremely sure that you know what you're doing. 
This is doubly true if SL/IP is being used as the primary connection to the 
outside world.
.NH 2
Required kernel configuration settings
.PP
To configure the kernel to include the Internet communication
protocols, define the INET option.  This automatically defines
the NLOOP option.
Include the pseudo-device ``pty'' in your machine's configuration
file using the NPTY options.
The ``pty'' pseudo-device forces the pseudo terminal device driver
to be configured into the system, see \fIpty\fP\|(4).  The NLOOP
option forces inclusion of the software loopback interface driver.
The loop driver is used in network testing as well as for the
system talking to itself rather than transmitting the data over the wire.
.PP
If you are planning to use the Internet network facilities on a 10Mb/s\(dg
.FS
.IP \(dg
All DEC PDP-11 Ethernet controllers were 10Mb/s devices. Therefore, we refer 
to any connected Ethernet as 10Mb/s, even when running on an emulator where the
Ethernet speed might be much faster.
.FE
Ethernet, the pseudo-device ``ether'' should also be included
in the configuration using the NETHER option; this forces inclusion of
the ARP (Address Resolution Protocol) module used in mapping between 48-bit
Ethernet and 32-bit Internet addresses.
.PP
Before configuring the appropriate networking hardware, you should
consult the manual pages in Section 4 of the Programmer's Manual.
The following table lists the devices for which software support exists.
.DS
.TS
l l.
Device name	Manufacturer and product
_
de	DEC DEUNA/DELUA 10Mb/s Ethernet
qe	DEC DEQNA 10Mb/s Ethernet
qt	DEC DELQA-YM (DELQA-T, DELQA-Plus) 10Mb/s Ethernet
sl	Serial line IP (SL/IP)
ec	3Com 10Mb/s Ethernet\(dd
il	Interlan 1010 and 10101A 10Mb/s Ethernet interfaces\(dd
vv	Proteon ProNET - Token Ring Interface\(dd
acc	LH/DH-11 1822 IMP/PSN Interface\(dd
.TE
.DE
.FS
.IP \(dd
These devices were known to work in 2.11BSD at one time but have not been 
tested (due to lack of suitable hardware) in many years and may not work.
.FE
.PP
SL/IP is also available.  It is surprisingly efficient.  Over a 9600 baud
line it is not unusual to see \fBftp\fP rates in the 800 bytes per second
range.
.PP
\fBNOTE:\fR Configuring, building and installing a kernel is described in 
Appendix A.
.PP
\fBNOTE:\fR All of the following information and examples are based on the new 
\fI/etc/rc.conf\fR configuration method. If you have upgraded from a system 
prior to PL500, your existing network configuration will be preserved and you 
can skip the following sections. However, while both old-style and new-style 
network startup scripts are supported on PL500, future patch levels will
likely only provide changes to the new-style startup scripts.
.NH 2
Basic network configuration
.PP
Basic network configuration involves setting up enough of the networking that 
you can connect to the system via \fItelnet\fR and complete the rest of the 
configuration tasks from a terminal (real or emulated). This dates back to the 
era when PDP-11 systems often had printing terminals as consoles, which made 
editing files a bit of a chore. If you are running 2.11BSD on a PDP-11 with a 
video terminal or emulator, you can continue with the advanced configuration 
steps via the console if you prefer.
.NH 3
Enabling networking and basic TCP/IP configuration in rc.conf
.PP
Edit the file \fI/etc/rc.conf\fR and make the following changes:
.DS
.TS
lw(0.5i) l.
rc_configured=	- Change from NO to YES
hostname=	- Change to your desired host/domain name (like mypdp.example.com)
address=	- Change to the IP address for this system
netmask=	- Change to the network mask for your network
gateway=	- Change to the default gateway for your network
interface=	- Change to the name of your network device (refer to the table in Section 5.1)
.TE
.DE
.PP
In years past, you would have received the necessary information from your 
network manager. Today, \fIyou\fR are probably your network manager. You can 
likely determine your domain name, netmask and gateway by looking for this 
information on either your router or on other computers on your network. The 
exact procedure depends on the software those devices are running, and all of 
the possibilities are too extensive to list here.
.PP
Your IP address must be unique on your network, or both your 2.11BSD system 
and the other system assigned the same IP address will have trouble 
communicating on the network. On modern systems, the DHCP protocol is normally 
used to automatically assign IP addresses as well as other settings. 2.11BSD 
pre-dates DHCP and requires these settings to be configured manually\(dg.
.PP
The comments in the \fI/etc/rc.conf\fR file provide some guidance.
.NH 3
Editing the /etc/hosts file
.PP
Edit the file \fI/etc/hosts\fR and replace the line following "# Define your 
host's IP address and name (both with and without domain name)" so that it has 
your IP address (it must match the definition in \fI/etc/rc.conf\fR), a tab, 
the full hostname with domain, a space character, and the hostname without 
the domain. The sample line in the file should make this clear.
.PP
Next, edit the line following "# Edit the following line *ONLY* to change the 
domain name" to match the domain name (without the host name) you selected in 
\fI/etc/rc.conf\fR above.
.FS
.IP \(dg
It is unlikely that you will have your own domain name or public IP address 
block allocation. In that case, you will want to use the domain name and an 
unused IP address in the same block that the other systems on your network are 
using. Note that many mail servers on the Internet will refuse to accept email 
from a host/domain name that doesn't exist (example.com is specifically 
reserved for use in sample configurations such as this). That's just the way 
it is in the modern world.
.FE
.NH 3
Editing the /etc/resolv.conf file
.PP
Edit the \fI/etc/resolv.conf\fR file to specify your domain name on the line 
that begins with "domain". Again, this is just the domain name (such as 
example.com), not the complete hostname (such as mypdp.example.com).
.PP
The name servers listed on each line that begins with "nameserver" are 
generally sufficient - they are public name servers. If you are operating your 
own name server(s), you may want to change these lines to match your name 
server(s).
.NH 2
Advanced network configuration in /etc/rc.conf
.PP
There are quite a few optional network services which are enabled or disabled 
via settings in \fI/etc/rc.conf\fR. Some of those services also have their own 
configuration files, which will also be discussed in the appropriate section 
below. The sections below are in the order they appear in the 
\fI/etc/rc.conf\fR file.
.NH 3
The rwhod service
.PP
The rwhod service (when set to YES) enables both the broadcasting of status 
(host name, uptime, load average, and logged-in users) to other hosts on the 
local network as well as receiving the same data from other hosts on the local 
network. When rwhod is enabled, the \fIrwho(1)\fR and \fIruptime(1)\fR 
commands will display this information. If rwhod is not enabled, these commands
will not report any information. \fBNOTE:\fR Whether or not to run the rwhod 
service can be a matter of some debate, as it broadcasts information across the
local network. If none of your other systems have rwhod enabled, there is no 
point in enabling it on 2.11BSD. Otherwise, enabling it will allow you to see 
the status of those hosts and those hosts will be able to see the status of 
your 2.11BSD system.
.NH 3
The named service
.PP
The named service (when set to YES) causes the BIND name server to run. You 
almost definitely do not want to do this as BIND consumes a good amount of 
system resources, while there are public name servers (which you configured in 
Section 5.2.3) which you can use. Therefore, configuring BIND is beyond the 
scope of this document.
.NH 3
The sendmail service
.PP
sendmail is the utility that processes email. It is normally already enabled, 
but some additional configuration steps are required. sendmail configuration is
so arcane that many books have been written about it. However, the necessary 
configuration changes for 2.11BSD are relatively simple.
.PP
First, edit the \fI/etc/sendmail.cf\fR file and if necessary, change the line 
that reads "DDEXAMPLE.COM" to use your local domain (if not using example.com).
As an example \fBonly\fR, if your domain was FOO.COM\(dg you would change the 
line to read "DDFOO.COM" ("DD" means "Define Domain").
.FS
.IP \(dg
Please don't use FOO.COM - that is only being used as an example. Use whatever 
your local domain is, or leave it set to EXAMPLE.COM if you don't have a local 
domain.
.FE
.PP
Next, also in \fI/etc/sendmail.cf\fR, edit the line that reads 
"DARELAY.EXAMPLE.COM" and change it to the name of a host that will accept 
SMTP email from your 2.11BSD system. Similar to the above section, if your SMTP
server was called SMTP.FOO.COM\(dg, you would change this line to read 
"DASMTP.FOO.COM" ("DA" means "Define ARPANET relay").
.PP
Finally, you must add a host definition for your SMTP server in the file 
\fI/etc/hosts\fR in the line following "# Define your SMTP gateway/relay system
(used in sendmail.cf)". Like the host IP address and name you set in Section 
5.2.2, this will be an IP address, a tab, the full host name you used in "DA" 
above, a space, and then just the host name (without the domain name part).
.NH 3
The NTP (Network Time Protocol) service
.PP
NTP is a distributed time service available on the Internet. It provides a 
relatively high-precision time reference based on GMT.\(dd
.FS
.IP \(dd
You will also need to have correctly set your system's local time zone, as 
described in Section 4.1.2, "Setting the correct time zone".
.FE
.PP
Enabling this involves setting both the ntpd and ntp lines to YES. You will 
also need to edit the \fI/etc/ntp.conf\fR file to define several network time 
servers. First, remove the entire "peer   127.0.0.1" line and replace it 
with:
.DS
server  0.pool.ntp.org\(dg
server  1.pool.ntp.org
server  2.pool.ntp.org
server  3.pool.ntp.org
.DE
.FS
.IP \(dg
The distributed NTP service relies on all systems "playing by the rules". 
First, you should only use "server" lines in the \fI/etc/ntp.conf\fR file, not 
"peer" (server means you are only asking for the time, while peer means that 
you are willing to ask for or supply the time). Second, you should only use NTP
servers that have offered to provide public time service to any system. The 
pool.ntp.org servers are a rotating set of public time servers intended for 
clients like 2.11BSD.
.FE
.NH 3
The timed service
.PP
\fItimed\fR is an older method of synchronizing time across multiple systems. 
It should normally be left disabled unless you have older systems also running 
timed.
.PP
.NH 3
The routed service
.PP
\fIrouted\fR is an older method of discovering and reporting network routes. 
The routedflags should be set to NO, as modern configurations separate routers 
from host systems, and 2.11BSD uses the default gateway configured in Section 
5.2.1.
.NH 2
Configuring network access
.PP
The previous steps set up basic TCP/IP connectivity and optionally configured 
additional services. There are a few more steps you will need to take to allow 
users to access your 2.11BSD system.
.NH 3
Creating the pseudo terminals
.PP
When you configured your kernel (or if you're using one of the "canned" disk 
images) the number of pseudo terminals was specified in the NPTY section of the
kernel configuration. If they don't already exist, you will need to create the 
device nodes for the pseudo terminals. These device nodes are created in sets 
of 16 at a time. Each pseudo terminal really consists of two files in /dev: a 
master and a slave. The master pseudo terminal file is named /dev/ptyp?, while 
the slave side is /dev/ttyp?. Pseudo terminals are also used by several 
programs not related to the network. To create 16 pseudo terminals (normally 
plenty) execute the following commands:
.DS
\fB#\fP cd /dev
\fB#\fP MAKEDEV pty0\(dd
.DE
.FS
.IP \(dd
If these device nodes already exist, you will receive a bunch of "File exists" 
error messages. They are harmless and just indicate that your system already 
has had the pseudo terminals created.
.FE
.PP
More pseudo terminals may be made in groups of 16 by specifying \fIpty1\fP, 
\fIpty2\fP, \fIpty3\fP, etc.
.NH 3
Editing /etc/ttys
.PP
Now that you have created the pseudo terminals, you may need to let the 
system know to allow logins on them. To do this, edit the \fI/etc/ttys\fR file
In addition to creating the pseudo terminals. The file should already contain 
definitions for the first 16 pseudo terminals, such as the following example:
.DS
ttyp0   none                    network         secure\(sc
.DE
.PP
If you created additional pseudo terminals as shown in the previous section, 
you should add them to the end of the \fI/etc/ttys\fR file.
.FS
.IP \(sc
"secure" means that root is allowed to log in directly on that terminal. You 
probably do not want to allow this as it can be a major security hole. Instead,
users should log in with their regular username and then use the \fIsu(1)\fR 
command to obtain root access (if they are in group ``wheel''). Removing 
"secure" from any ttyp? lines where it appears is strongly recommended.
.FE
.NH 3
Editing /etc/hosts.allow
.PP
The \fI/etc/hosts.allow\fR file controls which systems can access various 
network services. As distributed, it looks like this:
.DS
ftpd: .example.com LOCAL
rlogind: .example.com LOCAL
rshd: .example.com LOCAL
tftpd: .example.com LOCAL
popper: .example.com LOCAL
telnetd: .example.com LOCAL
.DE
.PP
To control incoming telnet sessions, you would edit the ``telnetd'' line. The 
format of each line is described in the \fIhosts_access(5)\fR man page, 
accessed as follows:
.DS
man 5 hosts_access
.DE
.PP
The format and various options are described in detail in that man page and 
will not be repeated here. Briefly, you would replace the ``.example.com'' 
line with your local domain is (refer to Section 5.2 for additional 
information).
.NH 2
Network servers
.PP
In \*(2B most of the server programs are started up by a
``super server'', the Internet daemon.  The Internet 
daemon, \fIinetd\fP, acts as a master server for
programs specified in its configuration file, \fI/etc/inetd.conf\fP,
listening for service requests for these servers, and starting
up the appropriate program whenever a request is received.
The configuration file contains lines containing a service
name (as found in \fI/etc/services\fP), the type of socket the
server expects (e.g. stream or dgram), the protocol to be
used with the socket (as found in \fI/etc/protocols\fP), whether
to wait for each server to complete before starting up another,
the user name as which the server should run, the server
program's name, and at most five arguments to pass to the
server program.
Some trivial services are implemented internally in \fIinetd\fP,
and their servers are listed as ``internal.''
For example, an entry for the file
transfer protocol server would appear as
.DS
ftp	stream	tcp	nowait	root	/usr/libexec/ftpd	ftpd -l
.DE
.PP
or if you are using the \fItcp_wrapper\fP\(dg program as
.DS
ftp	stream	tcp	nowait	root	/usr/libexec/tcpd	ftpd -l
.DE
.FS
.IP \(dg
The \fItcp_wrapper\fP program is what consults the \fI/etc/hosts.allow\fR 
file, discussed above. Not all services can use \fItcp_wrapper\fP, either 
due to being an internal service or various other conditions.
.FE
.PP
Consult
.IR inetd (8)
for more detail on the format of the configuration file
and the operation of the Internet daemon.
.NH 2
Network data bases
.PP
Several data files are used by the network library routines
and server programs.  Most of these files are host independent
and updated only rarely.
.DS
.TS
l l l.
File	Manual reference	Use
_
/etc/hosts	\fIhosts\fP\|(5)	host IP addresses, names and aliases
/etc/networks	\fInetworks\fP\|(5)	network names
/etc/services	\fIservices\fP\|(5)	list of known services
/etc/protocols	\fIprotocols\fP\|(5)	protocol names
/etc/hosts.allow	\fIhosts_access\fP\|(5)	sites who can connect to services
/etc/hosts.deny	\fIhosts_access\fP\|(5)	sites blocked from services
(Continued on next page)
.TE
.DE
.\" This is to force a page break in the middle of a table
.bp
.DS
.TS
l l l.
/etc/hosts.equiv	\fIrshd\fP\|(8)	list of ``trusted'' hosts
/etc/rc.local	\fIrc\fP\|(8)	command script for starting local servers
/etc/ftpusers	\fIftpd\fP\|(8)	list of ``unwelcome'' ftp users
/etc/hosts.lpd	\fIlpd\fP\|(8)	list of hosts allowed to access printers
/etc/inetd.conf	\fIinetd\fP\|(8)	list of servers started by \fIinetd\fP
.TE
.DE
.NH 3
Use of /etc/hosts
.PP
When using the host address routines that use Internet name servers,
the file \fI/etc/hosts\fP is only used for setting interface addresses
and at other times when the network is not yet started,
and therefore it need only contain addresses for local hosts.
.PP
If you are connected to the Internet, it is highly recommended that
you use the default name server resolver routines for host name and 
address lookups, as this provides lookup service for all hosts and addresses
on the Internet. If your system is not connected to the Internet, you can use 
the \fI/etc/hosts\fR file to list the names and addresses of hosts on your 
local network.
.NH 3
/etc/hosts.equiv
.PP
The remote login and shell servers use an
authentication scheme based on trusted hosts.  The \fIhosts.equiv\fP
file contains a list of hosts that are considered trusted
and, under a single administrative control.  When a user
contacts a remote login or shell server requesting service,
the client process passes the user's name and the official
name of the host on which the client is located.  In the simple
case, if the host's name is located in \fIhosts.equiv\fP and
the user has an account on the server's machine, then service
is rendered (i.e. the user is allowed to log in, or the command
is executed).  Users may expand this ``equivalence'' of
machines by installing a \fI.rhosts\fP file in their login directory.
The root login is handled specially, bypassing the \fIhosts.equiv\fP
file, and using only the \fI/.rhosts\fP file.
.PP
Thus, to create a class of equivalent machines, the \fIhosts.equiv\fP
file should contain the \fIofficial\fP names for those machines.
.NH 3
/etc/rc.local
.PP
Most network servers are automatically started up at boot time
by the network startup scripts or by the Internet daemon (see above).
These include the following:
.DS
.TS
l l l.
Program	Server	Started by
_
rshd	shell server	inetd
rexecd	exec server	inetd
rlogind	login server	inetd
telnetd	TELNET server	inetd
ftpd	FTP server	inetd
fingerd	Finger server	inetd
tftpd	TFTP server	inetd
rwhod	system status daemon	/etc/rc.conf
syslogd	error logging server	/etc/rc.conf
sendmail	SMTP server	/etc/rc.conf
ntp	network time protocol (NTP)	/etc/rc.conf
named	local name server (DNS)	/etc/rc.conf\(dg
routed	routing table management daemon	/etc/rc.conf\(dg
timed	time distribution service	/etc/rc.conf\(dg
.TE
.DE
.FS
.IP \(dg
These services should normally \fBnot\fR be enabled.
.FE
.PP
Consult the manual pages and accompanying documentation (particularly
for sendmail) for details about their operation.
.PP
To have other network servers started up as well,
the appropriate line should be added
to either the Internet daemon's configuration file \fI/etc/inetd.conf\fP\(dg, 
or commands similar to the following should be placed in the site dependent
file \fI/etc/rc.local\fP\(dg.
.FS
.IP \(dg
The choice of \fI/etc/inetd.conf\fP or \fI/etc/rc.local\fP depends on whether 
your network server should be activated "on demand" (use \fI/etc/inetd.conf\fP)
or "always on" (use \fI/etc/rc.local\fP).
.FE
.DS
if [ -f /usr/local/bin/myserver ]; then
	/usr/local/bin/myserver & echo -n ' myserver'	>/dev/console
f\&i
.DE
.NH 3
/etc/ftpusers
.PP
The FTP server included in the system provides support for an
anonymous FTP account.
.PP
An anonymous account is enabled by creating a user \fIftp\fP.
When a client uses the anonymous account a \fIchroot\fP\|(2)
system call is performed by the server to restrict the client
from moving outside that part of the filesystem where the
user ftp home directory is located.  Because a \fIchroot\fP call
is used, certain programs and files used by the server 
process must be placed in the ftp home directory. 
Further, one must be
sure that all directories and executable images are unwritable.
The following directory setup is recommended.
.DS
\fB#\fP cd ~ftp
\fB#\fP chmod 555 .; chown ftp .; chgrp ftp .
\fB#\fP mkdir bin etc pub
\fB#\fP chown root bin etc
\fB#\fP chmod 555 bin etc
\fB#\fP chown ftp pub
\fB#\fP chmod 777 pub
\fB#\fP cd bin
\fB#\fP cp /bin/sh /bin/ls .
\fB#\fP chmod 111 sh ls
\fB#\fP cd ../etc
\fB#\fP cp /etc/passwd /etc/group .
\fB#\fP chmod 444 passwd group
.DE
When local users wish to place files in the anonymous
area, they must be placed in a subdirectory.  In the
setup here, the directory \fI~ftp/pub\fP is used.
.PP
NOTE: Mode 777 on the 'pub' directory can and has been abused!  Changing the
mode to 555 is a good choice but would require administrative assistance for
placing files in the 'pub' directory.  Probably not a bad idea though.
.PP
Another issue to consider is the copy of \fI/etc/passwd\fP
placed here.  It may be copied by users who use the
anonymous account.  They may then try to break the 
passwords of users on your machine for further access.
A good choice of users to include in this copy might
be root, daemon, uucp, and the ftp user.
.PP
Aside from the problems of directory modes and such,
the ftp server may provide a loophole for interlopers
if certain user accounts are allowed.
The file \fI/etc/ftpusers\fP is checked on each connection.
If the requested user name is located in the file, the
request for service is denied.  This file normally has
the following names on our systems.
.DS
uucp
root
.DE
Accounts with nonstandard shells should be listed in
this file.  Accounts without passwords
need not be listed in this file, the ftp server will
not service these users.
.NH 2
Important additional words about security
.PP
2.11BSD dates from an era when the Internet was a much smaller network, and 
many users knew "the management" of other Internet sites. As such, it was 
possible to just call or email someone at another site if people from that site
were trying to break into or otherwise damage your system. In the intervening 
35+ years, the Internet has become much larger, both in terms of the number of 
users and geographic reach. We also have cases where complaints to the 
management of another site are routinely ignored. Compounding things, recent 
"tests" of AI agents have sometimes gone rogue and started probing other 
Internet sites for security vulnerabilities.
.NH 3
DO NOT put a 2.11BSD system directly on the public Internet
.PP
It is \fBstrongly\fR discouraged to have a 2.11BSD system directly 
accessible from the public Internet. Many users will be running 2.11BSD behind
a router that does NAT translations, which will prevent many forms of external
attacks. Or a separate firewall can be used, depending on your network 
configuration. Either NAT or a firewall will generally let your 2.11BSD system 
contact the outside Internet, while preventing access from outside.
.PP
There has been 35+ years of security research and bug fixes to the *BSD code 
since 2.11BSD was first released. While many security fixes have been 
incorporated into 2.11BSD, there is no representation that it is completely 
secure.
.NH 3
Plaintext passwords
.PP
While 2.11BSD incorporates the password hashing algorithms from 4BSD (meaning 
that the password file does not have plaintext passwords), limitations of the 
PDP-11 CPU speed\(dg limit the complexity of the hashing. 2.11BSD has neither 
md5 or sha1 hashing. For similar reasons, this means that 2.11BSD does not 
support the SSH (Secure Shell) protocol. Only telnet, rlogin, etc. are 
available. This means that anyone who logs into a 2.11BSD system from the 
"outside world" is doing so by sending a plaintext password over the Internet 
from their system to yours, and Bad Guys can intercept that communication and 
recover your username and password.
.FS
.IP \(dg
As an example, it is possible to emulate a fully-loaded PDP-11/70 system which 
originally cost nearly $100,000 (in 1980s dollars!) with a Raspberry Pi Model 
3B+ or newer, and have the emulation run faster than the original PDP-11/70.
.FE
.NH 3
The secure keyword in /etc/ttys
.PP
When a line in \fI/etc/ttys\fR has the keyword ``secure'', it means that people 
who know the root password can simply log in as root. This should be disabled 
where it is not needed. For maximum security, only the system console should 
permit direct root logins via the ``secure'' keyword and it should be removed 
from all other lines. Users can log in with their regular username and then use
the \fIsu\fR command (assuming they are in group ``wheel'') and provide the 
root password. Usage of \fIsu\fR should be limited to cases where it is 
absolutely necessary (normally, for system management tasks). Referring to the 
previous section, you can see that when using \fIsu\fR over the Internet will 
send the root password in plaintext over the network.
.NH 3
Using a VPN
.PP
If appropriate, you can set up a VPN (Virtual Private Network) on your router 
or firewall. This will allow you to establish a secure connection from the 
Internet to your 2.11BSD system. The VPN will encrypt all data starting at the 
far end, and will only decrypt it once the data arrives at your router or 
firewall.
.PP
There are many different VPN packages available. Consult your router or 
firewall documentation to find out how to configure that device, as well as 
finding out what VPN client software works with the device. Depending on the 
VPN software in use, configuring your router or firewall for a VPN may provide 
you with a file to use on the VPN client. The file will contain all of the 
necessary information for the VPN client to connect to the VPN server.
