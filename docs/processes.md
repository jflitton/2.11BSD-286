# Processes and the kernel

How a process is confined to ring 3, how it enters the kernel, what its
memory is, and how the kernel switches between processes. All of this
runs in the emulator under `tests/kernel`, which boots the kernel with a
test program as `/etc/init`; `tests/user` checks the protection of a
process more closely, under a stand-in for the kernel.

## A process's segments

Each process has a local descriptor table. Segment N of its executable is
slot N, so its selector is `(N << 3) | 7`, the value the linker step wrote
into the program ([executable format](executable-format.md)). A small-model
program runs with CS = 0x0F and DS = SS = ES = 0x17.

A process's memory is that list of segments, 32 at most, and nothing else:
there is no fixed text, data and stack as on the PDP-11. The kernel keeps
two tables with an entry for every slot of every process (`pseg` and `ldt`,
`sys/i286/seg.h`): what it knows about the segment, and the descriptor the
processor uses. Both are far arrays outside the kernel's data segment.

- **Code segments are shared.** All the code segments of one executable
  lie one after another in a single piece of memory, which belongs to the
  file's text structure, as a PDP-11 program's text and overlays do. Every
  process running the program has descriptors that point into it. They are
  ring 3, readable, not writable.
- **Every other segment is the process's own**: a piece of memory of its
  own, ring 3, writable, copied at `fork`.
- A descriptor's limit is the segment's exact size in bytes.

The segment the executable names as its data segment holds the program's
data, then its stack, of the fixed size the executable gives, and above
that the break. `sbrk` moves the end of that one segment, to the byte: the
segment grows in place if the memory after it is free, and is moved if
not. Nothing a program can address lies past the break, so a stray
reference there is a fault. Where the stack sits is not settled (below).

The global table has one slot, `GDT_LDT`, that names the running process's
table.

A process runs with interrupts on and I/O privilege level 0. What the
processor then refuses it, each as a fault that arrives in the kernel:

| the process tries | exception |
|---|---|
| `CLI`, `STI`, `IN`, `OUT`, `HLT`, `LMSW` and the other privileged instructions | 13 |
| loading any selector of the global table, or calling or jumping through one | 13, error code the selector |
| `INT` with any number but the system call's, 3 or 4 | 13 |
| reading or writing past the end of a segment, or writing its code | 13 |
| moving the stack pointer outside the stack's segment and pushing | 12 |
| a 386 instruction | 6 |

`POPF` cannot turn interrupts off or raise the I/O privilege level: the
processor ignores those bits from ring 3.

## Entering the kernel

When a process is interrupted, faults or makes a system call, the
processor switches to the kernel stack named in the task state segment and
pushes the process's SS, SP, flags, CS and IP there. `sys/i286/mch_trap.s`
pushes the rest, to make a `struct trapframe` (`sys/i286/frame.h`).

The kernel stack is empty whenever the process is running, so the
process's frame is always at the top of its kernel stack. That is where
the kernel finds a process's registers, and how a process is started: the
kernel makes the frame what it would be had the process been interrupted
at its first instruction, and returns through it.

`USERMODE(tf->tf_cs)` says whether a frame is a process's.

On the way back to a process the kernel reloads the selectors in the
frame. If one of them is no longer good, the fault that follows is in the
kernel; `trap` knows the three instructions it can happen at, gives DS or
ES the null selector, and sends the process a segmentation violation.

The same fault can come on the way back to the kernel itself, from an
interrupt that found it running, and then no process is to blame. The
kernel does not keep ES: it holds whatever the last far pointer put
there, which after a copy to or from a process is that process's
selector, and it stays there when the kernel goes on to another
process, or when the process executes a program with fewer segments.
An interrupt saves it and cannot put it back, since the process that is
running now has no such segment. `trap` sees that the frame is the
kernel's, gives ES the null selector and goes on; what was interrupted
was not using ES, or it would have loaded it since. Before it told the
two apart, the process that happened to be running got the segmentation
violation. That needs a program with more segments than another, so it
waited for a medium-model program, `csh`, to be at work while the disk
and the daemons kept the kernel busy: once `tests/system` had `syslogd`,
`cron` and accounting running, a program the user's `csh` started, or
root's `telnet`, died so in each run.

## System calls

```
        mov     ax, number
        mov     bx, first argument word     ; then CX, DX, SI, DI, BP
        int     0x30
```

Arguments are in registers, a word each, in the order BX, CX, DX, SI, DI,
BP. A `long` is two words, low word first. A pointer is two words, offset
and then selector, in every memory model: a small-model program's stub
passes DS as the selector, or 0 with a null pointer. The kernel therefore
sees one argument layout and never needs to know a program's model to
find or widen an argument.

`mch_trap.s` pushes the registers one at a time so that those six are
next to each other in the frame, in argument order, and `syscall` copies
the six words from there to `u.u_arg`. It copies them, where it could have
used them in place, because the kernel's system call code changes its
arguments as it goes, and a call that is interrupted and started again
must find the process's registers as the process left them.

