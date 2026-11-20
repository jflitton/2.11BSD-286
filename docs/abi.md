# Calling convention and kernel interface

Decided November 2026. The calling convention and the helper routines run
under the tests: `tests/crt` in real mode with near calls, `tests/pm` in
protected mode with far ones. The system call entry and the user address
type run in the kernel itself, under `tests/kernel`;
[processes](processes.md) describes them as built.

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
| structure argument | on the stack, whole, filled out to a word |
| structure result | of 1, 2 or 4 bytes, in AL, AX or DX:AX; any other, the caller passes the address of a buffer in SI, which is not preserved by that call |
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

A plain `char` is signed (`-j`). Open Watcom's own default is unsigned,
and the tree keeps small negative numbers in `char`s: with the default,
the kernel compiled without complaint, and an `open` of a device that was
not there returned a file descriptor.

Known strictness: the compiler rejects a call whose arguments disagree in
number or type with an old-style definition anywhere in the same file,
before the call or after it, and an old-style definition that disagrees
with a prototype, even only in a `const`. Those are fixed in the source;
[the C library and programs](userland.md) says how, for user space.

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
the arguments are already in the frame the kernel saves, six words to
move and nothing to fetch from the process, and the kernel no longer
needs to know a program's memory model to find them or to widen a 16-bit
pointer.

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

## Far pointers in the kernel, and what the compiler does not check

The kernel is compiled with near data pointers, and a far pointer is the
exception: an address in a process, a buffer's data (`mapin(bp)` is a far
pointer kept in the buffer's header; nothing is mapped), a table that was
moved out of the kernel's data segment. Old-style C gives the compiler
little to check such a mixture with, and three things turned out to
matter.

- A function that takes a far pointer must be declared, with a
  prototype, wherever it is called. Without one the caller pushes a near
  pointer as two bytes where the function expects four. The copy routines
  and `bcopy`, `bzero` and `bcmp`, which take far pointers so that one
  routine serves memory inside and outside the data segment, are declared
  in `machine/seg.h`, which `param.h` includes for every file.
- The compiler warns when a far pointer is narrowed implicitly, and the
  kernel's Makefile makes that warning an error. It says nothing about a
  cast, and the tree is full of `(caddr_t)` casts that were harmless when
  every pointer was 16 bits. Nor does it check a far argument to a
  function with no prototype. `tools/farlint.py` looks for both by
  pattern. In the network, whose data segment is not the stack's, the
  address of a local is itself far, and `tools/netcheck.py` keeps count
  of the places that make it near ([the network](network.md)).
- A based pointer is not checked at all. The terminals' character
  blocks are in a segment of their own, and a pointer to one is
  `CLSPACE`, 16 bits that are an offset in that segment
  (`machine/machparam.h`). Assigned to an ordinary pointer, which is
  also 16 bits, it draws no warning, and then points into the kernel's
  data. Such pointers are kept to `sys/tty_subr.c` and three loops in
  `sys/tty.c`, as the PDP-11's `UCB_CLIST` kept them. An array inside
  what one points at is far, not based, and wants a cast back.
- `sizeof` a pointer is no longer `sizeof (int)`. One system call copied
  out `sizeof (uap->status)` bytes and so wrote four bytes over a
  two-byte variable in the process.

Two more things are the compiler's own:

- It has no common storage. A variable declared in a header without
  `extern` is defined in every file that includes the header, the linker
  keeps the first definition and warns, and the first may be an empty
  one while a later one has the value. The kernel's headers now say
  `extern`, the variables are defined once (most in
  `sys/sys/init_main.c`), and the Makefile makes the linker's warning an
  error. User space, with far more such headers, has common storage
  added instead: `tools/commonize.py` makes each variable without a
  value an OMF communal as each file is compiled
  ([the C library and programs](userland.md)).
- A null near pointer converted to far is a null far pointer, not offset
  0 of the data segment. So the address of an object at offset 0 of a
  program's data segment cannot be passed to the kernel, and a program's
  start-up code keeps the first bytes of that segment unused
  ([processes](processes.md)).
