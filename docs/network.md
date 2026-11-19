# The network

2.11BSD's networking is compiled into the kernel: sockets, the Internet
protocols (IP, ICMP, UDP and TCP), ARP, raw sockets, the routing table
and Unix-domain sockets, with the loopback interface, eight
pseudo-terminals, and a driver for NE1000 and NE2000 Ethernet cards and
the many that copy them. `tests/system` configures the loopback
interface, pings it, and logs in over it with `telnet`, to `telnetd`
started by `inetd`; then it sets up the Ethernet interface, pings the
network's gateway, and logs in from the host with `telnet`, sending a
file each way.

The code is upstream's, as the PDP-11 runs it, with changes where it
assumed the PDP-11's address spaces; what it needed from the 286 is in
`usr/src/sys/i286`: `net_mac.h`, `mch_net.s`, `net_sys.c`, `net_data.c`
and `in_cksum.c`. The card's driver is `usr/src/sys/i286if/if_ne.c`.

## Its data segment

On the PDP-11 the network runs in supervisor mode, in an address space of
its own, and the kernel and it call each other through `KScall` and
`SKcall` ([memory model](memory-model.md)). Its data space there is 64K,
as a segment is here, and holds its variables and its mbufs. So the
network keeps a data segment of its own, `NET_GROUP`, and within it every
pointer is near, as within the PDP-11's supervisor space. Its files are
compiled with `-nd=NET`, which names the segment, and the kernel's
Makefile lists them as `NET`.

As built the segment is 39K: 170 mbufs of 128 bytes, 12 clusters of 1K
and about 4K of variables, ARP's table among them. The kernel's own data
segment gained 6 bytes by it. Where upstream chose a value for 16-bit machines, the 286 takes the
PDP-11's: at most 15 packets in an interface's queue and 8K in a socket's
buffer, whose counts are 16 bits.

## Its stack

The network runs on the kernel's stacks: the process's kernel stack in a
system call, the interrupt stack in its software interrupt. So in the
network's code DS is its own segment and SS is always the kernel's
(`-zu` tells the compiler that the two differ), and the network reaches
the kernel's data through SS. A pointer of that kind is declared with
`KSPACE` (`machine/machparam.h`), which is Open Watcom's
`__based(__segname("_STACK"))` in the network and nothing in the
kernel. That is how the network sees `u`, through `netuptr`, a copy of
`uptr` that `KScall` keeps; the `struct uio` of a read or write; a
sysctl's lengths; and what else a system call keeps on its stack.

The alternative was a stack for the network in its own segment, so that
SS and DS were the same there. The network sleeps, inside `soreceive`
and the like, so it would want a stack for every slot, copied with the
slot when processes take turns in it, and the interrupt entry would have
to change stacks when it found the processor on one. Running on the
kernel's stacks costs instead wherever the network takes the address of
one of its locals, and a trial compile found about 16 such places, the
network's data being mostly in mbufs.

A local's address in the network is far, SS and offset. Passed where a
far pointer is wanted, as to `bcopy`, it is right. Taken as a near
pointer it would be an offset in the wrong segment, and the compiler
would say nothing when a cast does it, as in `(caddr_t)&local`, or when
the far pointer goes to a function with no prototype, which then gets
two bytes it does not expect. Each such place was found and changed:

- the routines that take one as a pointer argument declare it `KSPACE`:
  the address family's hash routine (`&h` in `rtalloc`), the protocols'
  option routines (`sosetopt`'s `&m0`), `ip_insertoptions` (`&len`);
- `rtinit` takes its addresses `ANYSPACE` (far), with a prototype in
  `route.h`, since some callers pass locals and some the network's data;
  a pointer that walks from a list's head on the stack down its links,
  as in `m_copy` and `sosend`, is `ANYSPACE` too;
- a local that is the network's own data in disguise moved off the
  stack: `ip_output`'s spare route and `rtredirect`'s are static (both
  run at `splnet` and do not sleep), and `rtinit`'s entry and
  `sorflush`'s copy of a socket buffer are in mbufs;
- casts of locals' addresses to near pointers went (`in_ifinit`,
  `ip_dooptions`, `ifconf`).