A call with more than six words of arguments takes one argument instead,
the far address of them all, and the kernel copies them in. Seven calls
do: `select`, `pselect`, `sigaction`, `sysctl`, `sendto`, `recvfrom` and
`getsockopt`.

The kernel returns with the carry flag clear and the result in AX, or
DX:AX for a `long` or a second value; or with carry set and the error
number in AX, which the stub stores in `errno` before returning -1. Every
other register comes back as it was.

The gate leaves interrupts on. It is the only gate besides 3 (breakpoint)
and 4 (`INTO`) that a process may use.

A structure a program hands the kernel has the same layout whatever the
program's model: a pointer in it is far. So far that is `struct iovec`,
`struct msghdr`, `struct sigaction` and `struct sigaltstack`; a library
for a model with near pointers converts them. The one structure that
cannot be converted so, because an `ioctl` passes it, is `struct
ifconf`, whose pointer is far in every model ([the network](network.md)). The one exception is the
lists of arguments and environment given to `execve`, which may be long:
the kernel reads them as the calling program's model has them, near
pointers in the list's own segment or far ones.

Stubs are written from a table of each call's arguments by
`tools/mkstubs.py`, for the small model or the medium, where a stub is
called far and a pointer to a function is already far. The C library's
table is
`usr/src/lib/libc/i286/sys/syscalls.tab`, and each line of it is checked
against the kernel's own table: the stub must pass as many words as the
kernel takes for the call. `vfork` needs a stub of
its own: the child runs on its parent's stack until it executes another
program, so the stub leaves nothing there, and keeps its return address
in a register across the call.

## A program's start

`execve` reads the executable's header and table of segments, attaches or
loads the code, makes each data segment and reads its contents, and
copies the arguments and environment to the top of the stack. The stack
is the size the executable gives with room for them added, rounded up to
a click, so that the size a program is linked with is its own need. The
program starts with:

| | |
|---|---|
| CS:IP | the entry point |
| DS, ES, SS | the data segment |
| SP | pointing at the count of arguments; after it the pointers to them, a null, the pointers to the environment and a null, each the size of a data pointer in the program's model |
| BX | the break |
| the rest | 0 |

A program's start-up code must leave the first bytes of its data segment
unused. An object at offset 0 would have the address a null pointer has,
and its address passed to the kernel would arrive as a null pointer.

## The process's memory, from the kernel

An address in a process is a selector and an offset, 32 bits, where the
kernel's own pointers are 16. Its type is `uaddr_t` (`sys/i286/seg.h`), a
far pointer that the kernel never dereferences itself.

`copyin` and `copyout` (`sys/i286/mch_copy.s`) do the work, and they
check nothing themselves:

- They set the two privilege bits of the selector to ring 3 before
  loading it. The processor then refuses any segment the process could
  not have loaded itself: every kernel segment, and everything else in the
  global table.
- The processor's own limit and write-protection checks catch a bad
  offset, a length that runs past the end, and a write to code.

Either way the result is a fault inside the kernel. Before touching the
process's memory the copy routines put a recovery address in `nofault`;
`trap`, on a fault in the kernel with `nofault` set, resumes there, and
the routine returns `EFAULT`. It costs the successful path two stores.

In `tests/user` a process passes every selector of the global table to
system calls that read and write through them, and all are refused. With
the one instruction that sets the privilege bits removed, the same test
has the process overwrite the kernel.

The kernel's side of a copy is a far pointer too, so the same routines
serve the kernel's data segment and the memory outside it; so are the
arguments of `bcopy`, `bzero` and `bcmp`. [The ABI notes](abi.md) say what
that costs in care.

## Slots: the user structure and kernel stack

A process's user structure and kernel stack are one piece of memory,
3,584 bytes: the structure, 878 bytes, at the bottom, and the stack
growing down toward it from the top. While the process runs, that piece
is in the kernel's data segment, in a *slot*, and `u` is `*uptr`, with
`uptr` pointing at it. The kernel has a few slots (6 as configured,
`sys/PC/param.c`). The first is the swapper's alone.

A kernel stack is full of pointers into itself and into the user
structure, so a process can only ever run in the slot it was given when
it was made. Processes that have different slots switch with no copying
at all. Processes given the same slot take turns in it: when one is to
run and another is there, the one there is copied out to memory of its
own (`p_addr`) and the one wanted is copied in, each no more than its
user structure and the part of its stack in use. A new process gets the
slot with the fewest processes.

`swtch` goes from one process to another by way of the swapper, as
2.11BSD does everywhere. That is what makes the copying safe: it is done
by the swapper, whose stack is in a slot nothing is ever copied over.

Switching is `setjmp` and `longjmp` (`sys/i286/mch_swtch.s`), which save
and restore SI, DI, BP, SP and a return address. `longjmp` to another
process changes `uptr`, the kernel stack pointer in the task state
segment and, if the process is not the last to have run, the local
descriptor table.

### A new process

