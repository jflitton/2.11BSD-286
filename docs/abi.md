# Calling convention and kernel interface

Decided November 2026. The calling convention has been compiled and its
output read; none of it has been run yet.

## C calling convention

One convention for the kernel and for programs, set by a single pragma in
`tools/cc286.h` that `tools/cc286` gives to every compilation.

| | |
|---|---|
| arguments | on the stack, pushed right to left; the caller removes them |
| names | a leading underscore: `main` is `_main` |
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

Those are all that 338 compiled files of the tree referred to.

## System calls

A program enters the kernel with a software interrupt, the call number in
AX and the arguments on its own stack as for a C call. A 286 call gate was
the alternative; it can copy arguments to the kernel stack itself, but the
cost comes out within a few clocks of the interrupt (about 133 against
137 plus 4 for each argument word), and an interrupt frame is the simpler
thing for signals and restarted calls.

## User addresses in the kernel

The kernel's own pointers are 16 bits. A large-model program's pointers
are 32, a selector and an offset. The PDP-11 code uses one type,
`caddr_t`, for both kinds of address; here an address in user memory is a
type of its own, 32 bits wide, everywhere one flows: system call argument
structures, `copyin` and `copyout` and their relatives, `struct uio` and
`struct iovec`, and the pathname argument to `namei`.

A small- or medium-model program passes 16-bit data pointers. They are
widened with the process's data selector on entry to the kernel, so the
rest of the kernel sees one form.

This is done from the start. The kernel has 93 system call argument
structures and about 144 calls to the copy routines; introducing the type
later would mean touching each of them twice.
