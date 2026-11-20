# The C library and programs

How the system's programs are built for the 286, and what the tree's code
needed to build and run here. All of it runs in the emulator under
`tests/libc` and `tests/system`, the second of which boots the system from
a hard disk to multi-user and logs in on a serial port and on the
console's screens.

## Upstream's programs, and newer ones

The programs are upstream's, kept as 2.11BSD has them, so that each
behaves as its manual page says and the system stays one to learn
2.11BSD from. What changes in them is what the port needs: bugs, most
of them an assumption that a 16-bit `int` breaks ([what the PDP-11 let
programs assume](#what-the-pdp-11-let-programs-assume)); what this
compiler needs ([below](#what-this-compiler-needed)); and the settings
that upstream leaves to each machine, such as the sizes of `ex`'s
buffers in `ex_tune.h`, which already differ between the PDP-11 and
the VAX. Configuration and data, the termcap, `/etc` and root's files,
are this system's to set up.

What a user now expects and upstream's program does not do goes in a
program of this port's own, beside upstream's rather than in its place,
so that both are there. Each is in a directory of its own in
`usr/src/i286`, and the Makefile lists them as `OWNPROGS`. The first is
`adduser`. 2.11BSD has `adduser(8)` only as a manual page that says how
to add a user by hand; `adduser` asks for the login name, full name,
group, shell, home directory and uid, offering `staff`, `/bin/csh`,
`/home/name` and the lowest uid from 100 that nobody has; has upstream's
`chpass -a` add the entry, so that the password file is locked and its
database made again as `vipw` does; makes the home directory with copies
of the files in `/usr/skel`, all the user's; and runs `passwd`. The
entry is added with the password `*`, which nothing matches, so that the
account cannot be used until `passwd` has given it one. It is in C
rather than the shell: the Bourne shell has no arithmetic, to find a
free uid with, and would run `expr` for each sum.

The second is `bench`, a benchmark, which 2.11BSD has none of: 18
tests, each a fixed amount of one kind of work, of the processor, system
calls, pipes, `fork` and `exec`, the file system, a pseudo-terminal,
TCP and users logged in by telnet, timed by the system's clock ([performance](performance.md)).

The next planned is an editor in the manner of `vim`, beside
`vi`: one that fills a window of any size, where `vi` refuses a window
bigger than its copy of the screen ("Screen too large for internal
buffer"); whose Home and End go to the start and end of the line, where
`vi`'s Home goes to the top of the screen and it knows no End; whose
Page Up, Page Down, Delete and Insert work with no `.exrc`; and which
knows the cursor keys in insert mode too.

## Building

```sh
. tools/env.sh
make -C usr/src/lib/libc/i286   # crt0.o and libc.lib in obj/lib/s and obj/lib/m
make -C usr/src/i286            # the programs, each in obj/bin
make -C usr/src/i286 root       # obj/root/root.fs, a root file system
make -C usr/src/i286 root STRIP=no   # the same, the programs with their symbols
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

The programs are listed in `usr/src/i286/Makefile` with their sources,
and with `NAME_MODEL = m` for one that is medium-model; each is compiled
and linked in one run of `tools/cc`, and what the compiler says goes to
`obj/bin/log`. `libutil`, `libtermcap`, `libm`, `libcurses` and `libwrap`
are built for each model a program wants, in `obj/lib/s` or
`obj/lib/m`. Upstream's own
Makefiles are not used: they install as they go, with the PDP-11's
tools. The build also needs a C compiler for the machine it runs on,
`cc` unless `HOSTCC` names another, for the tree's `yacc` and `lex`
([grammars](#grammars-yacc-and-lex)).

Four programs are medium-model so far: `csh`, `ex` and `tcsh`, here,
and `sendmail` ([mail](#mail)). `csh` was linked with two
overlays on the PDP-11; here its 67K of code is in two segments, and its
data segment has 11K of data and 8K of stack, leaving about 45K for its
heap. `ex`, which is also `vi`, had two overlays as well. Its 77K of
code is in two segments, and its data segment has 35K of data and 8K of
stack. That leaves about 21K for the table of lines, two bytes a line,
or 10,000 lines. Of the data, 16K is `vi`'s copy of the screen, three
times the PDP-11's 5,600 bytes, since a window that `telnet` logs in
from is often far bigger than a terminal was: it holds 200 columns by 80
lines, and the widest window is 256 columns. `vi` refuses a bigger
window, as upstream's does, with "Screen too large for internal buffer".
Its error messages are in `/usr/share/misc/exstrings`, as on the PDP-11,
not in its data. `tools/mkstr.py` does what `mkstr` did: it takes each
message out of a copy of the C, in `obj/ex`, and puts the message's
offset in the file in its place. That moves 5.7K out of the data
segment. `/usr/share/misc/termcap` is upstream's, with the commonest
terminals moved to the front, as its Makefile has `ex` do;
`tools/mktermcap.py` runs the same editor commands.

`vi` knows the arrow keys and Home from the termcap, and maps Home to
`H`, the top of the screen; upstream's xterm entry did not give Home,
and now does. Root's `.exrc` (`usr/src/i286/root`) maps End to `L`, the
bottom of the screen, Page Up and Page Down to ^B and ^F, Delete to `x`
and Insert to `i`, with `vi`'s own `map`, as an xterm and the console's
screens send them. Without such maps those keys' escape sequences are
commands to `vi`: Home and End open a line, and Delete and the page keys
change the case of a character.

`tcsh` had seven overlays. Its 127K of code is in two segments, and its
data segment has 26K of data and 8K of stack, leaving about 30K for its
heap. Its own `mkprstr` took the format of each `xprintf` out of its C
and into `/etc/tcsh6.00strgs`, and made the call one to `mkprintf`,
which reads the format from the file; `tools/mkstr.py` does that too
(`-e xprintf=mkprintf -n 5`), and moves 6.5K. Three of its headers are
made from its C, as its Makefile makes them: `sh.err.h` with the
16-bit flags of its error numbers, which its Makefile chose with
`unifdef -Dpdp11`.

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
- Profiling; `recvmsg` and `sendmsg`, whose structures need converting
  and whose kernel side is not written; and the PDP-11's `compat-2.9`.

`nlist` is in it, reading this system's format ([programs that read the
kernel](#programs-that-read-the-kernel)).

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
twice is an error at link time. What is left in the file's `_BSS` is its
static variables, and they are moved down to its start, with `_BSS` made
only as long as they need: otherwise every file keeps room for every
variable its headers define. That was 28K of the 64K data segment of
`csh`, whose `sh.h` defines 1.3K of variables and is included by 19
files, and 6K of the shell's. The compiler refers to a static variable by
name too, or, for one declared in a function, by its offset in `_BSS`,
which is in the instruction or data the reference is in; that offset is
changed as the variable moves.

The kernel is linked without communals, and the headers it shares with
programs declare its variables `extern` where upstream's defined them,
with each defined once in the kernel. A program that relied on a header
for a variable of its own defines it now: `netstat` read the kernel's
count of open files into the `nfile` that `sys/file.h` gave it.

**Its own keywords.** Old programs use `inline`, `near`, `far`,
`interrupt` and others as names. The first is a keyword here and the rest
are macros the compiler predefines; `tools/cc` undefines and renames
them. The system's headers write the keywords with two underscores when
they mean them. `_leave`, `_try`, `_except` and `_finally` are predefined
too, for the compiler's exception handling, and undefined the same way:
`curses.h` has a member `_leave`, so no program that includes it
compiled.

**Strictness.** The compiler rejects, as errors:

- an old-style definition that disagrees with a prototype, even in a
  `const` or in a parameter narrower than an `int`;
- a call that disagrees in its number or types of arguments with an
  old-style definition anywhere in the file, before the call or after it;
- a function used before it is declared `void` and then defined so,
  as `sa`'s `usage` was, which is declared first now.

The usual case of the second is a `fatal(status, fmt, a)` called with
more or fewer arguments, as in `chown`, or `fsck`'s `errexit`, `pfatal`
and `pwarn`; those became variadic, with `vfprintf` or `vprintf`, and
`csh`'s `error` with a `vprintf` of its own. `ex`'s `error` takes a
message and sometimes an argument, an `int` or a pointer, so it is
variadic too. Where only trailing arguments that go unused were left
out, as in `csh`'s `search` and `execute`, `telnet`'s `printoption`
and `dosynch` and `find`'s `cpio`, the calls pass them now; `telnet`'s `call`, which walks
its arguments to the null that ends them, is variadic. `mkfs` passes a
block of inodes to routines that take a `char *`, and casts it now. A
few of the disagreements were real bugs on any 16-bit machine, the
PDP-11 included: `dd` passed an `int` where `number()` takes a `long`,
the XNS address parser passed an `int` where `cvtbase()` takes a `long`,
and `srandom` was defined with a `long` and declared with an `unsigned`.
`ex`'s `cerror`, for a bad regular expression, had a real bug of its own
on the PDP-11. It took the message as a string, but `mkstr` gives it the
message's offset in the file. It printed that offset with `"%s"`, which
printed whatever was at that address in its data. It passes the offset
on to `error` now, as the other callers do.

The compiler also takes any macro named `va_start` to be the variadic
one, and refuses it in a function not declared with `...`. So a function
written with `varargs.h`, whose `va_alist` is an ordinary parameter, is
rewritten with `stdarg.h`; `ex`'s `printf` was the first.

**Prototypes.** Code written for both kinds of compiler, as `tcsh` is,
declares its functions with `__P((...))`, which keeps the prototype
where `__STDC__` is true and drops it for the PDP-11's compiler. This
compiler has `__STDC__`, so the prototypes are kept, and are held to as
the PDP-11's never were. Three of `tcsh`'s signal handlers were
declared to take nothing, and the compiler will not pass a function of
that type to `signal`, whose prototype takes a `void (*)(int)`; they are
declared with the `int` now. Its `qsort` of `~user` names has a cast of
its comparison function, as its other `qsort`s have, and so does `ps`'s
of the kernel's wait channels.

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
  break now. `csh` frees only what lies between `end` and its stack, to
  pass over strings in its data and on its stack, and so here would free
  nothing; on the 286 it frees what lies above where the break started
  (`_minbrk`). `c2` takes the heap to start at `end` too, and is not
  built yet. `sendmail` freezes what lies from `end` to the break, which
  here is the BSS and the heap with the stack between them, and writes
  the two each by itself ([mail](#mail)).
- **That `printf` can be written in PDP-11 assembly.** `csh` prints
  through its own `putchar`, with the formats of `doprnt11.s`; on the 286
  that is C, in `bin/csh/printf.c`.
- **That the network calls exist.** In a kernel without networking they
  end a process with SIGSYS. `syslog` has a fallback for that, writing
  to `/usr/adm/messages` and then the console, which was for the PDP-11
  only and is for the 286 too. With the network compiled in, as now,
  `syslog` gets its socket and sends to `syslogd` at `/dev/log`, and
  writes to the console itself only if `syslogd` is not running.
- **That the year begins with 19.** `chpass` showed a date as `19`
  and the years since 1900, so 2026 as 19126, which it could not read
  back; it shows the whole year now.
- **That the password database exists.** With only the flat
  `master.passwd`, `getpwent` took the password from the start of the
  file, so root's was "root". It was meant for the ndbm files, where the
  password's place holds an offset; it is used only for those now.
- **That only the PDP-11 has a 16-bit `int`.** The tree's `#ifdef pdp11`
  sometimes means that, and its other branch was written for a 32-bit
  `int`. `rand` returned its generator's low 16 bits, the poorest and
  half the time negative, where `RAND_MAX` is 0x7fff. It and `valloc`,
  for which a page boundary means no more in a segment than on the
  PDP-11, take the PDP-11's branch now, as `syslog` does. So do `csh`'s
  allocator, its limit on the words a pattern makes, and its printing of
  the most negative `int`; its `time` leaves out the PDP-11's count of
  overlay changes. `tcsh` has the same, and also takes the PDP-11's
  flags for its error numbers, its smaller buffers for the terminal and
  the line editor, a `long` for the milliseconds `time` prints, and the
  freeing of every word of a pattern's expansion, not only the list.
  Its `$HOSTTYPE` is `i286`. Its `SIGN_EXTEND_CHAR` was written for a
  32-bit `int`: with 16 bits, `0xffffff00` is a `long`, so the result was
  never negative and `$<` did not stop at a byte with its high bit set.
  `ifconfig` gave its `NEXTARG` the 32-bit branch's value, which a
  16-bit `int` cuts short, so that `netmask`, `metric`, `broadcast` and
  `ipdst` never took the argument after them. `route` printed an
  address as four `long`s to a `%u` that takes an `int`, which the
  PDP-11's branch casts. `telnetd` passed a 0 where
  `lmode` takes a `long`, a bug on any 16-bit machine, and `ping` worked
  out its average time in microseconds in an `int`, which a 16-bit one
  holds only to 32 ms. The programs have many more such conditionals, to
  be read as each is built.
- **That `/etc/autoconfig` exists.** On the PDP-11 `init` runs it to find
  the devices; this kernel finds its own, and `init` no longer runs it.

The kernel changed for two of these: without quotas compiled in, the
quota calls fail with EINVAL, which `login` looks for, rather than end
the caller with SIGSYS; and `execve` gives the arguments and environment
room above the program's stack, so that the stack size a program is
linked with is its own (8K by default).

## Before a program is built

`tools/preflight.py` looks a program over, a C file or a directory of
them, and says what the two sections above lead one to expect of it. It
is run over a batch of programs before any of them is built, to see
which need nothing, which need what, and which hide a bug that no build
will show.

- **What the compiler rejects**, counted by kind, with what each kind
  has usually wanted.
- **Calls that disagree across files.** The compiler holds a call to an
  old-style definition's arguments only within one file. Between files,
  and between a program and the library, the PDP-11 had `lint`, which
  this tree does not carry. The script has the compiler do it. It
  appends to each file a call of every function the file defines, with a
  structure for every argument, and reads the types out of the
  complaints, typedefs resolved. Then it compiles each file again with
  an empty definition appended for every function the file calls and
  another file defines, taking arguments of those sizes, and the
  compiler checks the file's own calls against them. The library's
  functions are worked out the same way from its sources, once, and the
  system calls' from `sys/syscalls.tab`. A function with a prototype in
  scope is left alone: the compiler converts its arguments. What comes
  out is sorted by what it costs: a *wrong size* (an `int` where a
  `long` is taken, or a function returning a `long` that the caller
  never declared and whose result it uses) is a bug on the PDP-11 too; a
  different *count* is to be read; an `int` for a pointer is the same
  size and only counted.
- **Traps**: lines that match something that has gone wrong before. A
  conditional on the machine, the kernel's memory, an executable's
  header, `varargs.h`, the break and `end`, a null pointer to `ioctl`, a
  year after "19", an allocation of a bare count. A list to read.
- **A trial build**: if every file compiles, the program is linked, with
  whichever libraries it turns out to need, small model and then
  medium. That gives the names the libraries lack, names defined twice,
  the size, and the functions with 1K or more of local variables for
  the 8K stack.

`tests/preflight` runs it on two files that have one of each thing it
looks for across files, and calls that are right.

Run first over the 182 programs and directories of `bin`, `usr.bin`,
`ucb`, `sbin`, `usr.sbin` and `libexec` that were not built then, which
takes ten seconds, it found that 118 compile as they are and 103 of those
link;
33 more have five errors or fewer. It found 60 calls of the wrong size
in 15 programs: `lseek` given an `unsigned` offset in `trpt` and `trsp`
and its result taken as an `int` in `lpr` and `ftpd`, `inet_addr`
undeclared in `tftp`, a `long` for `getpwuid` in `id`, an `int` for
`ntohl` in `routed`. It found `socket`, `bind` and `connect` called with
an argument short, in `whois` and others written before the calls took
a length; and `shutdown` defining a `warn` of its own beside the
library's. It does not look into subdirectories, so `sendmail`, `named`
and the like are given by their own directories.

Run over the 94 programs that were built then, each with the sources and
flags `usr/src/i286/Makefile` gives it, it agreed with the build, in that all
compile and link, and found what the build could not. `dd` called
`gettimeofday` with one argument, so the kernel was given whatever lay
above it on the stack as the place to put the time zone. `netstat`'s
`plural` took an `int` and was given a `long` 135 times, which reads the
low half here and the high half on the PDP-11, where every count was
plural. The shell and `w` compared `lseek`'s result as an `int` without
declaring it, `test` gave `isatty` a `long`, and `bc` left with `exit()`
and so with whatever status the stack held. Those are fixed. What it
still lists there is to be read and is not wrong: functions that take
their arguments by address, as `tcsh`'s `mkprintf` does, or are called
with trailing arguments left off, as in `ex` and `csh`; and `sethostid`,
which `unistd.h` declares as an `int` and the library defines as a
`long`, returning 0 or -1 either way.

## Programs that read the kernel

`ps`, `w` (which is also `uptime`), `pstat`, `vmstat`, `iostat`,
`netstat`, `arp` and `route -f` look at the kernel's tables as they do
on the PDP-11. Each finds a table
by its name in `/unix` with `nlist` and reads it through `/dev/kmem`, and
`ps` and `w` read a process's user structure and arguments through
`/dev/mem` and the swap device. They run with group `kmem`, which those
devices and the swap partitions belong to, as upstream installs them.
`tools/mkfs.py` gives a file a group when its mode in `root.proto` is
followed by one, as in `2755:2`.

`nlist` is upstream's `gen/nlist.c`, reading this system's format
([executable format](executable-format.md)). `sys/exec.h` has a `struct
xexec`, the header with the table of segments after it, in the place of
the PDP-11's table of overlays, and `N_SYMOFF` and `N_STROFF` in
`a.out.h` add up the segments' sizes from it (`lib/libc/i286/gen/nsym.c`).
`ps` has its own copy of `nlist`, which the same headers serve. A
symbol's value is its segment, in `n_ovly`, and its offset in that
segment.

`/dev/kmem` takes an offset as a far pointer is made: the kernel's
segment in the high 16 bits, counting from 1 as the executable does,
and the offset in it in the low. A symbol's `n_ovly` and `n_value` are
then where to read it. Segment 0 is the kernel's data segment, so an
offset below 64K is an address in that segment, as on the PDP-11, and a
program that knows only `n_value` reads what is there. Most of the
kernel's tables are in that segment. The tables of every process's
segments (`pseg`) are in a segment of their own, and the network's
variables and mbufs are in another. Code can be read through
`/dev/kmem` and not written.

The PDP-11's `ps` works out a process's user structure and stack from
`p_addr`, `p_daddr` and `p_saddr`. On the 286 it reads the kernel's
tables instead ([processes](processes.md)):

- The user structure is in one of the kernel's slots while the process
  is the one in it, which `slotproc` says, and at `p_addr` otherwise.
  It stays in memory when the process is swapped out.
- The arguments are at the top of the stack, in the segment `u_dseg`
  names, between the data and the break.
- Where that segment is, in memory or on the swap device, is in `pseg`.

That lookup is a function of its own in `ps`, `getu286`, and `w` has a
copy of it. So `w` needs `nlist` and `/dev/kmem` too, which upstream's
does not.

The top of the stack is laid out as on the PDP-11, a word of zero with
the strings below it, so `ps` finds the arguments as it does there. It
shows what a program has written over its own arguments: an interactive
`csh` writes `-` over its name's first letter, as upstream's does, and
`init` blanks its second argument. `ps` takes the names of wait
channels from the kernel's data segment only, since a wait channel is
an address in it.

The PDP-11's network is in a space of its own, which `netstat`, `arp`
and `route -f` reach at an address the kernel gives them (`_netdata`).
On the 286 the network's pointers are offsets in its own data segment.
They reach them with that segment's number in the high 16 bits, the
number the symbols give for the network's variables, and otherwise take
the PDP-11's way through their code.

`pstat` lists a process's slot and the size of all its own segments,
where the PDP-11's lists the addresses and sizes of its data and stack.
Its `-u` takes a process's `p_addr`, as `ps -l` shows it, and reads the
user structure from the process's slot if the process is the one in it.
It prints the 286's fields in place of the PDP-11's segmentation
registers, overlays and FP11, with the addresses in the process in
hexadecimal, as selector and offset. It lists the console's terminals
and the serial ports, whose counts the kernel keeps for it as the
PDP-11's does for its own lines (`ncons`, `ncom`). It gives the maps of
memory and of the swap area in kilobytes by shifting right, where the
PDP-11's shifts left by a negative count, which its `ash` takes as a
shift right and the 286 does not.

A program reads the kernel's structures with its own idea of their
layout, so they are laid out the same in every model. The addresses in
the process that the user structure holds are declared far, as the
kernel has them: the signal handlers, the trampoline and the signal
stack's base. So is a terminal's output routine, `t_oproc`, which is far
in the medium-model kernel. `signal.h` gives a `struct sigaltstack` a
far base only in the kernel, and says so with `__SIGALTSTACK_FAR`, which
`user.h` goes by rather than by `KERNEL`: `pstat` defines `KERNEL` to
include `user.h`, and has had `signal.h` without it. The kernel's own
pointers are near, as in a small- or medium-model program, so a
large-model program could not read these structures as they are.

`/etc/rc` runs `dev_mkdb`, which makes `/var/run/dev.db`. That is the
database in which `devname(3)` finds a device's name, for the terminals
`ps` shows and for the swap device. Then `rc` runs `ps -U`, which keeps
the symbols `ps` needs in `/var/run/psdatabase`, so that it need not
read `/unix` each time.

`vmstat` and `iostat` read the counts the kernel keeps with `UCB_METER`
([processes](processes.md#counting)): the time spent in user, system and
idle, the system calls, traps, interrupts and context switches, forks,
swaps, the name cache, and each disk's transfers.

`top`, `tracebuf` and `sysctl` came with the tools for measuring the
system ([performance](performance.md)). `tracebuf`, which prints the
kernel's counts of what its buffers saved the disk, is as upstream has
it. `top` reads the table of processes through `/dev/kmem` and each
one's user structure for its times and its name, and finds that as `ps`
and `w` do, with a copy of their lookup (`getu286`); it takes the size
of the stack from the user structure, a process here having no
`p_ssize`, and counts free memory in this machine's clicks, where it
had the PDP-11's 64 bytes written out. Three things in it were wrong on
any machine and are put right:

- It kept the addresses of the kernel's tables in an `int` and widened
  that to seek to them, so a table in the upper half of the kernel's
  data, as the table of processes is here, was sought at a negative
  offset.
- It adjusted `argc` and `argv` inside the loop that reads its options,
  so only the first option given was taken.
- It passed the table of processes to its own `readat` as the wrong
  type of pointer, and its comparison to `qsort` likewise.

`sysctl` knows the PDP-11's names under `machdep` for its tape and disk
controllers, which are now there only where the machine's `cpu.h`
defines them. Given several names it printed the first one once for
each: `parse(*argv, 1)` in a loop that never moved `argv`.

## The log, and what the clock runs

`syslogd` is upstream's, unchanged. Messages reach it from three places.
Programs send theirs to the Unix-domain datagram socket it binds as
`/dev/log`, other machines to UDP port 514, and it reads the kernel's
from `/dev/klog`: all that the kernel has printed since that was last
read, so the messages of the boot are the first lines of the log.
`/etc/syslog.conf` is upstream's too, and says where each kind goes: to
the files in `/usr/adm` (`messages`, `cron`, `daemonlog`, `debuglog`,
`lpd-errs`) and `/usr/spool/mqueue/syslog`, to the console, and to the
terminals of users. `syslogd` writes only to files that exist, so the
root file system has them, empty. `logger` sends a message from the
shell.

A line the kernel prints with `printf` does not wake `syslogd`, here as
upstream: only its `log` and `tprintf` do. The line waits in the buffer
until something else wakes `syslogd`, a message from a program or its
own timer, which runs every two minutes. `syslog.conf` sends the
kernel's errors to the terminals of root and of operator as well as to
the files, so such a line then appears on each terminal root is logged
in on.

`syslogd` is linked with a stack of 12K. It reads the kernel's 4K of
messages into an array in `main`, and three of the functions below
that each format a line of 1K in an array of their own, as does the
handler of its timer on top of them. That is more than the 8K a
program's stack is here unless the Makefile gives it another size, and
on the 286 the stack does not grow as the PDP-11's does.

`/dev/klog` could not be opened until `syslogd` tried. `logopen` refused
a log whose `msg_click` was zero: on the PDP-11 that is where the log's
buffer is, and on the 286 the buffers are far arrays in the kernel and
the field is not used. The check is gone. The log device has two more
logs: `/dev/errlog`, which nothing reads yet, and `/dev/acctlog`.

`dmesg` prints the kernel's messages from the buffer itself. The
PDP-11's reads the buffer from `/dev/mem`, at the click that `msgbuf`
gives. On the 286 `msgbuf` holds a far pointer to it, and `dmesg` reads
it from `/dev/kmem` ([programs that read the
kernel](#programs-that-read-the-kernel)), in the segment whose selector
that is: the selectors of the kernel's segments are in the order of the
segments' numbers. A program's `struct msgbuf` has that pointer as an
offset and a selector, the offset in the member that `dmesg` then
points at its own copy.

`cron` and `crontab` are upstream's, Paul Vixie's, unchanged. `cron`
makes `/var/cron/tabs`, where `crontab` keeps each user's table, and
it runs `/etc/crontab` as well, which is this port's own. That has
upstream's line that runs `atrun` every quarter of an hour. Upstream's
also runs `/usr/adm/daily`, `weekly` and `monthly` and mails root what
they print, but the tree has no such scripts. This one has `sendmail
-q` try each hour to deliver what is in mail's queue, which upstream
leaves to `sendmail`'s daemon ([mail](#mail)). `cron` mails what a job
prints to the job's owner, through `sendmail`.

`cron` reads the tables again when `/etc/crontab` or the directory of
tables has a later time of modification than it last saw. So it does
not notice a new table while either is dated in the future. The kernel
takes the clock chip's time to be universal time. The test harness's
86Box kept the host's local time in the chip, which on a host west of
Greenwich made every file of a root file system built there some hours
newer than the clock; it keeps universal time now (`time_sync` in
`tests/harness/86box.cfg`).

`at`, `atq` and `atrm` run as `daemon`, which owns the queue,
`/usr/spool/at`, and `cron` runs `atrun` as root, which runs each job
whose time has come as its owner. `atrun` could not run any. It reads
the four lines at the head of a job with four calls of `fscanf`, and
each format ends with `%*[^\n]\n`, to pass over the rest of the line.
Where the line has no more, that conversion matches nothing, and
`fscanf` returns there with the newline unread. The next call's format
began with `#`, which a newline does not match. The tree's older
`_doscan` went on after a conversion that matched nothing; `vfscanf`,
which upstream's library uses now as well, stops, as the C standard
says. The later formats begin with a blank now, which passes over the
newline. (`atq` and `atrm` open the file again for each line they
want.) `atrun` mails a job's owner, through `/bin/mail`, if the job
wrote to its standard error or was queued with `-m`.

`/etc/rc` turns on the accounting of processes as upstream's does, with
`accton /usr/adm/acct`. `accton` writes `/etc/acctd.cf` and starts
`acctd`. The kernel writes a record to the log device as each process
ends, and `acctd` reads them from `/dev/acctlog` and appends them to
the file, stopping while the file system has less than 2% free.
`lastcomm` lists the records and `sa` adds them up. They are some way
behind: the kernel wakes `acctd` at every tenth record, and `acctd`
writes the file a kilobyte at a time, 32 records. `acctd` does not
make the file, so it is in the root file system, empty.

`find` is built because `/etc/rc` clears `/tmp` with it.

## Grammars: yacc and lex

`expr`, `awk`, `egrep`, `bc` and `make` each have a grammar for `yacc`,
and `awk` a scanner for `lex`, which is itself a `yacc` grammar. The C
is made from them by the tree's own `yacc` and `lex`, not by whatever
the machine the system is built on has under those names, so that the
tables and the parser around them, `yaccpar`, are upstream's. The
Makefile builds the two with that machine's C compiler, in `obj/host`,
and runs them there; the C they make is in `obj/gen`.

Both are C of before the standard, and are compiled as such
(`-std=gnu89`, with the compiler's complaints about missing return
values and about integers passed for pointers made warnings again).
They were written for machines where a pointer is the size of an `int`,
and the machine they are built on now is not one. What that took:

- `yacc` keeps the grammar's rules in an array of `int` and then, in
  the same array, the items of its states, which are two pointers each.
  That works wherever a pointer may be stored at an `int`'s alignment,
  but the items take twice the room they did. `files`, which sets the
  size of `yacc`'s tables, let `HUGE` be given to the compiler in place
  of its `MEDIUM`, and the path of `yaccpar` likewise; the build's
  `yacc` has both. The tables written do not depend on the size, which
  `tests/system` shows by the way: the system's own `yacc` is `MEDIUM`.
- `lex` keeps either a number or a pointer to characters in the left
  and right of each node of its parse tree, and in the values of its
  grammar's symbols, all of them `int`. They are an `INTPTR` now, which
  is `int` unless the compiler is told otherwise; the build's `lex` is
  told `long`. The functions that take one are declared with their
  parameters when it is not `int`, so that a number passed is widened.
  It also asked `malloc` for the tree's memory once, to see whether
  there was enough, by way of an `int`; that goes by way of a pointer.
- `lex` gave `errorf` the value `stdout` where it is defined, which
  needs `stdout` to be a constant; `main` gives it now. The paths of
  its drivers, `ncform` and `nrform`, may be given to the compiler.
  `lex` has an `index` of its own, with the arguments the other way
  round from the library's, and calls it undeclared; the compiler is
  told not to take it for the library's (`-fno-builtin-index`).
- Each has an `error` that takes a format and sometimes an argument.
  Both are variadic now, with `vfprintf`, which this system's compiler
  wanted too ([strictness](#what-this-compiler-needed)).

`lex` chooses its character set with `#ifdef unix`, which the PDP-11's
compiler defines and this one does not; the Makefile gives it `-Dunix`.

`awk`'s Makefile makes three more files, and so does this one: `awk.h`,
the tokens' numbers, which `yacc -d` writes; `token.c` with its table of
the tokens' names made from that, by `sed` where upstream has an `ex`
script, and at present the same as the file in the tree; and
`proctab.c`, the function that runs each token, which is written by a
program of `awk`'s own, `proc`, built for the machine the build runs on.
`proc` used `tokname`, which returns a pointer, undeclared.

The system has `yacc` and `lex` themselves too, in `/usr/bin`, with
`yaccpar` in `/usr/share/misc` and `lex`'s drivers in
`/usr/libdata/lex`. `tests/system` has each make C of `awk`'s grammar
and scanner, which must be byte for byte what the build's made: so the
programs on the 286 and the ones compiled for the build's machine, from
the same source, agree. There is no C compiler on the system to give
their output to, so the libraries that go with them, `liby` and `libln`,
are not built.

`awk` uses floating point throughout, and the mathematical library for
`sqrt`, `exp` and `log`. `libm` is `usr.lib/libom`, the older of
upstream's two and the one in C, which is what upstream installs under
that name on the PDP-11. As in the Seventh Edition, this `awk` reads its
input even when the program is only a `BEGIN`.

`bc` compiles its input for `dc`, which it runs, and `bc -l` reads its
library of functions from `/usr/share/misc/lib.b`. `dc` could not copy a
number that was zero: such a number has no digits, `copy` asked
`malloc` for no bytes, and this patch level's `malloc` returns nothing
for that, which `dc` took for memory running out ("out of space:
copy"). The routine that allocates a new number already asks for one
byte more for that reason; the one that copies does now. Any sum that
passed through zero failed, among them `bc -l`'s arctangent.

What else the compiler wanted of these programs, all of the kinds listed
[above](#what-this-compiler-needed): `expr`'s `match` and `yyerror`
declare their parameters; `awk`'s `error` is variadic and declared in
`awk.def`; `egrep` passes `execute` a null pointer, not a 0; `bc`'s
`bundle`, which walked up the stack from its first argument, is
variadic, and a string passed to `output` is cast; `dc` casts between
its two kinds of block where it passes one for the other; `make`'s
signal handler is `void`, and an array of characters is initialized with
a string, not a character. `make`'s `mkqlist` returned no value for an
empty list, where it had just made its buffer the empty string; it
returns the buffer.

## Programs that work on text and files

The next 77 were chosen with that script: those of `usr.bin` and `ucb`,
and some of `bin`, `usr.sbin`, `sbin` and `libexec`, that need nothing
but themselves, together with those that wanted a few lines changed. 58
are compiled as upstream has them. All are small-model programs.

| Where | Programs |
|---|---|
| `/bin` | `diff`, `sed`, `strip`, `su`, `tar` |
| `/usr/bin` | `basename`, `cal`, `cb`, `checkeq`, `col`, `comm`, `crypt`, `deroff`, `fgrep`, `file`, `id`, `install`, `join`, `look`, `m4`, `mesg`, `patch`, `printf`, `ptx`, `renice`, `rev`, `shar`, `sort`, `spline`, `split`, `sum`, `tabs`, `touch`, `tr`, `traverse`, `tsort`, `tty`, `uname`, `uniq`, `units`, `whereis`, `xargs` |
| `/usr/ucb` | `apply`, `checknr`, `clear`, `colcrt`, `colrm`, `compress`, `ctags`, `expand`, `fold`, `head`, `last`, `leave`, `lock`, `mkstr`, `more`, `printenv`, `script`, `soelim`, `strings`, `tail`, `tset`, `ul`, `unexpand`, `unifdef`, `users`, `wc`, `what`, `xstr`, `yes` |
| `/usr/libexec` | `diff3`, `diffh`, `makekey` |
| `/usr/sbin`, `/sbin` | `ac`, `chroot`; `shutdown` |

With them go what upstream installs beside them: the scripts `true`,
`false`, `whoami`, `groups`, `nohup`, `which` and `diff3`, which runs
the program of that name in `/usr/libexec`; the other names `e` for
`ed`, `u` for `users`, `uncompress` and `zcat` for `compress`, `page`
for `more` and `reset` for `tset`; and `units`, `eign` and `more.help`
in `/usr/share/misc`. `install` is `xinstall` in the tree and in
`obj/bin`.

What the compiler needed:

- `sed` keeps a command in a union of two structures that differ in one
  member, and names the members without saying which structure, on 121
  lines. The union is one structure with no name now, and that member a
  union with none, which this compiler takes; no line that uses them is
  changed.
- `more`'s `execute` hands `execv` the address of its third argument,
  for the ones after it. It is variadic, with a prototype before its
  calls. `join`'s `error` is variadic too.
- `ul` has a `puts` of its own and `apply` a `system`. Their definitions
  take what the headers' prototypes say.
- `shutdown` has a `warn`, and so has the library, in the file with
  `err`, which `shutdown` calls. Its own is static.
- Casts: of the comparison `mkstr` gives `qsort`; of a structure's
  address in `units`; and of a bare 0 for a pointer in `id`, `more`,
  `tset` and `diff`.

What the machine needed, each in an `#ifdef i286`:

- `file` knows the system's executables, as `medium model executable
  not stripped` or `small model set-uid executable not stripped`. It
  still knows the PDP-11's, three of whose magic numbers `a.out.h` no
  longer has.
- `strings` looks for strings in an executable's data segments, where
  the PDP-11's looked in its data.
- `sort` takes its 32K above the break. It measured from `end`, where
  the stack is here, and so had 8K less than it meant.
- `patch` keeps the PDP-11's limit of 48K on a file it holds in memory.
  The size of a larger one does not fit the 16 bits `malloc` takes.
- `diff` counts a directory's entries before it allocates for them, as
  on the PDP-11, where the VAX grows the list an entry at a time.
- `strip` ends a file with its last segment and sets the sizes of the
  two tables after that to zero: the symbols, and their names. With it
  `install -s` works.

Bugs, which the PDP-11 has too:

- `touch -t` set a time a day late for any date before March of 2000.
  It asked `isleap` about the years since 1900, so 2000 was an ordinary
  year, and counted the day of the month as a day gone by; from March of
  2000 the two cancel.
- `chroot` changed to the new root and then gave `chroot` the same
  name, which a relative name no longer names from there. It gives ".".
- `id`, given a number for a user, passed `getpwuid` a `long`.
- `ctags` left out an argument of `pfnote` for a Fortran function, and
  `diff -c` one of `fetch`.

What upstream does that the test's author did not expect, and that is
left as it is:

- The Bourne shell does not redirect a built-in command, so `: >file`
  makes nothing. It writes an empty line when a command in a pipeline is
  killed for writing to a closed pipe, as `yes | sed 1q` is. At a
  terminal it does not leave on `exit`, only at the end of the file.
- `awk` reckons in `float`: 7919 times 6000 is not exact.
- `tr -s` wants both strings, and `tr` drops every NUL it reads.
  `printf` stops at a `\0` in its format.
- `nohup` writes to `nohup.out` when its output is a file too: its
  `test -t`, with no number, is true of one.
- `script` sets the terminal's modes as it starts, which discards what
  was typed before its shell's first prompt.
- `spline` writes its points from the last to the first. `diff -h`, of
  files of five and six lines that differ in the third, says that the
  rest of each was changed.
- `uname -s` is `BSD` and `uname -r` is `2.11`. Root's group is
  `daemon`.

Left out of this batch: `tc` and `tk`, which are for a Tektronix
4014; `from`, `biff` and `msgs`, which came with
[mail](#mail), and `calendar`, which runs `/lib/cpp`; `tcopy`, `mt` and `tp`, for tapes; the scripts
`lorder`, `mkdep` and `lint`, which run `nm`, the compiler and the
passes of `lint`; `spell`, which needs its word lists made; `indent`,
where the script found 16 calls to read, each with another number of
arguments than the function is defined with;
the programs that read the kernel, `fstat`, `top` and `gcore`; those
for object files, `nm`, `ar`, `ranlib`, `size` and `ld`; and
the network's, which came next. `look` has no `/usr/dict/words` to
search by default, the tree having none.

`tests/programs` has the system run a check or more of each: 75 short
scripts, each with what it must write, to the byte. They are in
`tests/programs/checks`, with the files they read.

## The network's programs

After `telnet` came the rest of what the network is used with: 31
programs and `ntest`, `ntp`'s check of its own arithmetic. 18 are
compiled as upstream has them. All are small-model programs; `ftp` is
the largest, with 54K of code.

| Where | Programs |
|---|---|
| `/bin` | `rcp` |
| `/usr/ucb` | `finger`, `ftp`, `rlogin`, `rsh`, `ruptime`, `rwho`, `safe_finger`, `talk`, `tcpdchk`, `tcpdmatch`, `tftp`, `try-from`, `whois` |
| `/usr/libexec` | `fingerd`, `ftpd`, `ntalkd`, `rexecd`, `rlogind`, `rshd`, `tcpd`, `tftpd` |
| `/usr/sbin`, `/sbin` | `nc`, `ntp`, `ntpd`, `ntpdc`, `rwhod`, `timed`, `timedc`, `traceroute`; `routed` |

`ntalkd` is `talkd` in the tree and in `obj/bin`. `rcp`, `rlogin`, `rsh`
and `traceroute` run as root, to bind a reserved port or open a raw
socket, `timedc` too, and `talk` with the group `tty`, as upstream
installs them.

Two libraries came with them. `talk` draws its two windows with
`libcurses`, which is upstream's as it stands but for one declaration:
its `printw` hands the address of its last argument to the library's
`vfprintf`, which the calling convention and this `stdarg.h` let it do
([the ABI](abi.md)). `tcpd` and its four companions share `libwrap`,
which upstream's Makefile makes for them too: `tcpdmatch` and `tcpdchk`
define some of its functions themselves, and take only the rest from it.

What the compiler needed:

- A function of a program's own that formats a message and is called
  with whatever the message takes: `error` in `rcp`, `rshd` and
  `rexecd`, `reply`, `lreply` and `setproctitle` in `ftpd`, and `holler`
  and `bail` in `nc`. Each took a fixed number of `int`s or pointers and
  passed them all on; `ftpd` gave its `reply` a `long` among them. They
  are variadic now.
- `ftp` has a `getpass` of its own, whose argument is `const` now, as
  the header's is. `routed`'s `ifinit` returned a value in one place
  and none elsewhere, and no caller looked. Casts, of a null pointer in
  `ntpd` and of an address in `routed`.
- `libcurses` declared `tgoto` at the end of a list of static
  variables, and so as static.

What the machine needed:

- `timed` has its Internet checksum in a directory for the machine. The
  PDP-11's is C whose assembly an `ed` script finishes; the 286's is
  `timed/i286/cksum.c`, in C alone.
- The PDP-11's branch where it is there for the 16-bit `int`: the value
  `timed` and `timedc` use for a host that does not answer, the mask on
  `routed`'s hash, and the size of the buffer `ntpdc` asks for.
  `rwhod` writes what it receives to a file for `rwho` and `ruptime`,
  with the numbers turned from the network's byte order, on the 286 as
  on the VAX and the PDP-11.
- `rshd` and `rexecd` hold a command line on the stack, of 5K, `ftp`
  and `ftpd` what a pattern expands to, and `tcpd` and its companions a
  rule's options and a banner. They are linked with a stack of 16K.
- `ftpd` did not declare `lseek`, and so took half of what it returns,
  where a restarted transfer is to begin, for whether it failed. On the
  PDP-11 that is the upper half, which serves; here it is the lower,
  and a transfer restarted at 32768 was refused. It is declared now.
- `ftpd` says what kind of system it is only where `unix` is defined,
  as the PDP-11's compiler defines it. Here the Makefile does, for
  `ftpd`, and `ftp` then sends files as they are, without being told.

Bugs, which the PDP-11 has too:

- `ftp` opens a data connection on the descriptor it used for the last,
  after closing that one in case it is still open. In passive mode,
  which is how it starts, the last was closed when its transfer ended,
  and `put` has by then opened the file to send, which gets the same
  number. So a `put` after a `get` closed its own file, and then read
  from the new connection, which had nothing to say: both ends waited.
  It forgets the descriptor when the transfer ends now.
- `ftpd` answered a failed `MDTM` with `%s` and the error, having given
  the routine a format where it takes a name. Its answer to `SYST`
  printed the system's version, a `long`, as an `int`.
- `rlogind` left out the descriptor in a call of its `fatalperror`,
  for when it cannot tell who has connected. It and `rshd` gave
  `setsockopt` the address of a length where the length goes.
- `tftp` did not declare `inet_addr`, so an address given in numbers
  lost its upper half.
- `ntpd` did not declare `atof`, for the size of a jump in time it will
  make at once; gave `htonl` an `int` twice; and shifted an `int` 1 by
  18 where it works out how fast to correct the clock.
- `routed` gave `ntohl` an `int`, a metric already in the machine's
  order, on its way to a trace record, and printed that record's metric,
  a `short`, as a `long`.

What upstream does that the test's author did not expect, and that is
left as it is:

- `routed -s` on a machine with one interface never sends its routes,
  since all of them are through the interface it would send them on; so
  it does not hear itself, takes the interface to be dead after three
  minutes, and deletes the network's route from the kernel. `routed -q`
  listens only, and leaves the interface alone. Upstream's `rc.conf`
  says not to run it at all.
- `ntpd` passes over a peer that is the machine itself, as upstream's
  `ntp.conf` names, and `ntpdc` waits for ever on an `ntpd` that has no
  peers: its alarm does not interrupt the read.
- `rlogin`'s `~.` is taken by the first `rlogin` when one is inside
  another, and ends both.
- `fingerd` runs with `-s`, and then answers a request that names no
  user with `must provide username`.
- `nc -r`, for ports in a random order, asks for 64K, which comes to
  none. `safe_finger` stops reading after 100000 bytes, counted in an
  `int`; its minute's limit is what stops it.
- `tcpd` knows a client's user only if the client runs `identd`, which
  nothing here does yet.
- Anonymous `ftp` is confined to the home directory of the user `ftp`,
  and what is not under it is not there: `/bin/ls` for `dir`, and
  `/etc/syserrlst`, without which an error is `Unknown error: 2`.
- The Bourne shell has no functions, and a here document for a command
  put in the background is gone before the command reads it.

`inetd.conf` is as upstream's now: `ftpd`, `telnetd`, `rshd` and
`rlogind` are started through `tcpd`, `fingerd` as `nobody`, and
`rexecd`, `tftpd` and `ntalkd` are there to be turned on. `tcpd` goes
by `/etc/hosts.allow` and `/etc/hosts.deny`. Upstream's `hosts.allow`
lets in its example domain and the hosts whose names have no dot, and
its `hosts.deny` refuses the rest, which on a network without names is
everyone but the machine itself; the port's `hosts.allow` lets in
everyone, as before there was a `tcpd`, and says how to narrow that.
`hosts.equiv` names `localhost`. `/etc/rc` starts `rwhod`, `routed`,
`timed` and `ntpd` if `/etc/rc.conf` says to, as upstream's does; none
is on as shipped.

Left out: `identd`, which reads the kernel's tables to find whose a
connection is, with the others that read the kernel; `comsat`, `biff`
and `popper`, which came with [mail](#mail); `named` and its tools,
though the library's resolver is there to ask another machine's;
`mkhosts`, for a table of hosts the library here does not read;
`slattach`, since the kernel has no serial line interface; `trpt` and
`trsp`, since it does not keep the traces they print; `rmt`, for
tapes; and `rdist`, `tn3270`, `tip` and `uucp`.

`tests/network` has the system run these, on the loopback interface
and, for the daemons that speak to the whole network, on the Ethernet
card: 21 checks in `tests/network/checks`, and `rlogin` and `talk` at
terminals.

## Mail

Then mail, which `cron`, `at` and the editor were waiting for:
`sendmail`, `/bin/mail` and `Mail`, and what waited for them, `comsat`
and `biff`, `from`, `msgs`, `vacation` and `popper`. Of the 10, 4 are
compiled as upstream has them. `sendmail` is medium-model, and the rest
small.

| Where | Programs |
|---|---|
| `/bin` | `mail` |
| `/usr/ucb` | `Mail`, which is also `mail`; `fmt`, `biff`, `from`, `msgs`, `vacation` |
| `/usr/sbin` | `sendmail`, which is also `newaliases` and `mailq` in `/usr/bin` |
| `/usr/libexec` | `comsat`, `popper` |

A letter goes to `sendmail`, from whoever sends one: `/bin/mail` given
names, `Mail`, `cron`, `at`'s `atrun`, `expreserve` through `/bin/mail`,
or another machine by SMTP. `sendmail` works out from
`/etc/sendmail.cf` whom it is for and hands it on: to `/bin/mail -d` for
a user here, which adds it to the user's mailbox in `/usr/spool/mail`
and tells `comsat`, which tells the user at each terminal `biff y` was
given on; to a program, for an alias such as `msgs`'s; or by SMTP to
another machine. `sendmail.cf` is upstream's: mail for a user here is
delivered here, and the rest goes to a relay it calls
`RELAY.EXAMPLE.COM`, to be set for the machine's network. `/etc/aliases`
is upstream's too.

sendmail looks an alias up in a database, `aliases.dir` and
`aliases.pag`, which `newaliases` makes. Until the database is there
and has its last key, every `sendmail` waits for someone to finish
making it, as long as `sendmail.cf`'s option `a` says: ten minutes. So
the root file system has it, made by `tools/mkaliases.py` with
`tools/mkpasswd.py`'s copy of ndbm, as the password database is; the
system's own `newaliases` makes the same bytes, which `tests/network`
checks. A user may forward mail with a `.forward`, which is how
`vacation` is set to answer it.

`sendmail` had three overlays on the PDP-11. Here its 70K of code is in
two segments, and its data segment has 21K of data and 16K of stack,
leaving about 27K for its heap. It takes the PDP-11's sizes for its
tables (`conf.h`): lines of 256 bytes, ten jobs to a run of the queue.
On the PDP-11 it is linked with `libstubs`, which has `ctimed`, another
process, do `localtime` and the like for it, to save room in its code;
here it is linked without, and has the library's own. The PDP-11's
build also takes its messages out into `/usr/share/misc/sendmail.sr`
with `mkstr` and `xstr`, as `ex`'s are, which this one has the room not
to do. What the compiler needed: `syserr`, `usrerr`, `message`,
`nmessage`, `setproctitle` and `smtpmessage`, which took five arguments
or three and passed them on, are variadic; and `daemon.c` has two
`#else`s to one `#ifndef DAEMON`, which the PDP-11's preprocessor took
as a toggle and this one refuses.

`sendmail -bz` freezes its configuration: it writes the memory it read
`sendmail.cf` into, from `edata` to the break, to `/etc/sendmail.fc`,
and a later `sendmail` reads that back in place of the file, which is
quicker. On the 286 the stack lies between `end` and the heap, with the
arguments and environment above it, so the BSS and the heap are written
each by itself, and the heap read back to where it was. That is above
the later `sendmail`'s own arguments only if they and its environment
take no more clicks than those of the one that froze it; otherwise it
reads `sendmail.cf`, as it does when the frozen file is empty, as it is
shipped, as upstream ships it. `tests/network` freezes the
configuration, moves `sendmail.cf` away and has a letter delivered.

`sendmail`'s daemon (`sendmail -bd -q1h`) takes mail from other
machines by SMTP and every hour tries again to deliver what is in its
queue. Upstream's `rc.conf` starts it; this one's leaves it off, since
it holds its 70K of code and at least 37K of data while it waits, a
sixth of what a machine of 1 MB has for programs, and mail sent here is
delivered without it.
`/etc/crontab` has `sendmail -q` try the queue each hour instead.
`sendmail=YES` in `rc.conf` starts it, as upstream's `rc.local` does.

`/bin/mail` passes letters to send to `sendmail` and delivers what
`sendmail` gives it with `-d`. Its `panic` is variadic, and the handler
of its signals `void`. It took the size of a mailbox before a letter
from `ftell` on a stream that `fdopen` had opened to append, which the
tree's stdio leaves where the descriptor is, at the start, until it has
written. So it told `comsat` the new letter began at the start of the
mailbox, and if the write failed it would have cut the mailbox back to
nothing. It asks the descriptor for the end now. That is the tree's
stdio, and a bug on the PDP-11 too.

`Mail` passes its `start_command` the nulls that end its arguments
where one call left them off, and is linked with a stack of 16K, two of
its commands' functions having 3K of local variables each. `fmt` left
out the length of a word in one call of `pack`, a bug on any machine.
`comsat` kept the time `utmp` was last changed in a 16-bit `u_int`,
which never held one, so it read the file again every 15 seconds; it
is a `time_t`. `popper`'s `pop_msg` used `va_start` without being
declared with `...`, which this compiler refuses. `vacation` keeps whom
it has answered in a dbm database, with `libdbm`, which is built for it
and `sendmail` in each model.

`ex` saves what is being edited when its terminal hangs up, and the
`preserve` command does the same: it runs `expreserve`, which copies
the editor's buffer to `/usr/preserve` and mails the user that it has.
`ex` always said "Preserve failed!", and on a hangup left its
temporary files for `expreserve -a` to save, and mail about, again at
the next boot. Its `waitfor` waits for every child, until `wait` has no
more to give, and leaves `rpid` at -1, so that `preserve`'s test that
`rpid` is the child's never held. It leaves `rpid` the child's now, if
the child was among them. A bug on the PDP-11 too.

What upstream does that the test's author did not expect, and that is
left as it is:

- A letter root sends is postmarked "From daemon" in the mailbox.
  `sendmail` runs no mailer as root, but as `daemon`, and gives
  `/bin/mail` the sender's name only when it was given one, by `-f` or
  SMTP; the `From:` line in the letter is root's.
- A client that sends its SMTP commands without waiting for each
  answer, as `nc` from a file does, has what it sent after a letter's
  `MAIL` read again. `sendmail` takes each letter in a child, which
  reads ahead from the connection and ends when the letter is taken;
  the parent then reads from its own buffer what came before the child
  did.
- `comsat`, while it runs, reads who is logged in every 15 seconds, so
  it does not tell someone who logged in since. It rings the bell after
  the address in "New mail for".
- `Mail` notes each letter it sends, the sender, the number of
  recipients and the size, in `/usr/spool/mqueue/syslog`, where
  `sendmail`'s log goes too.
- `popper` leaves an empty `.user.pop` in `/usr/spool/mail`, and marks
  a letter it has listed but not given `Status: U`.
- `vacation` has no message of its own to send: the tree has no
  `/usr/share/misc/vacation.def`, so a user writes `.vacation.msg`.

`popper` is on the system and not started: upstream's `inetd.conf` does
not name it, and its `services` has no `pop3`. This system's
`inetd.conf` has the line, commented out, and says what `services`
needs. `comsat` is started by `inetd` as upstream's has it.

Left out: `calendar`, which runs the C preprocessor, `/lib/cpp`, that
comes with the compiler; `rmail`, for mail by UUCP; and
`sendmail.MX`, which asks a name server for a domain's mail exchanger.

`tests/network` has the system run 12 checks of mail after the others:
`newaliases`; `/bin/mail` and `Mail` sending to a user, `Mail` and
`from` reading; `sendmail`'s SMTP server, from a client that waits for
each answer; an alias; `msgs` and `vacation`, which an alias and a
`.forward` give letters to; `popper`, once `inetd` is told of it; a
frozen configuration; `ex`'s `preserve`, which mails the user; and
`cron`, which mails a job's owner what it printed. Then a user logged
in from the host with `telnet` gives `biff y`, and `comsat` must tell
that terminal of the letter root sends, from where the letter begins.

## nroff and the manual

Then `nroff`, and with it the manual: `man`, and `apropos`, which is
also `whatis`.

| Where | |
|---|---|
| `/usr/bin` | `nroff` |
| `/usr/ucb` | `man`, `apropos`, which is also `whatis` |
| `/usr/share/tmac` | the macros: `-man`, and `-mdoc`, which `tmac.an` reads for a page that starts with `.Dd`; `-ms`, `-me` and the rest of upstream's |
| `/usr/share/me`, `/usr/share/ms` | what `-me` and `-ms` read |
| `/usr/share/term` | the tables of the printers `nroff -T` drives |
| `/usr/man` | the manual, formatted: `cat1` to `cat8`, and `whatis` |

`nroff` is 2.11BSD's own, reworked since 2020 with prototypes, a hash
table of its requests and printers' tables that it reads as text. With
no `-T` it writes for a terminal, by a table compiled in, and it makes
bold by printing a character twice with a backspace between, and
underlines with `_` and a backspace, which `more` shows as the terminal
can. It had separate code and data on the PDP-11; here it is
small-model, with 36K of code and 30K of data. It finds its macros
and printers' tables by `paths.h` only where the preprocessor says
`__BSD2_11__`, which upstream's says and this compiler is told for
`nroff` alone. Three of its handlers of signals are declared to take
the signal's number and defined to take nothing: this compiler let it
pass, the host's does not, and they take it now. A printer's table is a
table in C, which `mktab` writes out as text; the build compiles
`mktab` with each, for the machine it is run on, as upstream's
`nroff_term` Makefile does. The tables and the links among them are
upstream's 25 and 8.

`man` shows only what is formatted already: it looks for
`/usr/man/catN/NAME.0` in the sections in its order, 1, 8 and 6, then
the others, and pages through it with `more -s`, or `$PAGER`, or copies
it out when its output is not a terminal. Upstream formats each page
once, with `manroff`, `nroff -man -h`, as it installs it on the PDP-11
itself, in `usr/src/man` and in each program's Makefile; to format a
page each time it is asked for would be slow on either machine: `ls`'s
page takes 8.5 seconds on the 286, `csh`'s 65, `mkdir`'s, which is
mdoc's, 30, and `printf(3)`'s 3.6 minutes. Here nothing is installed on
a running system: the root file system is made on the machine the
system is built on. So the build formats the pages there, with the
tree's `nroff` built for that machine, as `yacc` and `lex` are, in
`obj/host`. That `nroff` is the system's but that it
looks for its macros in `tmac/` where it is run, which is where it is,
and the copies of the macros there say `tmac/` where the system's say
`/usr/share/tmac/`. `tools/mkman.py` gives it each page, with what the
page's `.so` lines name put in their places: another page beside it,
as `errno.2` is `intro.2`, or a header from the tree's `usr/include`,
as `utmp(5)` and others show one. The system's `nroff` makes of a page what the
build's did, byte for byte, which `tests/programs` checks with three
pages, and was seen to of `printf(3)` and `tcsh`'s too. Each page's
footer says `Printed` and the date it was formatted, as upstream's do:
here, the day it was built.

`usr/src/i286/man.list` says which pages go on the system. Of sections
1 and 8, those of the programs on it, wherever their sources keep them,
and the introductions; of 2, 3, 5 and 7, the kernel's calls, the
libraries, the files and the rest, what upstream's Makefile of each
formats, with the pages of those kinds that `cron` and `tcpd` keep
with them; of 4, those not of the PDP-11's devices: `tty`, `pty`,
`null`, and the network's. Section 6 is the games, and 3f Fortran's
library, which have nothing on the system to go with them. That is 542
pages, with the 307 other names upstream's Makefiles give them by `ln`,
`egrep` for `grep` and `fprintf` for `printf`, and 2.1 MB. `whatis` is
made as `usr/src/man`'s Makefile makes it: its `makewhatis.sed` takes
the name and the line under NAME of each page, and `sort -u` sorts
them. The system's own `sed` and `sort` make the same, which
`tests/programs` checks of sections 4 and 7.

`makewhatis.sed` took a page's section only from a heading that says
"UNIX Programmer's Manual", and took off only the underlining of a
name. The pages of mdoc, 45 of them, say "BSD Programmer's Manual" or
"BSD Reference Manual", and make the name bold, so `whatis lseek` said
"l^Hls^Hse^Hee^Hek^Hk (l)". It takes the section from any heading that
ends in "Manual" now, and takes off what is printed over, either way. A
bug on the PDP-11 too. `shar`'s page's heading names an institute,
and has no section in `whatis`.

What upstream does that is left as it is:

- `timedc`'s page starts with an empty page: its source starts with an
  empty line.
- mdoc's `.Dd` without a date gives the year as "19" and
  `tm_year`, 19126; no page here is without one.
- `man -f` and `man -k` are `whatis` and `apropos`, run by name from
  the `PATH`.

Left out: `troff`, which drives a phototypesetter; `tbl`, `eqn` and
`neqn`, `refer` and the rest that `nroff` is given documents through;
and of the manual, the pages of programs not on the system, and
upstream's `adduser(8)`, which tells how to add a user by hand, where
`adduser` here is a program of this port's own. Neither it nor `bench`
has a page yet.

`tests/programs` has the system format three pages, `ls`, `mkdir` and
`csh`, and make of each what the build did; format a document with
`-ms`, with `-me`, and with superscripts and fonts for its own table,
`-T37` and `-Tlpr`; show pages with `man`, by section, by another name,
and to say where one is; look pages up with `whatis` and `apropos`; and
make the lines of `whatis` for two sections again. At the terminal it
shows `ls`'s page with `man`, which `more` pages through.

## The root file system

`usr/src/i286/root.proto` lists it: the kernel and `/boot`, `/etc/init`,
`getty`, `login`, the shell and the programs, the devices, and `/etc`:
upstream's `gettytab`, `master.passwd`, `group` and `shells`; this
port's own `rc`, `rc.conf`, `netstart`, `ttys`, `fstab` and `motd`
(`usr/src/i286/etc`); root's `.profile`, `.cshrc` and `.login`, and the
short `/.profile` for the single-user shell, which are upstream's
(`usr/src/etc/root`), but for the erase character, which `.profile` and
`.login` make ^H only on the console's screens ([the
console](console.md)), and root's `.exrc`, this port's own
(`usr/src/i286/root`); and what the Makefile makes: `syserrlst`, the
terminal capabilities, and `passwd` with the password database. As
upstream has them, the capabilities are in `/usr/share/misc/termcap`,
and `/etc/termcap` is a symbolic link to that file. `fstab` names the
disk the root is to be on: the Makefile makes it `wd0`, the first on the
AT disk controller, unless `make root ROOTDISK=sd0` makes it the first
SCSI disk. The database, `passwd.dir` and `passwd.pag`, is what
`getpwnam` and the rest look users up in; without it they read
`master.passwd`, which only root may, and nobody else can tell whose a
file is. Upstream makes it and `passwd` with `mkpasswd -p
master.passwd`; here `tools/mkpasswd.py` does, and writes the same bytes
as `mkpasswd` on the 286, which `tests/system` checks. `/var/run/utmp`
is there from the start and readable by all, as upstream installs it:
otherwise `init` makes it under the kernel's umask, 026, and only root
can run `who`. The root has a `lost+found`, as a file system `mkfs`
makes does, a `/mnt`, and a `/home` for users. `/usr/skel` has a new
user's `.profile`, `.cshrc` and `.login`, which `adduser` copies;
upstream's `adduser(8)` names the directory but the tree has nothing in
it, so these are this port's own (`usr/src/i286/skel`), root's without
root's prompt and paths, and with the same erase character on the
console's screens.

Upstream's `rc` is the PDP-11's, with its buses and network interfaces,
and calls some things that are not built yet: the quota programs,
`savecore` and the network's other daemons. This port's checks the file
systems as upstream's does. At an automatic boot `fsck -p` checks each
in `fstab` that was not unmounted cleanly, the root first and alone; if
it had to change the root, which is mounted, `rc` starts the system
again with `reboot -n`, so that nothing the kernel held of the old is
written over what `fsck` wrote. From single user `rc` checks nothing,
that having been done by hand. `mount -a` mounts the rest of `fstab`,
and `dev_mkdb` and `ps -U` make the databases of devices and of the
kernel's symbols that `ps` reads ([programs that read the
kernel](#programs-that-read-the-kernel)). Then `rc` reads `/etc/rc.conf`, which names the machine and gives the
Ethernet interface's address, netmask and gateway, and runs
`/etc/netstart`, which sets up the loopback interface and, if `rc.conf`
gives an address, the Ethernet interface and the default route, as
upstream's `rc.conf` and `netstart` do for the PDP-11's interfaces; as
shipped there is no address, to be set for the machine. Then, in
upstream's order, it starts `syslogd`; has `expreserve` save what `ex`
was editing when the system went down; clears `/tmp`; starts `update`,
which has the kernel write what it holds for the disks every 30
seconds, and `cron`; turns accounting on ([the log, and what the clock
runs](#the-log-and-what-the-clock-runs)); and starts `inetd`, and the
daemons `rc.conf` asks for ([the network's
programs](#the-networks-programs)), among them `sendmail`'s, which is
off as shipped ([mail](#mail)). `inetd.conf`, `hosts.allow` and
`hosts.equiv` are this port's own, as is a `hosts` that names only
`localhost`; `services`, `protocols`, `hosts.deny`, `ftpusers` and
`ntp.conf` are upstream's ([the network](network.md)).

The programs: `init`, `getty`, `login`, `sh`, `csh`, `ls`, `cat`, `cp`,
`mv`, `rm`, `ln`, `mkdir`, `rmdir`, `chmod`, `chgrp`, `cmp`, `dd`, `df`,
`du`, `echo`, `ed`, `grep`, `kill`, `nice`, `od`, `pr`, `pwd`, `stty`,
`sync`, `tee`, `test` (and `[`), `time`, `who`, `wall`, `write`, `date`,
`hostname`, `hostid`, `pagesize`, `passwd`, `chpass` (and `chfn` and
`chsh`), `ps`, `sysctl`, `tcsh`, `expr`, `awk` and `make`, and in
`/usr/bin` `sleep`, `iostat`, `bench`, `find`, `crontab`, `at`, `atq`, `atrm`, `egrep`,
`bc`, `dc`, `yacc` and `lex`, and in `/sbin` `mount`, `umount`,
`mknod`, `fsck`, `mkfs`, `newfs`, `mkpasswd`, `dmesg`, `reboot` and
`halt`. `/usr/ucb` has `ex`, `vi` and `view`, one file under three
names, as upstream has them, `w` and `uptime`, one file under two,
`vmstat`, `top`, `logger` and `lastcomm`. `/usr/sbin` has `update`,
`vipw`, `chown`, `adduser`, `dev_mkdb`, `pstat`, `tracebuf`, `syslogd`,
`cron`, `accton`,
`sa`, `exrecover` and `expreserve`, and `/usr/libexec` `atrun` and
`acctd`. For the network there are `ifconfig`, `ping` and `route` in
`/sbin`, `inetd` and `arp` in `/usr/sbin`, `telnetd` in `/usr/libexec`,
and `telnet` and `netstat` in `/usr/ucb`. `tools/mkfs.py` makes these
other names as hard links, and also `[` and `halt`. It makes everything
root's unless the list says otherwise, as it does for `at`, `atq`,
`atrm` and their queue, which are `daemon`'s, and for the programs that
run with the group `kmem`. The programs that work on text and files are
listed [above](#programs-that-work-on-text-and-files), the
network's others [after them](#the-networks-programs), and mail's
[after those](#mail), with mail's files: `/etc/sendmail.cf`,
`/etc/aliases` and its database, an empty `/etc/sendmail.fc` and
`/var/log/sendmail.st`, `/etc/Mail.rc`, the help of `sendmail` and
`Mail` in `/usr/share/misc`, and the directories `/usr/spool/mail` and
`/usr/msgs`. `nroff`'s macros, its printers' tables and the manual are
[after those](#nroff-and-the-manual): `tools/mkman.py` writes a list of
the manual's pages and their links, which `root.proto` takes in with a
line `i ../../../obj/man/man.proto`.

The programs are in it without their symbol tables, as upstream's
`install -s` leaves them. `obj/bin` has each as it was linked, with its
symbols, which is where to look up the address a program faulted at; the
Makefile has `tools/strip.py` make of each the copy in `obj/root/bin`
that `root.proto` names. The kernel loads neither the symbols nor their
names (`kern_exec.c` reads the header, the table of segments and the
segments), so a stripped program takes the same memory and starts the
same; what it saves is an eighth of the file. `make root STRIP=no` copies
the programs as they are, for when the symbols are wanted on the system
itself, and making it either way after the other makes the copies and
the file system again. `/unix` keeps its symbols always: `ps` and the
rest look them up there ([programs that read the
kernel](#programs-that-read-the-kernel)). The system's own `strip` makes
of a file what `tools/strip.py` does, which `tests/programs` checks on
the kernel.

The file system is 12000 blocks of 1K, of which the system takes 7650.
The 220 programs are 3.7 MB of that, and 4.3 MB with their symbols, and
the manual's pages are 2.1 MB in 2500 blocks. It was 8000 blocks until
the manual came. A machine's own disk is made with as many as it has
room for (`make root ROOTSIZE=...`, or `tools/mkfs.py -s`).

## Following a program

The kernel's `sctrace`, set with `tools/kpatch.py obj/PC/unix sctrace=1`,
prints on the console each system call that fails, each call that is not
in this kernel, each program executed, each exit and each fault, as
`login[7]: call 149: error 22`. With 2 it prints every call.

The Bourne shell faults on purpose. As in the Seventh Edition, it
catches the segmentation violation it gets on running past its break,
moves the break up and goes on (`bin/sh/fault.c`). So each shell shows
a fault or two, as `sh[2]: trap 13 at F:12D`, and they are among the
last exceptions the harness lists when a test fails.

## Tested, and not

`tests/libc` boots the kernel with a test program as `/etc/init` on a
RAM disk. The program is built for both models; the small-model one is
`init`, and when it is done it runs the medium-model one, whose code is
packed into 8K segments so that it has four of them. Each checks, 74
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
file system, a swap area and room for another file system on an IDE disk
shared by a partition table, and boots it with no diskette. The root's
superblock is not marked clean, as a power cut leaves it, so `/etc/rc`
has `fsck` check it, which must mark it clean, and starts the system
again; the second time the root must be clean, and `rc` must start
`syslogd`, clear `/tmp`, and start `update`, `cron`, accounting and
`inetd`, which the console's screen shows. The system goes
multi-user and runs `getty`; the test logs in as root on the first
serial port, and in the shell makes a directory, copies and compares a
file, runs a loop, a second shell and a pipeline, expands a pattern, and
lists `/etc`. Then in `csh` it sets variables and a list and does
arithmetic, runs a `foreach` typed a line at a time, repeats a command
from the history, defines and uses an alias, tests files, reads
`$status`, makes an error, uses `pushd`, puts a job in the background
and kills it by its number, and times a command. Back in the shell it
edits a new file with `ex`: it appends two lines, substitutes in one,
makes a bad regular expression, which must give the message from
`exstrings`, and writes the file. It has `ex -r` (`exrecover`) list the
saved files, of which there must be none. In `vi`, on the serial port as
a VT100, it opens a line at the end and types into it, deletes the first
line, and writes the file, whose words the shell then prints. In `tcsh`
it prints `$HOSTTYPE`, edits a line with the line editor, completes a
file name, repeats the last command from the history with ^P, lists
`/usr/ucb` with `ls-F`, and puts a job in the background, lists it and
kills it. Then it logs in on two of the console's screens with the
keyboard and uses `vi` there ([the console](console.md)). Back on the
serial port it makes the password database from `master.passwd` with
`mkpasswd`, which must be the same as the one on the disk; adds a user
with `adduser`, taking what it offers but for the group, and gives it a
password; and lists the home directory, where the files from `/usr/skel`
must be the user's. It makes a file system in the spare partition with
`newfs`, checks it with `fsck`, mounts it, copies files to it, unmounts
it, and checks it twice more: `fsck -p` must leave it alone as clean,
and `fsck -f` must find the files. `ps axl` must show `inetd` waiting in
`select`, and `vmstat -s`, `iostat` and `pstat -T` must print their
counts. `dmesg` must show the kernel's three lines about memory, and
`/usr/adm/messages` must have them too, from when `syslogd` started,
and the line `logger` sends, which must be in `debuglog` as well. It
makes a file system in a partition of 100K and copies the kernel into
it, which does not fit: the kernel's complaint must reach
`/usr/adm/messages`, once another message has woken `syslogd`. `find`
must list the directories of `/usr/spool` and run a command on a file
it finds by name, `lastcomm` must list the `stty` of root's login, and
`sa -m` must have a line for root. `expr` must add, match a pattern and
take part of a string; `egrep` must count the lines that begin with
either of two names; `bc -l` must give 2 to the 64th and pi to ten
places, which is `dc`'s work, and `dc` must write 255 in base 16. `awk`
must format numbers, call the mathematical functions, pick a line of
`fstab` by one of its fields, count words in an array and split a
string. `make` must make a file from another by a rule that uses `$@`
and `$?`, and the second time find it up to date. And the system's
`yacc` and `lex` must make of `awk`'s grammar and scanner, on a file
system the test puts them on, the same C as the build's own made
([grammars](#grammars-yacc-and-lex)). It shows
the loopback interface with
`ifconfig`, pings it, and logs in over it with `telnet` as the new user,
with a wrong password first, which must be refused. The user's shell is
`csh`, which runs the `.cshrc` and `.login` from `/usr/skel`. `who` must
show the login on `ttyp0` from `localhost`, and the user must be able to
see who owns `/etc/passwd` and not to read `master.passwd`. The user
puts `sleep` in the background, which `ps axu` must show as the user's on
`ttyp0` with its arguments; `w` must show the user there; and `netstat`
must show the connection twice, from each end. The user has no table
for `cron`, gives it one with `crontab` that writes the date to a file
every minute, and queues a job with `at`, which `atq` must list as the
user's. The user
changes the full name with `chfn`, editing with `ed`, which
`/etc/passwd` must then show, and the password, which `passwd` must
first ask for, and logs out with ^D; `telnet` must report the connection
closed. Then the user logs in again with the new password, and out with
^D. Root's telnet over the loopback interface logs out with ^D too,
since `exit` in an interactive Bourne shell goes back to its prompt, as
upstream's shell has it (`exitsh` in `bin/sh/error.c`). It sets up the
Ethernet interface, an NE2000 of the emulator's on a network of its own,
by reading `/etc/rc.conf` and running `/etc/netstart` with an address
and a gateway given; pings the gateway; adds the default route again,
which `route` must refuse, `netstart` having added it; and has `arp`
show the gateway's address, and `route -f` flush the default route.
Root then gives the user's job in `at`'s queue a name that says its
time has come, and runs `atrun`, as `cron` does every quarter of an
hour; the file the job writes must be the user's, and so must the file
`cron` has written for the user by then, which the test waits for, up
to a minute. Meanwhile
the host logs in over telnet through that network, where `who` must show
the login from the gateway, and sends `/etc/services` three times into
`cat`, which must give a file the same as the system's copies; then the
system sends the first 20K of `/etc/termcap` back, which must arrive as
it is ([the network](network.md)). That telnet says it is an
`xterm-ghostty` in a window 200 columns by 80 lines: `TERM` must be
`xterm`, root's `.profile` must leave DEL the erase character, which the
shell and then `vi`'s insert mode must obey, `vi` must draw its last
line on the window's last, and in `vi` Home, End and Delete must go to
the top line, go to the bottom one and delete a character. Last it runs
`who`, and halts the machine with `reboot -h`. Afterwards
`tools/fsck.py` must find nothing wrong with the root or with either of
the file systems the test mounted and wrote on, and must find the
copied files.

`tests/programs` boots the same system from a disk with the root, a
swap area and a small file system of checks, logs in as root on the
first serial port, mounts that file system and runs its `check.sh`. For
each of the 82 checks in `tests/programs/checks` it runs the check's
lines with the shell in an empty directory and compares what they wrote
with what the check says they must, and says `ok` or shows what was
written. A check is of one program, by its name, or of one use of it:
`sort` has one for its options and one that sorts 6000 lines, more than
it holds in memory, so that it merges files; `diff` has four, among
them one of directories and one against an empty file; `nroff` formats
three of the manual's pages and must make of each what the build did
([nroff and the manual](#nroff-and-the-manual)). The test fails
unless all 82 say `ok`. Then, at the terminal, it runs `tty` and `mesg`;
pages through a file and through a pipe with `more`, and through `ls`'s
page with `man`; records a shell
with `script`, which it leaves with ^D, and finds the command it typed
in the file; locks the terminal with `lock`, gives it a wrong key and
then the right one; has `tset` name the terminal and `clear` write the
sequence that clears a VT100; and stops the system with `shutdown -h
now` and a message, which must be shown before the system halts.
Afterwards `tools/fsck.py` must find nothing wrong with the root.

`tests/network` boots it with the Ethernet card, logs in as root on
the serial port, adds the users `guest` and `ftp`, and runs the 33
checks of `tests/network/checks` in the same way. The first names the
machine, brings its interface up at the address the emulator's network
expects, and has `inetd` read its table again with `rexecd`, `tftpd`
and `ntalkd` turned on. The rest use each program against its daemon on
the machine itself: `finger` through `fingerd`; `tcpdmatch` and
`tcpdchk` on the rules, `try-from` run by `rsh` to say what `tcpd` knows
of a client, and an `ftp` that `tcpd` refuses and logs once the rules
leave `ftpd` out; `rsh` with a command's output, its errors, its input,
and a user who has no `.rhosts`; `rcp` each way and of a directory;
`rexecd`; `tftp` with a file each way, the 146K termcap, and one it may
not read; `ftp` as a user, with files each way in both modes, a
directory made and removed, a file renamed, appended to and deleted,
then as root and with a wrong password, which are refused, and as
`anonymous`, who may read and list and not write; `nc` both ways over
TCP, to `inetd`'s own `tcpmux` and `echo`, and scanning two ports;
`whois` against an `nc` that listens; `traceroute` to the machine and
to the gateway; `rwhod` with `rwho` and `ruptime`; `timed`, which makes
itself the network's master, with `timedc`; `ntpd` with `ntpdc` and
`ntp`, and `ntest`, in which `ntp` checks its arithmetic; `routed`
with its trace; and the 12 of mail ([mail](#mail)). Then root uses
`rlogin` to the user, with `.rhosts` and then with a password; from
there `rsh` and a second `rlogin`, which `hosts.equiv` lets through;
and `~.` to leave. The host logs the user in with `telnet`, which
`tcpd` now stands before; the user gives `biff y`, and `comsat` must
show the letter root then sends; root asks for the user with `talk`, the user answers what the daemon wrote on the
terminal, each sees what the other types, and an interrupt ends it for
both.

`tests/bench` boots it and runs `bench`, and is run for what it
measures ([performance](performance.md)). It is a test of the tools
for measuring: that each of `bench`'s 18 tests gives a result, and the
same time by the emulator's clock as by the system's; that the profile
taken meanwhile puts the sieve's time in `bench`'s `sieve` and the
least system call's in the kernel's `syscall`; that `bench` takes
tests by name and does twice the work when asked; and that `tracebuf`
prints its counts, `sysctl` four values by name and the whole list, and
`top` a screen with its own process on it, which `q` ends.

Not done, or not tested:

- The large model has no library, and the three helpers for `__huge`
  pointers are not written.
- `csh`, `ex`, `tcsh` and `sendmail` are the medium-model programs so
  far. The other 11 that the PDP-11 overlays (`named`, `uucp`, `jove`,
  `kermit` and some games) are the ones meant for it. `csh`'s file name
  completion, and `tcsh` as a login shell, have not been tried.
- `ex`'s saving of what it was editing when its terminal hangs up has
  not been tried, only its `preserve` command, which runs the same
  `expreserve`.
- Many programs are not built yet: of the network's, `named` and
  `identd`; the compiler and what works on object files, among them
  `/lib/cpp`, which `calendar` waits for; `troff`, and `tbl`, `eqn` and
  the rest that `nroff` is given documents through;
  the line printer's; and `disklabel`. With no `disklabel`, a partition
  can be made a file system only where `tools/mkdisk.py` labelled it
  one: the kernel mounts nothing else.
- Of the network's programs, `tests/network` has only the machine
  itself to talk to, and the emulator's gateway for one hop of
  `traceroute`: nothing has been tried against another system's `ftpd`,
  `rlogind` or `rwhod`, nor has a name been looked up with another
  machine's name server. `rwhod`, `timed` and `routed` hear only their
  own broadcasts there, and `ntpd` has a peer that never answers.
  `rexecd` is given its request by `nc`, there being no client for it
  but the library's `rexec`. `ftp`'s macros, its `.netrc` and its
  transfers between two other machines, `tftpd` writing anywhere but to
  a file made ready for it, and `talk` with a machine other than its own
  are not tried. No letter has gone to another machine, which
  `sendmail.cf` sends to a relay to be named for the network, or come
  from one but the machine itself.
- Of the programs that work on text and files, `tests/programs` gives
  each a check or a few, not every option. `su` is tried only from root,
  which is asked for no password; `lock` only with a key typed for it,
  not with the user's password; `shutdown` only at once and to halt.
  `more` is not asked to search or to start `vi`, and `tar` reads and
  writes files, not a tape or a diskette.
- `yacc` and `lex` are on a system with no C compiler to give their
  output to, and `liby` and `libln` are not built. `lex -r`, for
  Ratfor, `awk`'s saving and restoring of itself (`-S` and `-R`), and
  `make` with archives, with its built-in rules or with anything but
  the simplest description file, have not been tried. Of the tree's
  other grammars, `eqn`, `rdist`, `struct` and `named`'s tools wait
  for their programs.
- Nothing trims the logs or the accounting file. Upstream's `crontab`
  has `/usr/adm/daily`, `weekly` and `monthly` do that, and the tree has
  no such scripts. `acctd` stops when the file system is nearly full;
  `syslogd` does not.
- `iostat`'s seek times (`msps`) are 0. An IDE or SCSI drive seeks by
  itself, so its driver counts no seeks, as upstream's MSCP driver
  counts none; upstream's drivers count them where they give the drive
  a seek to do.
- `fsck` keeps its tables in what is left of 56K of data, as on the
  PDP-11: room for a file system of about 40 MB with an inode for each
  4K, `newfs`'s default. A bigger one needs a scratch file, `fsck -t`,
  which `/etc/rc` names on the root.
- A program that overflows its stack runs into its data without a fault
  ([processes](processes.md)).
- With every call printed (`sctrace=2`), the kernel's printing can stall
  the serial port's interrupt-driven output, and the terminal stops.
- Most of it has run in the emulator only. The system has also booted
  on a real machine and been logged in to over the network.