A child cannot start on a copy of its parent's kernel stack, as it does
on the PDP-11, where every process's stack is at one address: the copy
would be in a different slot, and every pointer in it would point into
the parent's. So a child starts on a stack that holds one thing, a copy
of the frame its parent entered the kernel with. Its first act is
`childret` (`sys/i286/trap.c`), which puts `fork`'s result for a child in
that frame and returns through it. `newproc` therefore returns once, in
the parent.

The first process has no parent's frame. It is given `icode`, a few
instructions that execute `/etc/init`, as a code segment of its own.

## Swapping

A process is swapped as a whole, as in 2.11BSD: each of its own segments
is written to the swap device and its memory freed, and the code is
released if no other process in memory is using it. The user structure
and kernel stack stay in memory, 7 clicks a process, so the kernel always
has the process's table of segments to hand.

A process that cannot get memory to grow a segment, or to load a
program's code, swaps itself out and is brought back when there is room,
with the segment at its new size. `fork` without memory for the child
writes the child's segments straight to the swap device from the
parent's.

Memory is counted in clicks of 512 bytes, one sector of the swap device.

## Interrupts and their priority

The 286 has no interrupt priority levels of its own. `cpl` is a
variable; the `spl` routines change it and nothing else, which makes
them a handful of instructions. An interrupt request whose level is not
above `cpl` is put off: its line is shut at the interrupt controller and
noted in `ipending`, and its handler runs when `splx` or `spl0` lowers
the level far enough. The clock's deferred work (`softclock`) and the
network's software interrupt (`netintr`, [the network](network.md)) are
requested and run the same way. A device's handler is given to
`setirq`, which refuses a line that has one already; a handler in the
network, the Ethernet card's, is marked as `timeout`'s are, and called
through `KScall`, which gives it the network's data segment.

A process runs at level 0. The PDP-11 makes it so by restoring the PS on
the way back to user mode; here `userret`, which every way back to a
process passes through, lowers the level. Without that a process went on
at whatever level the kernel was at when it switched to it, which for a
new process or a preempted one was 7: every interrupt was put off until
the kernel next lowered the level, and the serial ports lost characters.

Handlers run on a stack of their own, so that a process's kernel stack
need not have room for them. On the way back to a process, if another
process should run, the kernel switches then.

## Counting

The kernel keeps upstream's counts (`UCB_METER`), which `vmstat` and
`iostat` read ([programs that read the
kernel](userland.md#programs-that-read-the-kernel)). Each tick of the
clock is charged to user time, nice time, system time or idle
(`gatherstats` in `sys/kern_clock.c`):

- a process's own code, at its nice, is user or nice time;
- the kernel with no process running, interrupted at level 0, which is
  the kernel waiting in `idle()`, is idle;
- anything else in the kernel is system time.

On the PDP-11 the level that was interrupted is in the processor status
the interrupt saved. Here `intr` notes it for the clock (`intrcpl`).
`idle()` sets `noproc`, which says that no process is running, as the
PDP-11's does. Before it did, the time the machine waited was charged
to process 0 as system time, and `ps` showed the swapper with all of it.

Traps and system calls are counted in `trap.c`, interrupts and the
software ones (the clock's and the network's) in `intr` and
`spllower`, context switches in `swtch`, and swaps where they happen.
The disk drivers count each transfer and how much it moved, and mark the
drive busy while it lasts, which the clock counts too (`dk_xfer`,
`dk_wds`, `dk_busy`, `dk_time`). The counts cost 472 bytes of the
kernel's data segment, and a little under 1.5K of code.

## Signals

The kernel continues a process in the trampoline code it named when it
installed the handler (its library's, for its memory model), with the
address of the handler in DX:AX and on the process's stack the signal's
number, a code, the far address of a `struct sigcontext`, and that
context: all the registers the process had. The trampoline calls the
handler and then `sigreturn` with the context.

`sigreturn` takes the registers back from the process, which may have
changed them. It refuses a context whose CS is not code of the process's
own, whose SS is not writable data of its own, or whose DS or ES is
neither null nor something it can read, and it does not let the flags
turn interrupts off or give the process the I/O ports.

## The 287

Its registers belong to the last process that used it. A process that is
switched to has the processor told to trap on the next 287 instruction,
unless it is that process, and the trap is where the registers are saved
for the one and loaded for the other. A process that never touches the
287 costs nothing at a switch.

## Not settled

- Where the stack sits in the data segment. It follows the data now, so a
  stack that overflows runs into the data without a fault. At the bottom
  of the segment an overflow would fault, and the break would be the end
  of the program's data, which the program knows; the kernel passes the
  break in BX only because, as things are, it does not. A program that
  takes the heap to start at `end`, as the PDP-11's did, finds its stack
  there instead: the shell did, and starts at the break now; `csh`, `c2`
  and `sendmail`, not built yet, do too. The arguments would have to go
  elsewhere, since a stack at the bottom cannot be made larger for them.
- More data segments for a program that asks for them, by a system call.
- A segment that is not in memory. The fault it causes, 11, is the hook
  for bringing one segment back instead of a whole process.
- 287 errors, which reach an AT-compatible machine as interrupt request
  13 and not as exception 16.
- Whether a process's kernel stack is big enough is not checked; nothing
  notices one that overflows into the user structure.
