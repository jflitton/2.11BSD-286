# Memory model

How programs and the kernel use the 286's segments. Decided November 2026;
the numbers are from compiling this tree with Open Watcom and from the clock
counts in Intel's *80286 and 80287 Programmer's Reference Manual*. The
kernel described here runs, but nothing has been timed, so the
performance figures are still estimates.

## What the 286 charges for

Protected mode, clocks (100 ns each at 10 MHz):

| operation | clocks |
|-----------|--------|
| load a word from memory | 5 |
| load a segment register | 17 to 21 |
| near call and return | 18 |
| far call and return | 51 |
| `rep movsw` | 5, plus 4 a word |
| trap from user to kernel, and return | 78 + 55 |

Crossing a segment costs three to four times what staying inside one does.
The design below keeps the common paths inside one code segment and one
data segment, and spends segment loads only where data is too big to live
there.

## User programs

A process is a list of segments in its own local descriptor table, not the
PDP-11's fixed text, data and stack. Each segment is at most 64K, is swapped
on its own, and code segments are shared between processes running the same
program.

| model | code | data | pointer sizes | replaces |
|-------|------|------|---------------|----------|
| small | one segment | one segment, holding data, heap and stack | both 16 bits | PDP-11 separate I and D |
| medium | many segments | one segment | function 32, data 16 | PDP-11 overlays |
| large | many segments | many segments; one object at most 64K | both 32 bits | nothing: this is the new capability |

Rules:

- A program states its model in its Makefile. Small is the default. The
  executable records the model, and the linker refuses to mix objects or
  libraries of different models.
- Models arrive in that order. Small first, because every existing program
  works in it unchanged. Medium next: 15 programs in the tree are linked
  with overlays today (csh, ex, sendmail, f77, named and others); csh, ex
  and tcsh are built so. Large last.
- Moving a program to large is a deliberate change made after cleaning it
  up. Old C that passes a bare `0` for a pointer, keeps a pointer in an
  `int`, or calls an undeclared function that returns a pointer compiles
  without complaint in large model and then fails.
- Objects over 64K are available through the `__huge` keyword in a
  large-model program, not as a model of their own.
- A system call hands a process extra data segments, so that a small-model
  program can keep one big buffer behind `__far` pointers without being
  converted.
- libc is built once per model.

## The kernel

Far code, near data: many code segments, which replace the PDP-11 kernel's
overlays, and one 64K data segment, so that a pointer still fits in an
`int` and the machine-independent C compiles largely as it stands. A
kernel with far data everywhere was rejected: every pointer and table would
double in size, and all of that C would need auditing.

Things too big for the kernel's data segment get segments of their own and
are reached through based or far pointers: the buffer cache's data, user
memory, the terminals' character blocks, and any table grown past its
PDP-11 size. The PDP-11 kernel already
treats buffers this way, through `mapin` and `mapseg5`, so this follows the
code's existing shape.

Networking keeps a data segment of its own. On the PDP-11 it runs in
supervisor mode in a separate address space, and the kernel and network
code call each other through `KScall` and `SKcall`. Here each of those
becomes a far call that switches the data segment, and the network runs
on the kernel's stacks, reaching the kernel's data through SS
([the network](network.md)).

### The user area and kernel stack

The PDP-11 remaps a page so that `u` and the kernel stack of the running
process are always at one address. The 286 cannot remap part of a segment,
and the compiler needs the stack inside the data segment so that the
address of a local variable is an ordinary pointer.

Three ways to give each process its own `u`, measured on the 54
machine-independent kernel files that compile today:

| `u` is | code size | segment-register loads in the code |
|--------|-----------|-------------------------------------|
| a fixed global, copied out and in at every switch | 47,244 | 20 |
| reached through a near pointer | 49,168 | 20 |
| in a segment of its own | 50,434 | 399 |

`struct user` is 928 bytes. Copying it and a few hundred bytes of stack
both ways is about 5,800 clocks, 0.6 ms, per switch. That is too much here:
the expected load is telnet sessions, where `telnetd` and the user's
program trade the processor for every chunk of input or output.

A segment of its own was rejected. It adds a segment load at about 380
places on the most-used structure in the kernel, it turns every `&u.u_x`
passed to a function into a bug (the compiler flagged 24), and it still
leaves the stack to copy.

The design is **slots**:

- `u` is a macro for `*up`, where `up` is a near pointer. None of the
  references to `u` change.
- The kernel's data segment holds several slots, each with one process's
  user area and kernel stack: 3,584 bytes as built, 878 of them the user
  area.
- A process is given a slot when it is made and runs only there, because
  its kernel stack holds pointers into itself. Processes with different
  slots are switched between by changing `uptr` and the stack pointer.
  Nothing is copied.
- Processes that share a slot are copied in and out of it as they take
  turns, each to and from memory of its own, and only the user area and
  the used part of the stack.
- The number of slots is a parameter of the kernel (6 as configured, one
  of them the swapper's).

[Processes](processes.md) describes it as built.

Slots compete with the process, inode and file tables for the 64K. Each
active telnet user keeps two processes busy, so the count wants to be about
twice the number of users working at once.

## Assembly

The machine-dependent layer is assembly, as it is on the PDP-11 (6,800
lines there): traps, system call entry, context switch, copies between
kernel and user, character queues.

Beyond that, these machine-independent paths get assembly versions, each
with its C version kept beside it as the reference:

- the network interface's packet transfer, using `rep insw` and `rep outsw`;
- the Internet checksum;
- copies of packet and socket buffers;
- the terminal line discipline's per-character path, which pseudo-terminals
  share;
- pathname component scanning and directory search;
- buffer cache hash lookup.

More is added only where a profile shows the need, because a file rewritten
in assembly stops receiving upstream's patches, which are diffs to the C.

Two structural measures matter as much:

- Hot paths are grouped into one code segment and call each other near.
  The 54 files above contain 804 far calls; each costs 33 clocks more than
  a near one.
- Open Watcom's register calling convention made the same code 5.6%
  smaller. It is safe only for functions declared with prototypes, so it is
  a candidate for chosen kernel functions, not a default.

An idea set aside until the plain port can be measured: joining a socket to
a pseudo-terminal inside the kernel, which would take `telnetd` and its
two context switches per character out of the path.
