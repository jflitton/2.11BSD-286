# Calling convention and kernel interface

Decided November 2026. The calling convention and the helper routines run
under the tests: `tests/crt` in real mode with near calls, `tests/pm` in
protected mode with far ones. The system call entry and the user address
type run under `tests/user`, with a stand-in for the kernel's system call
table; [processes](processes.md) describes them as built.

## C calling convention

One convention for the kernel and for programs, set by a single pragma in
`tools/cc286.h` that `tools/cc286` gives to every compilation.

| | |
|---|---|
| arguments | on the stack, pushed right to left; the caller removes them |
| names | a leading underscore: `main` is `_main` (the compiler treats `main` specially, so the pragma file names it too) |
| `char`, `int`, near pointer result | AX |
| `long`, far pointer result | DX:AX |
| `float`, `double` result | top of the 287 stack |
| structure result | the caller passes the address of a buffer in SI |
| preserved by the callee | SI, DI, BP, DS |
| free for the callee | AX, BX, CX, DX, ES |
| calls | near in small model, far in medium and large |

Why this one:

- The tree's old-style C calls functions with fewer or more arguments than
  they declare, and its `stdarg.h` and `varargs.h` step through the
  arguments in memory. Both need every argument on the stack and the caller
  to clean up.
- Open Watcom's own `cdecl` returns a `double` through a static buffer,
  which a signal handler that uses floating point would overwrite. The 287
  is required here, so results go on its stack.
- Open Watcom's register convention made kernel code 5.6% smaller but
  breaks the two habits above. It remains available for single kernel
  functions that are declared with prototypes, once a profile says which.

Known strictness: the compiler rejects a call whose argument count
disagrees with an old-style definition earlier in the same file, and an
old-style definition that disagrees with a prototype. Those are fixed in
the source.

## Helper routines

Compiled code calls these, and the system supplies them itself rather than
taking Open Watcom's library:

| routine | does |
|---------|------|
| `__I4M`, `__U4M` | 32-bit multiply |
| `__I4D`, `__U4D` | 32-bit divide and remainder |
| `__CHP` | round the top of the 287 stack toward zero, for conversion to an integer |
| `__PIA`, `__PTS`, `__PTC` | add to, subtract and compare `__huge` pointers |

Those are all that 338 compiled files of the tree referred to. The first
five are in `usr/src/lib/libc/i286/crt`, beside the PDP-11's `lmul.s` and
`ldiv.s` in `lib/libc/pdp/crt`; assembled with `-DFAR` they return far, for
the medium and large models. The `__huge` three are not written yet.

The compiler turns a call that is the last thing a function does into a
jump. Assembly that looks at its caller's return address sees the
caller's caller in that case.

## System calls

A program enters the kernel with a software interrupt, `INT 30h`: the call
number in AX and the arguments in BX, CX, DX, SI, DI and BP, a pointer
always as offset and selector. The result comes back in AX or DX:AX with
the carry flag clear, or an error number in AX with it set.
[Processes](processes.md) has the details.

Arguments were first left on the program's stack, as 2.11BSD has them on
the PDP-11, for the kernel to copy in. Registers are the better fit here:
the kernel gets the arguments for nothing, already in the frame it saves,
and it no longer needs to know a program's memory model to find them or to
widen a 16-bit pointer.

A 286 call gate was the alternative to the interrupt. It can copy
arguments to the kernel stack itself, but with arguments in registers
that buys nothing, and an interrupt frame is the simpler thing for signals
and restarted calls.

## What is kept from the PDP-11 system, and what is not

Kept, because code outside the machine-dependent layer depends on it:

- The system calls themselves, their numbers and what they do: the whole
  of the user-level source is written to them.
- The names and meanings of what the shared kernel code calls and
  includes: `copyin`, `copyout`, `spl`, `machine/seg.h` and the rest.
  Keeping them keeps that code close to upstream, whose patches are diffs
  to it.
- A symbol table `nlist` can read.

Not kept: anything that was the PDP-11's own answer to a hardware
question. Argument passing, the layout of a process's memory, the units
memory is counted in, the clock rate, the executable format and how the
kernel reaches memory outside its data segment are all decided for the
286. Where shared code assumes the PDP-11's answer, that code changes.

## User addresses in the kernel

The kernel's own pointers are 16 bits. A large-model program's pointers
are 32, a selector and an offset. The PDP-11 code uses one type,
`caddr_t`, for both kinds of address; here an address in user memory is a
type of its own, `uaddr_t`, 32 bits wide, everywhere one flows: system call argument
structures, `copyin` and `copyout` and their relatives, `struct uio` and
`struct iovec`, and the pathname argument to `namei`.

A small- or medium-model program's own data pointers are 16 bits. Its
system call stubs pass each with the data segment's selector, so the
kernel sees one form.

`uaddr_t` is a far pointer, not an integer. The compiler then keeps the
selector and offset apart, adds to the offset alone, and turns a kernel
address into one by assignment. The kernel does not dereference it:
`copyin` and `copyout` do, with the process's privilege.

This is done from the start. The kernel has 93 system call argument
structures and about 144 calls to the copy routines; introducing the type
later would mean touching each of them twice.
