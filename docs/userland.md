# The C library and programs

How the system's programs are built for the 286, and what the tree's code
needed to build and run here. All of it runs in the emulator under
`tests/libc` and `tests/system`, the second of which boots the system from
a hard disk to multi-user and logs in on the console.

## Building

```sh
. tools/env.sh
make -C usr/src/lib/libc/i286   # crt0.o and libc.lib in obj/lib/s and obj/lib/m
make -C usr/src/i286            # the programs, each in obj/bin
make -C usr/src/i286 root       # obj/root/root.fs, a root file system
```

The root file system needs the kernel and `/boot` made first
(`usr/src/sys/PC`, `usr/src/sys/i286stand`).

`tools/cc` is the cross build's `cc`: it compiles each `.c` with
`tools/cc286`, assembles each `.s` with NASM, and links behind `crt0.o`
with `libc.lib` last, converting the result to the system's format with
`tools/ne2aout.py`. It takes `-c`, `-o`, `-D`, `-I`, `-U`, `-L`, `-l`,
`-s` and `-stack N`, and ignores the PDP-11's `-i` and `-n`.

`-mm` makes a medium-model program, and `-ms`, the default, a small one
([memory model](memory-model.md)). The medium model's code is far, in as
many segments as it takes, where the PDP-11 would have overlays; its data
is the small model's, near and in one segment. Each model has its own
library, in `obj/lib/s` and `obj/lib/m`. The linker would take objects
of the two models together without complaint, and the program would
fail as it ran, so `tools/cc` reads the model the compiler records in
each object and library and refuses a mixture. The linker packs the code
of a medium-model program into segments of nearly 64K; `-packcode N`
makes them N bytes at most.

The programs are listed in `usr/src/i286/Makefile` with their sources;
each is compiled and linked in one run of `tools/cc`, and what the
compiler says goes to `obj/bin/log`. Upstream's own Makefiles are not
used: they install as they go, with the PDP-11's tools.

## The library

The tree's portable C, as `lib/libc`'s Makefiles list it, with
`lib/libc/i286` in place of `lib/libc/pdp`. It is built twice, once for
each model: the C compiled with `-ms` or `-mm`, the assembly assembled
with `-DFAR` for the medium model (`crt/DEFS.inc` gives the return, the
call and where the arguments start), and the stubs made for it.

| | |
|---|---|
| `csu/crt0.s` | start-up: `main(argc, argv, envp)`, `environ`, `__progname`, and where the break is, which the kernel passes in BX. The first bytes of the data segment are left unused, so that nothing has the address a null pointer has. |
| `sys/syscalls.tab` | a line for each system call: `tools/mkstubs.py` writes a stub for each, one object apiece so that a program gets only the calls it makes. The Makefile has it check each line against the kernel's own table (`sys/sys/init_sysent.c`): the stub must pass as many words as the kernel takes. |
| `sys/*.c`, `sigtramp.s` | the calls that need more than a stub: `brk` and `sbrk`; `execl`, `execle`, `execv`; `setlogin`; `sigaction`, with the trampoline it names to the kernel, which gets the handler as a far address (in the small model the library adds CS); `readv` and `writev`, `sigaltstack` and `sigstack`, whose structures hold pointers, which are far to the kernel and near in a small- or medium-model program. |
| `gen/setjmp.s` | `setjmp`, `longjmp`, `_setjmp`, `_longjmp`. A `jmp_buf` is a `struct sigcontext`, 17 words here (`setjmp.h`); `longjmp` has the kernel resume it with `sigreturn`, which restores the signal mask and every register at once. In the medium model the context's CS is the return address's. |
| `gen/ldexp.s`, `modf.s` | on the 287 |
| `net/htons.s` | byte order |
| `crt/` | the routines compiled code calls for 32-bit arithmetic ([the ABI notes](abi.md)) |

What `lib/libc/pdp/gen` has in assembly (the string routines, `bcopy` and
the like, `frexp`) comes from the C in `lib/libc/gen` that it stood in
for, and its `atof.c`, `ecvt.c` and `setjmperr.c` are used as they are.

Left out:

- `gen/fakcu.c`, an empty `_cleanup` for `exit` to call in a program
  without stdio. It works by the order of the PDP-11's library, which `ld`
  reads once; this linker took it in place of stdio's, and output was not
  flushed at exit. Every program has stdio's now, about 650 bytes.