`tools/netcheck.py` (below) keeps the count of what is left.

## Calls between the two

`KScall(func, nbytes, args...)` calls a network routine from the kernel;
`SKcall` a kernel routine from the network (`mch_net.s`). Each pushes the
`nbytes` of arguments again for the routine, makes DS the routine's
segment, and puts the caller's back after the call; `KScall` sets
`netuptr` and both put back the value they found, since processes that
run while the network sleeps may call it themselves. The macros the
PDP-11's code calls them by (`SOBIND`, `SLEEP` and the rest) are in
`machine/net_mac.h`, with a prototype for each routine, which both sides
see: what the caller passes and what the routine takes are one
declaration, and the compiler holds each definition to it.

A pointer that crosses means nothing on the other side unless it is only
handed back, as the kernel keeps a socket and the network a process. Where
the PDP-11's kernel passes the address of something on its stack, which
both sides could read there:

- what the network only reads or writes in place, it reaches with a
  `KSPACE` pointer: `socreate`'s and `sogetopt`'s results, `soreceive`'s
  uio and its names and rights, `sosend`'s uio;
- what the network would pass down through its protocols, the kernel's
  fake mbufs holding a name, an option or rights, and the buffers of an
  ioctl and of a `stat`, the `k*` entry points in `net_sys.c` copy to or
  from an mbuf of the network's around the real call. The kernel passes
  an ioctl's command without its size, so the socket's own ioctls are
  done there, and an interface's, a route's or a protocol's has the
  kernel's whole buffer, 256 bytes, copied to a cluster and back;
- where the network passes the address of one of its locals, the macro
  casts it to the kernel's near pointer, which is right: the local is on
  the kernel's stack. `unpbind` and `unpconn` take the path, which is in
  an mbuf of the network's, as a far pointer.

The network has its own copies of the kernel routines that would take
pointers to its data, as on the PDP-11: `printf`, a character at a time
to the kernel's `putchar`; `panic`; `uiomove`; `sysctl_int`. They are
named `netprintf` and so on by `net_mac.h`, since the two are linked into
one image, as is its mbuf free list, `mfreelist`, whose upstream name is
also one of the kernel's routines. Kernel routines it calls with no
copies of its own, `suser`, `ufalloc` and `ufavail`, it calls through
`SKcall`.

The network never names a kernel variable: the compiler would take the
address in the network's own segment. It reads `time`, `lbolt` and
`selwait` through `ktime`, `klbolt` and `kselwait`, which hold their
addresses in the kernel's segment (`mch_net.s`). The kernel reads and
writes a socket's process group with `mfsd` and `mtsd`, which reach into
the network's segment.

The network calls a few kernel routines directly, the ones that do not
depend on DS: the copy routines, `bcopy`, `bzero` and `bcmp`, which take
far pointers; the `spl` routines; `insque`, `remque` and `strlen`, which
work on the caller's data; port I/O; and the compiler's long arithmetic.
Each reaches the kernel's data, and its own arguments when it takes them
without a frame, through SS.

A network routine the clock is to call is marked with `SUPERFUN`, bit 0 of
its selector, as the PDP-11 marks one with bit 0 of its address, and
`softclock` calls it through `KScall`. Sleep channels the network passes
are made odd with `SUPERADD`, as on the PDP-11, to keep them apart from
the kernel's own.

The network's software interrupt is requested by `setsoftnet`, which sets
a bit in `spending`; `spllower` runs `netintr` at `splnet` once the level
drops below it, as it runs `softclock` ([processes](processes.md)).

## What the compiler does not see: netcheck

`tools/netcheck.py` runs at every link of the kernel, on its objects by
way of `wdis`, and fails the build if it finds:

- the network calling a kernel routine directly that is not on its list
  of those that do not depend on DS, or naming a kernel variable;
- the kernel calling a network routine directly, or naming a network
  variable;
