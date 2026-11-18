# Processes and the kernel

How a process is confined to ring 3, how it enters the kernel, and how the
kernel reaches its memory. Running under `tests/user`, with one process
and a stand-in for the kernel's `trap` and `syscall`; fork, exec, signals
and switching between processes are not written yet.

## A process's segments

Each process has a local descriptor table. Segment N of its executable is
slot N, so its selector is `(N << 3) | 7`, the value the linker step wrote
into the program ([executable format](executable-format.md)). A small-model
program runs with CS = 0x0F and DS = SS = ES = 0x17.

Code segments are ring 3, readable, not writable. Data segments are ring 3
and writable. A descriptor's limit is the segment's exact size. The global
table has one slot, `GDT_LDT`, that names the running process's table.

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
kernel fills in the frame as if the process had been interrupted at its
first instruction, and returns through it. `main` does that for the first
process, and the start-up code resumes the frame when `main` returns.

`USERMODE(tf->tf_cs)` says whether a frame is a process's.

## System calls

```
        mov     ax, number
        mov     bx, first argument word     ; then CX, DX, SI, DI, BP
        int     0x30
```

Arguments are in registers, a word each, in the order BX, CX, DX, SI, DI,
BP. A `long` is two words, low word first. A pointer is two words, offset
and then selector, in every memory model: a small-model program's stub
passes DS as the selector. The kernel therefore sees one argument layout
and never needs to know a program's model to find or widen an argument.

`mch_trap.s` pushes the registers one at a time so that those six are
next to each other in the frame, in argument order. A system call's
argument structure is laid over them where they are: nothing is fetched
from the process's stack and nothing is copied. Next to `PUSHA` and
`POPA` that costs about 20 clocks a trap; fetching three words from the
process's stack with `copyin` costs several times that on every call.
(Both from the manual's clock counts, not measured.)

A call with more than six words of arguments takes one argument instead,
a pointer to them, and the kernel copies them in. Of 2.11BSD's 139 system
calls, 11 have four or more arguments, and those are the candidates.

The kernel returns with the carry flag clear and the result in AX, or
DX:AX for a `long` or a second value; or with carry set and the error
number in AX, which the stub stores in `errno` before returning -1. All
other registers come back unchanged, except that the kernel may alter the
ones that carried arguments; a stub puts back SI, DI and BP, which belong
to its C caller.

The gate leaves interrupts on. It is the only gate besides 3 (breakpoint)
and 4 (`INTO`) that a process may use.

libc's stubs differ by memory model, as libc itself does: where the
arguments are on the stack, and whether a pointer there is one word or
two. They are regular enough to generate from a table of each call's
argument types.

## The process's memory, from the kernel

An address in a process is a selector and an offset, 32 bits, where the
kernel's own pointers are 16. Its type is `uaddr_t` (`sys/i286/seg.h`), a
far pointer that the kernel never dereferences itself. A kernel address
converts to one by assignment, so one field can hold either, which
`struct uio` needs.

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

## Not settled

- Where local descriptor tables live. In the test it is an array in the
  kernel's data, which at 8 bytes a segment for every process would crowd
  the 64K. A table can be a segment of the process's own memory instead.
- Pointers inside structures a program hands the kernel: `struct iovec`,
  `struct msghdr`, the argument and environment arrays of `execve`, the
  handler in `struct sigaction`. A small-model program's are 16 bits.
  Either libc converts them before the call, or the kernel reads them
  according to the model in the executable's header.
- A segment that is not in memory. The fault it causes, 11, is the hook
  for bringing a swapped-out segment back, in the process and in
  `copyin` and `copyout` alike.
- What the kernel must check in a frame it did not build itself, when a
  signal handler returns.
- The 287's registers at a switch between processes. The task-switched
  flag and exception 7 are what will save and restore them only for the
  processes that use it; `tests/pm` exercises that trap.