- `stdio/fpfcom.c`, printf's floating point for a program that has none.
  The PDP-11's compiler marks a program that uses floating point with a
  reference to `fltused`, which brings in the real `stdio/pfcom.c`; this
  one makes no such mark, so every program that formats has the real
  one, about 2K.
- `nlist`, which reads the PDP-11's a.out format and not this system's,
  so `ps` and the like wait for it; profiling; `recvmsg` and `sendmsg`,
  whose structures need converting and whose kernel side is not written;
  and the PDP-11's `compat-2.9`.

The system's error messages are in `/etc/syserrlst`, not in each
program's data, as on the PDP-11. `tools/mkerrlst.py` makes the file from
`usr.lib/liberrlst/errlst.c`.

Headers changed for the 286: `float.h` is new, for the 287
(`sys/i286/float.h`); `setjmp.h` sizes a `jmp_buf`; `stdlib.h` gives
`alloca` in line, which the compiler handles by taking the stack pointer
back from the frame pointer; `netinet/ip.h`, `netinet/tcp.h` and
`arpa/nameser.h` name the 286 among the little-endian machines.

## What this compiler needed

**Common storage.** Old C declares variables in headers without
`extern`, so that every file that includes one defines them, and relies
on the linker to make each name one variable, with the value from
whichever file gives it one. Open Watcom makes each uninitialized
variable an ordinary definition, and the linker keeps the first it meets.
In the shell that was an empty copy of its table of character classes.
`tools/commonize.py` rewrites each object as it is compiled (in
`tools/cc`, and in the library's Makefile): every public variable without
a value becomes an OMF communal, which the linker merges with the others
of the name and with the one that has a value. The compiler refers even
to a file's own variables through its list of external names, so moving
the name moves every reference. After that, a variable still defined
twice is an error at link time.

**Its own keywords.** Old programs use `inline`, `near`, `far`,
`interrupt` and others as names. The first is a keyword here and the rest
are macros the compiler predefines; `tools/cc` undefines and renames
them. The system's headers write the keywords with two underscores when
they mean them.

**Strictness.** The compiler rejects, as errors:

- an old-style definition that disagrees with a prototype, even in a
  `const` or in a parameter narrower than an `int`;
- a call that disagrees in its number or types of arguments with an
  old-style definition anywhere in the file, before the call or after it;
- a function used before it is declared `void` and then defined so.

The usual case of the second is a `fatal(status, fmt, a)` called with
more or fewer arguments; those became variadic, with `vfprintf`. A few of
the disagreements were real bugs on any 16-bit machine, the PDP-11
included: `dd` passed an `int` where `number()` takes a `long`, the XNS
address parser passed an `int` where `cvtbase()` takes a `long`, and
`srandom` was defined with a `long` and declared with an `unsigned`.

**PCC's member names.** PCC let a structure member's name be used with a
pointer to any structure, and the Bourne shell reaches every kind of
parse-tree node through one pointer type. Its proforma node now holds
each kind's members at their own offsets, in anonymous structures within
a union (`bin/sh/mode.h`); the shell's few other such uses have casts.

## What the PDP-11 let programs assume

- **That address 0 can be read.** `getty` passed 0 for the buffer of an
  `ioctl` that reads an `int`; it gets a pointer to a zero now.
- **That the heap begins at `end`.** On the 286 a program's stack lies
  between its data and the break ([processes](processes.md)). The shell
  started its arena at `end` and so on top of its stack; it starts at the
  break now. `csh`, `c2` and `sendmail` do the same and are not built yet.
- **That the network calls exist.** In a kernel without networking they
  end a process with SIGSYS. `syslog` has a fallback for that, writing
  to `/usr/adm/messages` and then the console, which was for the PDP-11
  only and is for the 286 too.
- **That the password database exists.** With only the flat
  `master.passwd`, `getpwent` took the password from the start of the
  file, so root's was "root". It was meant for the ndbm files, where the
  password's place holds an offset; it is used only for those now.
- **That only the PDP-11 has a 16-bit `int`.** The tree's `#ifdef pdp11`
  sometimes means that, and its other branch was written for a 32-bit
  `int`. `rand` returned its generator's low 16 bits, the poorest and
  half the time negative, where `RAND_MAX` is 0x7fff. It and `valloc`,
  for which a page boundary means no more in a segment than on the
  PDP-11, take the PDP-11's branch now, as `syslog` does. The programs
  have many more such conditionals, to be read as each is built.
- **That `/etc/autoconfig` exists.** On the PDP-11 `init` runs it to find
  the devices; this kernel finds its own, and `init` no longer runs it.

The kernel changed for two of these: without quotas compiled in, the
quota calls fail with EINVAL, which `login` looks for, rather than end
the caller with SIGSYS; and `execve` gives the arguments and environment
room above the program's stack, so that the stack size a program is
linked with is its own (8K by default).

## The root file system

`usr/src/i286/root.proto` lists it: the kernel and `/boot`, `/etc/init`,
`getty`, `login`, the shell and the programs, the devices, and `/etc`:
upstream's `gettytab`, `master.passwd`, `group` and `shells`; this port's
own `rc`, `ttys`, `fstab` and `motd` (`usr/src/i286/etc`); and `passwd`
and `syserrlst`, which the Makefile makes. Upstream's `rc` is the
PDP-11's, with its buses and network interfaces, and calls much that is
not built yet.

The programs: `init`, `getty`, `login`, `sh`, `ls`, `cat`, `cp`, `mv`,
`rm`, `ln`, `mkdir`, `rmdir`, `chmod`, `chgrp`, `cmp`, `dd`, `df`, `du`,
`echo`, `ed`, `grep`, `kill`, `nice`, `od`, `pr`, `pwd`, `stty`, `sync`,
`tee`, `test` (and `[`), `time`, `who`, `wall`, `write`, `date`,
`hostname`, `hostid`, `pagesize`, and in `/sbin` `mount`, `umount`,
`mknod`, `reboot` and `halt`.

## Following a program

The kernel's `sctrace`, set with `tools/kpatch.py obj/PC/unix sctrace=1`,
prints on the console each system call that fails, each call that is not
in this kernel, each program executed, each exit and each fault, as
`login[7]: call 149: error 22`. With 2 it prints every call.

## Tested, and not

`tests/libc` boots the kernel with a test program as `/etc/init` on a
RAM disk. The program is built for both models; the small-model one is
`init`, and when it is done it runs the medium-model one, whose code is
packed into 8K segments so that it has four of them. Each checks, 73
times: printf and scanf with integers, strings
and floating point; files through stdio, with seeks; the string
routines; `atoi`, `strtol`, `atof`, `modf`, `ldexp`, `frexp`, `qsort`;
`malloc`, `realloc`, `calloc` and the break; `setjmp` and `longjmp`, out
of a deep call and out of a signal handler; `sigaction`, blocked signals,
`alarm` and `pause`, `select` and `sleep`; `fork`, `wait`, `execl` of a
second program of the same model through a pipe, and `vfork`; `readv`
and `writev`; the password and
group files, `gmtime` and `ctime`, directories, `errno` and `strerror`,
`setenv`, `getopt`, `isatty` and `ttyname`, and `rand`.

`tests/system` builds everything as for a real machine, puts the root
file system and a swap area on an IDE disk shared by a partition table,
and boots it with no diskette. The system goes multi-user, runs `/etc/rc`
and `getty`; the test logs in as root, and in the shell makes a
directory, copies and compares a file, runs a loop, a second shell and a
pipeline, expands a pattern, lists `/etc`, runs `who`, and halts the
machine with `reboot -h`. Afterwards `tools/fsck.py` must find nothing
wrong with the file system and must find the copied file.

Not done, or not tested:

- The large model has no library, and the three helpers for `__huge`
  pointers are not written.
- No program the system builds is medium-model yet. The 15 that the
  PDP-11 overlays (`csh`, `ex` and `vi`, `sendmail` and others) are the
  ones meant for it; `usr/src/i286/Makefile` will need a way to name a
  program's model, and `libutil` a medium-model build.
- Most programs are not built: none of `/usr/bin` or `/usr/ucb` (`vi`,
  `more`, `csh`), and no `fsck`, `mkfs`, `ps`, `mkpasswd`, `passwd`,
  `cron`, `syslogd` or `update`. Nothing checks the file systems at boot.
- A program that overflows its stack runs into its data without a fault
  ([processes](processes.md)).
- With every call printed (`sctrace=2`), the kernel's printing can stall
  the console's interrupt-driven output, and the terminal stops.
- A user other than root, a password, and logging in anywhere but the
  console have not been tried.
- All of it has run in the emulator only.