- a function of the network's forming a near address of its stack a
  different number of times, with nothing about SS beside it, from the
  count in the script's `BARE` table. Those left are `KSPACE` pointers on
  purpose, and near pointers handed to the kernel, each read and found
  right; a change is to be read again before the table is.

Its first run found three protocols calling the kernel's `sysctl_int`,
whose arguments would have been in the wrong segment, and the count found
the places listed above.

Two more mistakes of the same kind that it does not catch, met on the
way: `caddr_t KSPACE p` declares a based variable of type `caddr_t`, not
a based pointer, which is `char KSPACE *p`; and assembly that reads its
arguments with BX, which defaults to DS, reads them from the wrong
segment when the network calls it.

## The loopback interface and pseudo-terminals

`NLOOP` is 1 and `NPTY` 8 (`sys/PC`). `/etc/rc` configures `lo0` as
127.0.0.1 and starts `inetd`, which answers `telnet` and the echo
service (`usr/src/i286/etc/inetd.conf`); `/etc/hosts` names only
`localhost`. The pseudo-terminals are `/dev/ptyp0` to `ptyp7` and
`/dev/ttyp0` to `ttyp7`, and `/etc/ttys` lists the second, off, so that
`who` shows who logged in on one. The programs built for the network are
`ifconfig`, `ping`, `telnet`, `telnetd` and `inetd`
([the C library and programs](userland.md)).

`telnetd` asks the client for its terminal type and its window's size,
and gives the pseudo-terminal that size and `TERM` that type; upstream's
took the size only from a client that offered it unasked, as BSD's
`telnet` does. A type the termcap lacks is often a newer name for one it
has, made by adding to the name, as `xterm-256color` and `xterm-ghostty`
are xterms, so `telnetd` shortens such a name a part at a time, at each
`-`, until the termcap knows it, and leaves it as it came if the termcap
never does. Upstream's set the erase and kill characters from an
uninitialized variable; they are now `gettytab`'s, DEL and ^U, which
`login` sets again anyway. A terminal emulator's Backspace sends DEL.
Root's `.profile` and `.login` make ^H the erase character only on the
console's screens, whose Backspace sends that ([the
console](console.md)).

## The Ethernet card

`if_ne.c` drives the NE1000, the NE2000, and the cards and emulations that
copy them: a National Semiconductor DP8390, or a copy of one, with buffer
memory that only the controller reaches, so that frames go in and out
through the card's data port by what the controller calls remote DMA.
It is compiled with the network and runs in it, as the PDP-11's
interface drivers do.

Cards sold as NE2000-compatible differ, from each other and from the
NE2000, so at boot the driver finds out what the card does rather than
going by its name:

- Where it is: the first card that answers at 0x300, 0x280, 0x320, 0x340
  or 0x360 (`neport`). Before anything is written that matters, the test
  that a DP8390 is there is that its tally counters read zero once read;
  then the card is reset.
- Its station address, from the PROM at the start of the card's memory,
  read a byte at a time. A card with a 16-bit data path gives each byte
  twice, an 8-bit one once; either is taken.
- How wide its data port is. Sixteen bits at a time is used only if the
  PROM read doubled and a check passes: 64 bytes written 16 bits at a
  time are read back a byte at a time, and the other way about. A card
  that is 8 bits wide behind a 16-bit face would pass a check made 16
  bits both ways, and lay its memory out wrong, a byte in every other
  place; an NE2000 emulated on an 8-bit card is one, and 86Box's 8-bit
  NE2000 is another. Otherwise it is a byte at a time, with the 286's
  `rep insb` and `rep outsb` if a check of them passes, and a loop if not.
- How much buffer memory there is, and where: 32K at 0x4000, as some
  emulations have; 16K there, as the NE2000 has; 8K at 0x2000, as the
  NE1000 has; or from 0x100, as Cabletron's cards. Each 8K of a size is
  written before any of it is read back, so that memory that repeats is
  not taken for more of it.
- Its interrupt request line, unless `neirq` gives it. The card is made
  to request one, by finishing a remote write, and the line found
  requesting in the 8259s' request registers then, and not before or
  after, is the card's; it is still masked, having no handler. (The
  usual way, a remote read of nothing, makes some emulations only pulse
  the line, which leaves nothing in the register.) If none is found, 3.

It says what it found:

    ne0 at 0x300 irq 3: 00:86:b0:12:34:56, 16K, 8-bit

The controller is run as National's later data sheets say, which the
copies follow:

- When the receive ring overflows, the driver recovers as they
  prescribe: stop, wait for a frame going out to stop too, take the
  frames in the ring with the transmitter looped back, and send again
  what the stop cut off. Nothing new is sent meanwhile.
- A frame header that cannot be right, which some copies write now and
  then, makes the driver drop everything the ring holds.
- The card's request to the 8259 is edge-triggered: if the card still
  wants service when the handler is done, its interrupt mask is cleared
  and set again, which makes a new edge.
- A watchdog runs once a second. It does what an interrupt was lost for,
  says so if that keeps happening (the line is most likely wrong), and
  starts again a card whose frame has not gone in three seconds.

Frames are copied straight between the card and mbufs, into clusters
when they are long, and one frame is sent at a time. A received ARP frame
goes to `arpinput` at once, at `splimp`, as on the PDP-11, and an IP one
to IP's queue. ARP, `netinet/if_ether.c`, took two changes of the kind
above: the locals of `arpwhohas` and `in_arpinput` whose addresses it
passes as near pointers are static, neither routine being entered twice
at once, and `arpresolve` takes the output routine's locals as far
pointers.

The card's interrupt handler is in the network, so the kernel calls it
through `KScall`: the driver gives `setirq` the handler marked with
`SUPERFUN`, as it gives `timeout` a function for `softclock`, and
`intr` sees the mark ([processes](processes.md)). `setirq` refuses a
line that already has a handler; the second serial port, which takes its
line, 3, when it is first opened, fails to open with `EBUSY` if a card
has it.

A card at 0x320 has ports up to 0x33f, over the AHA-1542's 0x330 and
0x334. The two cannot be in one machine, and the SCSI driver's probe gets
lost on such a card.

`/etc/rc.conf` names the machine and gives the interface's address,
netmask and gateway, and `/etc/rc` reads it and runs `/etc/netstart`,
which sets the interface up with `ifconfig` and adds the default route
with `route`. As shipped it has no address, and only the loopback
interface runs ([the C library and programs](userland.md)).

The driver has run on 86Box's NE1000s and NE2000s, five kinds, which
between them are 8 and 16 bits wide and have 8K or 16K; at each port of
the list, and on lines 3, 5, 7, 10 and 11. `tests/system` uses the
8-bit NE2000, `NETCARD` choosing another. The 32K layout and the PROM
read once are what emulations derived from Bochs's NE2000 do, as on the
PicoMEM, an 8-bit card whose data port is a byte wide however the
controller is set; the driver has not run on one, nor on any real card.

## Not done

- Two transmit buffers, so that one frame is copied to the card while
  the other goes, which the card's 16K would allow; the multicast
  filter; the workaround for the early DP8390s that want a remote read
  before each remote write.
- `syslogd`, `ftp`, `ftpd`, `rlogin`, `rlogind`, the mail system.
  (`netstat`, `arp` and `route -f` read the network's tables through
  `/dev/kmem`: [programs that read the
  kernel](userland.md#programs-that-read-the-kernel).)
- Unix-domain sockets are compiled in and not tried beyond `syslog`'s
  connect to a `/dev/log` that is not there.
- `recvmsg` and `sendmsg` in the C library: the kernel's `struct
  msghdr` holds far pointers now, as `struct iovec` does.
- The serial line interface, whose calls are left out of `net_mac.h`.
- The Internet checksum, mbuf copies and the network card's transfers in
  assembly ([memory model](memory-model.md)); the size of the mbuf pool,
  which could grow to fill the segment.
- Most of it has run in the emulator only. The system has also been
  logged in to over the network on a real machine.
