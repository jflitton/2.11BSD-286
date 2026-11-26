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
so that both are there. They are where 2.11BSD has a site's own
programs: each in a directory of `usr/src/local` with a Makefile of its
own, which installs it in `/usr/local`, and a page in
`/usr/local/man`, which `man` looks in after `/usr/man` and
`/usr/new/man`. The csh's `path` has `/usr/local`, and root's `.profile`
not, as upstream has them: root names one in full,
`/usr/local/adduser`. The other build's Makefile lists them as
`OWNPROGS`. The first is `adduser`. 2.11BSD has `adduser(8)` only as a manual page that says how
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

The third is `ve`, an editor in the manner of `vim`, beside `vi`
([below](#ve)): one that fills a window of any size, where `vi` refuses
a window bigger than its copy of the screen ("Screen too large for
internal buffer"); that edits a file of megabytes, where `vi` has room
for 10,000 lines; whose Home and End go to the start and end of the
line, where `vi`'s Home goes to the top of the screen and it knows no
End; whose Page Up, Page Down, Delete and Insert work with no `.exrc`;
and which knows the cursor keys in insert mode too.

## Building

```sh
. tools/env.sh
make -C usr/src                 # the libraries, the programs and the
                                # manual, by their Makefiles
make -C usr/src root            # the same, and obj/root/root.fs, a root
                                # file system
make -C usr/src root STRIP=no   # the same, the programs with their symbols
```

The root file system needs the kernel and `/boot` made first
(`usr/src/sys/PC`, `usr/src/sys/i286stand`).

`tools/cc` is the cross build's `cc`: it compiles each `.c` with
`tools/cc286`, assembles each `.s` with NASM, and links behind `crt0.o`
with `libc.lib` last, converting the result to the system's format with
`tools/ne2aout.py`. It takes `-c`, `-o`, `-D`, `-I`, `-U`, `-L`, `-l`,
`-s` and `-stack N`, and `-E`, which writes what the preprocessor makes
of each file, and ignores the PDP-11's `-i` and `-n`.

`-mm` makes a medium-model program, `-ml` a large one, and `-ms`, the
default, a small one ([memory model](memory-model.md)). The medium
model's code is far, in as many segments as it takes, where the PDP-11
would have overlays; its data is the small model's, near and in one
segment. The large model's data is far too ([below](#the-large-model)).
Each model has its own library, in `/lib`, `/lib/m` and `/lib/l` as
the system has them, and `tools/cc` alone has them in `obj/lib/s`,
`obj/lib/m` and `obj/lib/l`. The linker would take objects
of different models together without complaint, and the program would
fail as it ran, so `tools/cc` reads the model the compiler records in
each object and library and refuses a mixture. The linker packs the code
of a medium-model program into segments of nearly 64K; `-packcode N`
makes them N bytes at most.

The programs are made by their own Makefiles, upstream's, as the
system makes them with `make build` ([the system makes
itself](self-hosting.md)), with `-mm` in a Makefile's flags for one that
is medium-model, or `-ml` for large. Where the system is built they are
run by that machine's `make`, with the system's commands as that
machine has them (`tools/build`), in a copy of `usr/src` in
`obj/nbuild`; what the commands said is in `obj/nbuild/log`, and what
each compiled, where and how, in `obj/nbuild/sources`. `usr.lib`'s
Makefiles make `libutil`, `libtermcap`, `libm`, `libcurses`, `libdbm`,
`libln` and the rest for each model, in `/usr/lib`, `/usr/lib/m` and
`/usr/lib/l`, and `libwrap` for `tcpd`. The build also needs a C
compiler for the machine it runs on, `cc` unless `HOSTCC` names another,
for the system's compiler and the tree's `yacc`, `lex` and `nroff`
([grammars](#grammars-yacc-and-lex)), which it runs there.

Nine programs are medium-model so far: `csh`, `ex` and `tcsh`, here,
`sendmail` ([mail](#mail)), `jove` ([below](#less-jove-and-tip)), and
the games `hack`, `phantasia`, `rogue` and `warp` ([games](#games)).
`csh` was linked with two overlays on the PDP-11; here its 67K of code
is in two segments, and its
data segment has 11K of data and 8K of stack, leaving about 45K for its
heap. `ex`, which is also `vi`, had two overlays as well. Its 82K of
code is in two segments, and its data segment has 35K of data and 8K of
stack. That leaves about 21K for the table of lines, two bytes a line,
or 10,000 lines. Of the data, 16K is `vi`'s copy of the screen, three
times the PDP-11's 5,600 bytes, since a window that `telnet` logs in
from is often far bigger than a terminal was: it holds 200 columns by 80
lines, and the widest window is 256 columns. `vi` refuses a bigger
window, as upstream's does, with "Screen too large for internal buffer".
Its error messages are in `/usr/share/misc/exstrings`, as on the PDP-11,
not in its data: `mkstr` takes each message out of a copy of the C and
puts the message's offset in the file in its place, and `xstr` gathers
the other strings into one array, as its Makefile has it.
`/usr/share/misc/termcap` is upstream's, with the commonest
terminals moved to the front, as its Makefile has `ex` do;
`tools/mktermcap.py` runs the same editor commands.

`vi` knows the arrow keys and Home from the termcap, and maps Home to
`H`, the top of the screen; upstream's xterm entry did not give Home,
and now does. Root's `.exrc` (`usr/src/etc/root`) maps End to `L`, the
bottom of the screen, Page Up and Page Down to ^B and ^F, Delete to `x`
and Insert to `i`, with `vi`'s own `map`, as an xterm and the console's
screens send them. Without such maps those keys' escape sequences are
commands to `vi`: Home and End open a line, and Delete and the page keys
change the case of a character.

`tcsh` had seven overlays. Its 143K of code is in three segments, and
its data segment has 27K of data and 8K of stack, leaving about 29K for
its heap. Its own `mkprstr` takes the format of each `xprintf` out of
its C and into `/etc/tcsh6.00strgs`, 5.8K, and makes the call one to
`mkprintf`, which reads the format from the file; `xstr` has its other
strings, as its Makefile has it. Three of its headers are made from its
C, as its Makefile makes them: `sh.err.h` with the 16-bit flags of its
error numbers, which its Makefile chooses with `unifdef`, `-Di286`
where it was `-Dpdp11`.

## The library

The tree's portable C, as `lib/libc`'s Makefiles list it, with
`lib/libc/i286` in place of `lib/libc/pdp`. It is built three times,
once for each model: the C compiled with `-ms`, `-mm` or `-ml`, the
assembly assembled with `-DFAR` for the medium and large models and
`-DFARDATA` for the large (`crt/DEFS.inc` gives the return, the call and
where the arguments start), and the stubs made for it. On the system
`lib/libc`'s Makefiles make it, upstream's
([the system makes itself](self-hosting.md#upstreams-makefiles)); where
it is built, `lib/libc/i286/GNUmakefile`, of the same files and in the
same order.

| | |
|---|---|
| `csu/crt0.s` | start-up: `main(argc, argv, envp)`, `environ`, `__progname`, and where the break is, which the kernel passes in BX. The first bytes of the data segment are left unused, so that nothing has the address a null pointer has. |
| `sys/syscalls.tab` | a line for each system call: `mkstubs` (`sys/mkstubs.c`, compiled where the library is made, there or on the system) writes a stub for each, one object apiece so that a program gets only the calls it makes. The Makefile has it check each line against the kernel's own table (`sys/sys/init_sysent.c`): the stub must pass as many words as the kernel takes. |
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

`fseek` has 4.4BSD's optimisation again, which upstream took out as of
no use on the PDP-11: a seek within what a stream has read moves in its
buffer, and one beyond it reads the block it lands in, where each seek
had thrown the buffer away and read it anew. `nlist` seeks to each
symbol's name, and `w`, `vmstat` and `top` spent two seconds and more
in the kernel finding a few; `w` takes 0.7 seconds now, where it took 3.
Upstream's `fseek` also left an `ungetc`'d character's flag set, and the
first read after the seek returned what was in the buffer before it.

`printf` and the rest put each character with `fputc`, which on an
unbuffered stream, as `stderr` is, was a `write` of its own: `getpass`'s
`Password:` came through `telnet` a character a packet. As 4.3BSD's
`fprintf` and 4.4BSD's `__sbprintf` did, what is printed to such a
stream is formatted into a buffer and written at once.

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
rewritten with `stdarg.h`; `ex`'s `printf` was the first, and `jove`'s
sixteen, for its messages, its prompts and the commands it runs, are
so too.

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
  (`_minbrk`). Upstream's `c2` takes the heap to start at `end` too;
  the 286's (`usr/src/lib/c2/i286`) has its space from `malloc`.
  `sendmail` freezes what lies from `end` to the break, which
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
flags the port's own Makefile then gave it, it agreed with the build, in that all
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
`install` gives them that group, which where the system is built is
noted for the root (`tools/build/install.py`), and `tools/mkfs.py`
gives a file a group when its mode in the list it is given is followed
by one, as in `2755:2`.

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

`top` draws with `libcurses`, which keeps two copies of the screen and
gave each line of a copy room for as many pointers as it has
characters, where it holds characters, a byte each. In a window that
`telnet` logs in from, 200 columns by 80 lines, those were more than
`top`'s data segment, and the first thing to find no room failed with
a message about something else (`/var/run/dev: Cannot allocate
memory`). At a byte a character that window fits, with little to
spare. `top` cannot be of the large model, as `window` is
([below](#the-large-model)): it reads the kernel's table of processes
into its own `struct proc`, whose pointers are two bytes in the kernel
and would be four there.

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
upstream's lines: `atrun` every quarter of an hour, and
`/usr/adm/daily`, `weekly` and `monthly` (below), each piped to `mail
root`. It adds `sendmail -q` each hour, to deliver what is in mail's
queue, which upstream leaves to `sendmail`'s daemon ([mail](#mail)).
`cron` mails what a job prints to the job's owner, through `sendmail`.
The times in the table are local ([time zones](#time-zones)).

`daily` runs at five past midnight. It starts a new file of accounting
records, has `sa` add the old one to its summaries, `savacct` and
`usracct`, and compresses it, keeping four; it keeps eight days of
mail's log and of `daemonlog` and `debuglog`; it has `msgs -c` remove
old messages; and it lists mail's queue. `weekly`, at quarter past on
Sunday, keeps four weeks of `messages` and `cron`, compressed, makes
`/usr/man/whatis` again with `makewhatis`, and has `updatedb` make the
list of the system's files that `find` looks a name up in when given
only the name, `/var/db/find.codes`. It runs that as `nobody`, so that
the list has only what anyone may see; the root file system has the
file, empty and `nobody`'s, for it to write. `monthly`, on the first,
totals each user's time logged in with `ac -p` and keeps six months of
`wtmp`. `bigram` and `code`, which `updatedb` runs, are in
`/usr/libexec/find` with it.

The scripts are the port's, made from upstream's, in `usr/adm` in the
tree in place of those of the tape. Upstream's `daily`
also cleans up after UUCP, and `weekly` too, which is not on the system
([what is not ported](unported.md)). And upstream's move each log's older copies
along with a fixed list of `mv` commands, which on a new system mail
root an error for each copy not made yet, 27 the first night. The
port's move only the copies there are.

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

`/etc/rc` clears `/tmp` with `find`.

## Time zones

`localtime` takes the zone the system is in, and when its clocks
change, from `/etc/localtime`, or from the file in `/usr/share/zoneinfo`
that a user's `TZ` names; without either it keeps universal time, which
it calls GMT. The root file system has no `/etc/localtime`: `zic -l
US/Pacific`, say, makes it a link to that zone's file. The kernel takes
the clock chip to keep universal time
([above](#the-log-and-what-the-clock-runs)), so on a machine whose chip
keeps local time, as DOS has it, the chip must be set to universal time
in the machine's setup: the kernel reads the chip only as it starts and
never writes it, so `date` sets the time only until the next boot.

`zic` makes the files from upstream's tables in `usr/src/share/zoneinfo`:
72 zones and 13 other names for them, those of the United States,
Canada, Europe, Australia, New Zealand, Japan and Singapore, System V's,
and `GMT+N` and `GMT-N` for each whole hour. `zdump -v` shows each
zone's changes. A file holds a zone's changes from 1901 to 2038, the
range of a 32-bit `time_t`, as on the PDP-11. The build makes them, as
upstream's Makefile has the system do, but with upstream's `zic` built
for the machine the build runs on. That makes the same files: a file's
numbers are four bytes, high byte first, on any machine, and
`tests/programs` has the system's own `zic` make them all again and
compare. The build's `zic` is built with a 32-bit `time_t`
(`tools/hostzic.h`), since it works out from the type which years to
cover: with the 64 bits of `time_t` on such a host it would write each
rule's changes up to the year 292 billion.

Patch level 500 brought the rules of the United States and Canada up to
those of 2007, but it ended the United States' rule that sets the
clocks back in October in 1973, so that from 1974 to 2006 summer time
never ended there, and left Canada's going on after 2007, so that its
clocks went back in October and again in November. Both rules now end
in 2006. Compared with the database of the machine the build ran on,
the four zones of the 48 states, Arizona's, Hawaii's, and Canada's
Atlantic and Eastern zones have every change from 1970 to 2038 right,
and Alaska's those from 1984. The rest are as upstream had them in the 1990s, and many of
their places have changed their rules since: Indiana, Newfoundland,
British Columbia, much of Europe, Russia, Australia and New Zealand,
among others.

`zdump -v` showed the second and the second-last of its times wrong: it
added and took off a day as `SECS_PER_HOUR * HOURS_PER_DAY`, 86400 in
an int, which a 16-bit int does not hold, on the PDP-11 as here. It
uses `SECS_PER_DAY`, a long, now.

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
the same source, agree. `liby`, the library that goes with `yacc`, and
`lex`'s, `libln`, which `atc`'s scanner uses too ([games](#games)), are
on the system for what its [compiler](#the-compiler) makes of their
output.

`awk` uses floating point throughout, and the mathematical library for
`sqrt`, `exp` and `log`. `libm` is `usr.lib/libom`, the older of
upstream's two and the one in C, which is what upstream installs under
that name on the PDP-11. As in the Seventh Edition, this `awk` reads its
input even when the program is only a `BEGIN`. A statement it could not
parse, followed by another, made it fault after it had said so: the
grammar's rule for a statement in error left it the value of the error
token, which `linkum` took for the next statement and followed, past the
end of the data segment. That rule now makes no statement, as the empty
statement's does.

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
| `/usr/sbin`, `/sbin` | `ac`, `chroot`, `flformat`; `shutdown` |

With them go what upstream installs beside them: the scripts `true`,
`false`, `whoami`, `groups`, `nohup`, `which` and `diff3`, which runs
the program of that name in `/usr/libexec`; the other names `e` for
`ed`, `u` for `users`, `uncompress` and `zcat` for `compress`, `page`
for `more` and `reset` for `tset`; and `units`, `eign` and `more.help`
in `/usr/share/misc`. `install` is `xinstall` in the tree.

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
passes of `lint`, and came with [the compiler](#the-compiler), as
`calendar` did; `spell` and `indent`, which came [with what documents
go through](#what-documents-go-through); the programs that read the kernel, `fstat`, `top` and `gcore`; those
for object files, `nm`, `ar`, `ranlib`, `size` and `ld`; and
the network's, which came next. `look` searches `/usr/dict/words` by
default, which is not in upstream's source tree but on its `/usr` tape,
and was imported from there with the games ([games](#games)).

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

`ntalkd` is `talkd` in the tree. `rcp`, `rlogin`, `rsh`
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
  variables, and so as static. (It also gave each line of a window
  twice the room it needs: [`top`](#programs-that-read-the-kernel).)

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
- `tcpd` knows a client's user only if the client runs `identd`
  ([below](#named-identd-and-rdist)), which `inetd.conf` leaves off, as
  upstream's does.
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

Left out of these: `comsat`, `biff` and `popper`, which came with
[mail](#mail); `rmt`, which came with [the administrator's
programs](#backups-and-the-rest-of-the-administrators); `named`,
`identd` and `rdist`, which came later ([below](#named-identd-and-rdist)),
and `tip` ([below](#less-jove-and-tip)). Not ported: `mkhosts`, for a
table of hosts that the library here does not read; `slattach`, since
the kernel has no serial line interface; `trpt` and `trsp`, since it
does not keep the traces they print; and `tn3270` and `uucp`
([what is not ported](unported.md)).

`tests/network` has the system run these, on the loopback interface
and, for the daemons that speak to the whole network, on the Ethernet
card: 24 checks in `tests/network/checks`, with those of `named`,
`identd` and `rdist` below, and `rlogin` and `talk` at terminals.

## named, identd and rdist

`named`, the name server, is upstream's BIND 4.8 as it stands, with
`nslookup`, `nsquery` and `nstest`. Upstream's Makefile links it without
overlays, which it says it wants only when compiled with `DEBUG` and
`STATS`, and here it is a small-model program too: 58K of code and 21K
of data, which with its 8K of stack leaves about 35K for the zones it
loads and the answers it keeps. `nslookup`, of 47K, has its commands'
scanner made by `lex`. `nstest` prints nothing unless the library's
resolver is compiled with `DEBUG`, as its Makefile says.

`/etc/rc` starts `named` before `inetd` when `rc.conf` says
`named=YES`, as upstream's does, and it reads `/etc/named.boot`, of
which upstream has examples in `usr/src/usr.sbin/named/master` and
installs none. The C library asks it: `HOSTLOOKUP` is `named` in
`lib/libc/Makefile`, as upstream has it, so `gethostbyname` asks the
server `/etc/resolv.conf` names, or this machine's when there is no such
file, and reads `/etc/hosts` when none answers. `tests/network` has
`named` serve a zone of its own and the zone of 127.0.0.1's name, asks
it with `nslookup`, and has `finger` find a host by an alias in that
zone.

`identd` tells another machine whose a connection to it is, for that
machine's `tcpd` or its logs. It finds the connection among the kernel's
TCP connections, the open file that is that socket, and the process that
has the file open, as `fstat` does. Upstream's reads the PDP-11's
kernel: the network's memory from `/dev/mem` at the click `netdata`
gives, the network's symbols in `/netnix`, and a user structure in
memory or on the swap device. The 286's
(`libexec/identd/src/kernel/2.11bsd.c`) reads the network's data segment
through `/dev/kmem` by its number, which `/unix`'s symbol `tcb` has, and
a user structure in the kernel's slot or in memory, as `ps` does
([programs that read the kernel](#programs-that-read-the-kernel)).
`inetd` runs it as the user `sys`, whose group is `kmem`, and
`inetd.conf` has it commented out, as upstream's does. `itest`, which
shows what it finds of the kernel, `idecrypt` and the script `identconn`
are installed with it, and `libident`, with which a client asks an
`identd`, is built for the three models as the other libraries are.
Its Makefile's `GDEFS`, with which it logs each question and its answer
(`STRONG_LOG`), followed a commented-out line that ended in a `\`, and
2.11BSD's `make`, which goes on past an empty line after one, read it
as comment too; that line ends now, and both `make`s read `GDEFS`.
`tests/network` asks it whose two connections are, one root's and one a
user's, and about one that is nobody's, and has `tcpd`'s `try-from` name
the user at the other end of an `rsh`.

`rdist` brings files on other machines up to date with this one's: it
has `rdist -Server` run on each through `rsh`'s daemon, whose port it
binds as root. Upstream links it with `libstubs`, whose `ctime` and the
rest asked `ctimed` for what the PDP-11's program had no room for; the
library's own are linked here. `tests/network` has a user install a
directory with it on the same machine, and bring a file there up to
date.

What the cross compiler needed of these ([what this compiler
needed](#what-this-compiler-needed)): `named`'s signal handlers are
defined `void`, as its `main` declares them. `identd`'s `error1`, whose
second argument is one word, an `int` or a string, is variadic, and its
`getbuf` takes a `void *`. `rdist` casts the null it passes for a
`char *`, and the 0 it passed `notify` for a `time_t`, a bug on any
16-bit machine; its `makesubcmd` has lost the parameter that no call
gave it. `mkhosts`, which compiles though it does not link ([what is
not ported](unported.md)), declares `fetchhost` before it uses it.

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

`sendmail` had three overlays on the PDP-11. Here its 75K of code is in
two segments, and its data segment has 15K of data and 16K of stack,
leaving about 33K for its heap. It takes the PDP-11's sizes for its
tables (`conf.h`): lines of 256 bytes, ten jobs to a run of the queue.
On the PDP-11 it is linked with `libstubs`, which has `ctimed`, another
process, do `localtime` and the like for it, to save room in its code;
here it is linked without, and has the library's own. Its Makefile
takes its messages out into `/usr/share/misc/sendmail.sr` with `mkstr`
and `xstr`, as `ex`'s are, and so they are here too. What the compiler needed: `syserr`, `usrerr`, `message`,
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

Left out: `calendar`, which runs the C preprocessor, `/lib/cpp`, and
came with [the compiler](#the-compiler); `rmail`, for mail by UUCP; and
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
`/usr/share/tmac/`. The Makefiles' `nroff` and `/usr/man/manroff`
give it each page, with what the page's `.so` lines name put in their
places (`tools/build/nroff.py`): another page beside it,
as `errno.2` is `intro.2`, or a header from the tree's `usr/include`,
as `utmp(5)` and others show one. The system's `nroff` makes of a page what the
build's did, byte for byte, which `tests/programs` checks with three
pages, and was seen to of `printf(3)` and `tcsh`'s too. Each page's
footer says `Printed` and the date it was formatted, as upstream's do:
here, the day it was built.

The pages are those `usr/src/man`'s Makefiles format, and those the
programs' Makefiles format with them, as on 2.11BSD, where the system
is built too. `man`'s lists are upstream's less what
the system has not: of sections 1 and 8, the pages of the programs on
it, and the introductions; of 2, 3, 5 and 7, the kernel's calls, the
libraries, the files and the rest, all of upstream's, with the pages of
those kinds that `cron` and `tcpd` keep with them; of 4, those not of
the PDP-11's devices: `tty`, `pty`, `null`, `fl`, and the network's; of
6, those of the games on it ([games](#games)); and no 3f, Fortran's
library. That is 621 pages, with the 317 other names upstream's
Makefiles give them by `ln`, `egrep` for `grep` and `fprintf` for
`printf`, and 2.4 MB. `whatis` is
made as `usr/src/man`'s Makefile makes it: its `makewhatis.sed` takes
the name and the line under NAME of each page, and `sort -u` sorts
them. The system's `makewhatis`, upstream's script, which does that with
the system's own `sed` and `sort`, makes the same, which `tests/programs`
checks of sections 4 and 7.

`makewhatis.sed` took a page's section only from a heading that says
"UNIX Programmer's Manual", and took off only the underlining of a
name. The pages of mdoc, 45 of them, say "BSD Programmer's Manual" or
"BSD Reference Manual", and make the name bold, so `whatis lseek` said
"l^Hls^Hse^Hee^Hek^Hk (l)". It takes the section from any heading that
ends in "Manual" now, and takes off what is printed over, either way. It
also made one blank of each run of them only after a letter, a digit or
a comma, and `nroff -h` writes a tab for spaces where it can: `ncheck`'s
line has two spaces after its dash, and was "8 ncheck -", a tab, and the
rest, with no section. A run after a dash is one blank now too, which
mends eight lines of the library's, "(3) -" with spaces after it. Bugs
on the PDP-11 too. `shar`'s page's heading names an institute,
and has no section in `whatis`.

What upstream does that is left as it is:

- `timedc`'s page starts with an empty page: its source starts with an
  empty line.
- mdoc's `.Dd` without a date gives the year as "19" and
  `tm_year`, 19126; no page here is without one.
- `man -f` and `man -k` are `whatis` and `apropos`, run by name from
  the `PATH`.

Left out: `troff`, which drives a phototypesetter, and of the manual,
the pages of programs not on the system, and
upstream's `adduser(8)`, which tells how to add a user by hand, where
`adduser` here is a program of this port's own. Neither it nor `bench`
has a page yet.

`tests/programs` has the system format three pages, `ls`, `mkdir` and
`csh`, and make of each what the build did; format a document with
`-ms`, with `-me`, and with superscripts and fonts for its own table,
`-T37` and `-Tlpr`; show pages with `man`, by section, by another name,
and to say where one is; look pages up with `whatis` and `apropos`; and
make `whatis` of copies of two sections with `makewhatis`. At the terminal it
shows `ls`'s page with `man`, which `more` pages through.

## What documents go through

Then what a document goes through on its way to `nroff`, and what reads
its words: 19 programs, with the scripts and files that go with them. 11
are compiled as upstream has them. All are small-model programs.

| Where | |
|---|---|
| `/usr/bin` | `tbl`; `eqn` and `neqn`; `refer`, `addbib`, `lookbib`, `sortbib`, and the scripts `indxbib` and `roffbib`; `spellin`, `spellout`, and the script `spell`; the scripts `style`, `diction` and `explain` |
| `/usr/ucb` | `indent` |
| `/usr/libexec` | `spell`, which the script runs; in `refer`, `deliv`, `mkey`, `inv` and `hunt`, which `refer` and the others run; in `diction`, `style1`, `style2`, `style3` and `dprog` |
| `/usr/dict` | `hlista`, `hlistb` and `hstop`, `spell`'s lists; `papers`, the references `refer` looks in unless it is given others, and `runinv`, which indexes them |
| `/usr/pub` | `eqnchar`, the characters `eqn` and `neqn` know by name; `ascii` and `greek` |
| `/usr/share/misc`, `/usr/share/tmac` | `dict.d` and `explain.d`, the phrases `diction` and `explain` look for; `tmac.bib`, `roffbib`'s macros |

`eqn` and `neqn` are one grammar for the tree's `yacc` and the same
files, `neqn` compiled with `-DNEQN`; `eqn` writes for a typesetter.
`style` runs three scanners made by the tree's `lex`, which include
`diction`'s tables of words, and `diction` runs `dprog`. `refer`'s
`hunt` is `/usr/libexec/refer/hunt`, apart from the game.

`spell`'s lists are made as its Makefile makes them, by `spellin`,
built for the machine the system is built on, as the games' tools are
([games](#games)): `hlista` and `hlistb` of `/usr/dict/words` with the
American spellings or the British, and the local words; `hstop` of the
words at which `spell` stops before it looks for a stem. Each is a table
of 400,000 bits as shorts, low byte first, on the PDP-11 too, and they
are the same bytes as the lists on upstream's `/usr` tape, which were
left out of the import as made of what is there ([the upstream
branch](upstream.md)). The system's `spellin` makes the same of `stop`.

`refer` finds a reference by an index of its keys, which `mkey` takes
and `inv` hashes. The hash is reckoned in an `int`, which overflows,
and shifts by a count that is negative when the sum is, which each
machine takes its own way: the index is the machine's own. The PDP-11's
of `/usr/dict/papers`, on upstream's tape, was left out with the lists,
and differs from the system's, its `.ib` 2172 bytes to 1982. The build's
machine, whose `int` is 32 bits, would make another with them:
`tools/runinv.py` makes it as `mkey` and `inv` do on the 286, the same
to the byte as the system's, where upstream's Makefile has `runinv`
make it as it installs `refer`.

What the compiler needed:

- Casts, where a function takes an `int` and is given a pointer, or the
  other way, before its definition in the same file, which this compiler
  holds the call to: `tbl`'s `domore` and `point`, `eqn`'s `error` and
  `define`, and `new` in `refer`'s `glue5.c`. `mkey`'s `chkey` and
  `refer`'s `corout` take a string they pass on, and declare it a
  `char *` now.
- `tbl`'s `badsig` and `sortbib`'s `onintr` are defined `void`, as they
  are declared; `inv`'s `recopy` returns 0 after an error that does not
  come back, where it returned nothing.
- `tbl` and `spell` and its two are compiled with `-Dunix`, which
  upstream's compiler defines and this one does not: `tbl` keeps its
  handler of `SIGPIPE` under it, and `spell.h` without it takes the
  Honeywell's table, of `int`s shifted by up to 31 bits.

What the machine needed:

- `tbl` keeps the PDP-11's limit of 30 lines of a table's format, under
  `#if defined(pdp11) || defined(i286)`, where the VAX has 100: its
  tables of each line's columns take 10K at 30 and 32K at 100.
- `spell` and its two hold a table of 50000 bytes, and `dprog` one of
  52000, and each has a stack of 4K, which leaves room in its 64K for its
  arguments and stdio's buffers. `refer` reads a reference into 4.5K of
  its stack below the 1K of its `main` and what it runs of `hunt`'s, and
  `sortbib` a record into 9K: each has 16K.

Bugs, which the PDP-11 has too:

- `indxbib` moved `db.ia_` to `db.ia`, where `inv` writes `db_.ia`: it
  said it could not, and left the index under names nothing looks for.
- `inv` writes the place of each reference in the index's `.ic` file as
  an `int`, and as a `long` when the file passes 64K. `getw` reads the
  `int`s signed, so a place past 32K was a negative number, which `hunt`
  compares with the places it holds of the first key: such a reference
  was found by one key and not by two. The `long`s were written with
  `putw`, two bytes each, where `inv` counted four, and read with
  `getw`. `inv` writes `long`s from 32K now, four bytes each, and `inv`
  and `hunt` read them so.

What upstream does that is left as it is:

- `nroff` with no `-T` does not move up the page, so a box's sides that
  `tbl` draws come out below the table. `nroff -T37 | col` draws them.
- `eqn`'s page is formatted as the others are, without `eqn`, which
  upstream's Makefile runs first: the build's machine has no `eqn`, which
  keeps pointers in `int`s, and the equations show as they are written,
  between dollar signs. `tbl`'s page has its table formatted without
  `tbl`, as upstream's Makefile has it.

`tests/programs` has the system lay out a boxed table with `tbl`; set an
equation with `neqn` for `nroff` and with `eqn` for a typesetter; index
`/usr/dict/papers` with `runinv`, cite two of its references with
`refer` in a document for `-ms`, look one up with `lookbib`, index
references of its own with `indxbib`, and 4000 more, whose index's `.ic`
is 43K, and find and cite the last; sort references with `sortbib` and
print them with `roffbib`; find misspelt words with `spell`, American
and British, and in a list with `spellout`, and make `hstop` again with
`spellin`; find wordy phrases with `diction` and one with `explain`,
measure a document with `style`; and lay out C with `indent`.

## Disks and file systems

Then the programs that read a disk or a file system directly, and
`fstat`.

| Where | |
|---|---|
| `/bin` | `disklabel` |
| `/sbin` | `dcheck`, `icheck`, `ncheck`, `clri`, `badsect` |
| `/usr/sbin` | `quot`, `makewhatis` |
| `/usr/bin` | `fstat` |
| `/mdec` | `wduboot` and `sduboot`, the boot block, for `disklabel -B` |

`disklabel` prints a disk's label, as the kernel has it or, with `-r`,
as the disk does, and writes one: from a description in the form it
prints (`-R`), from that description after an editor has changed it
(`-e`), or with the boot block (`-B`). It is how a partition is made a
file system on the system itself, since the kernel mounts only what the
label calls one ([disks](disks.md)). It writes through the raw device of
the disk's partition `a`, as upstream's does, and in multi-user the
kernel opens no disk's raw device for writing while the block device is
mounted (securelevel 1, `sys_inode.c`): the label of the disk the root
is on, which is its partition `a`, can be written only from single user,
where `init` puts the securelevel back to 0. Another disk's can be
written at any time, its partitions that are open kept where they are.
A disk with no label is given its first with `-r`, which writes the
sector, the kernel writing a label only over one; and after `-W`,
since the one partition such a disk has is open to `disklabel` itself
and the new label makes it smaller
([the system makes itself](self-hosting.md#a-disk)). Its `-w`, which makes the label of
a kind of disk that `/etc/disktab` describes, has no use here: upstream's
`disktab` describes DEC's disks, and is not installed. A description
has no line for the number of sectors on the disk, so a label made from
one counts them from its cylinders, which are the whole drive's, where
`tools/mkdisk.py` gives the size of the system's part: on a disk shared
with another system, `disklabel` would let a partition reach into the
other's part. The kernel refuses that label now as it refuses to read
one ([disks](disks.md#whose-disk-it-is)), and `disklabel` says "No
space left on device". `-B` puts `/mdec/wduboot`, for an IDE disk, or
`sduboot`, for a SCSI one, both `hdboot.s`, in the first sector of
partition `a`, which is the first of the system's part, so a disk can be
made one that boots from the system itself, `/boot` being a file to copy
to it. `disklabel` called its `Warning` with one to three arguments of
different sizes, and the function took six `int`s; it takes a variable
list now, as `fsck`'s do.

`icheck` reads every inode of a file system, from its raw device or
from a file, and says whether each block is in one file or on the free
list, and with `-s` makes the free list again; `dcheck` whether each
file's link count is the number of entries for it in directories;
`ncheck` names the files with given inode numbers, or every file. `clri`
clears an inode, and `badsect` makes a file whose block is a bad
sector's, so that nothing else is given it, which `fsck` then keeps as
an ordinary file. `badsect` gives the block to `mknod` as the device
number of a file of type `IFMT`, and the kernel kept it where a device's
number goes, in the inode's first address, which `fsck` takes for the
block. That is the address's low word on the PDP-11, whose longs have
their high word first, and its high word on the 286, so `fsck` found
block 750 as 49152000 and threw the file away. The kernel's `mknod`
sets the whole address for that type now. `dcheck` lists inode 1, as
the Seventh Edition's did: `mkfs` makes it, "a historical hack", an
allocated file with no links, which is what `dcheck` lists. `quot` adds up the blocks and files of each user.
These are the Seventh Edition's, and had bugs on the PDP-11 too:

- `dcheck` read the inodes twice, and took them for directories both
  times: the second pass, which compares each link count with the
  entries the first counted, never ran, and it found nothing wrong but
  entries for inodes out of range. It also read only a directory's
  first block, and took that for every block after it.
- `icheck` counted the double indirect blocks twice in the total it
  says are used, and the triple ones not at all.
- `ncheck` read the superblock's 1K block into a structure under half
  that size, over what follows it, and printed the number of a block
  it could not read as an `int`.
- `clri` read nothing to check that an inode could be read:
  `read(f, buf, DEV_BSIZE != DEV_BSIZE)`.
- `badsect`, given a sector out of range, said so for ever.
- `icheck`, `ncheck` and `quot` read the superblock as the size of its
  structure, which a raw device here refuses, moving whole sectors
  only ([disks](disks.md#devices)); they read the block it is in, as
  `dcheck` does. That also keeps `icheck -s` from writing what follows
  the structure in its memory over the rest of the block.
- `quot` never noted each user's number, so one with no name was `#0`;
  sorted users by name, which one with no name does not have; and put
  the name of user 300 one past the end of its table of 300. Its file
  system, when it is given none, is `/dev/rwd0a` here, where it was the
  PDP-11's `rxp0a` and `rxp0h`.

`fsck` is 4.3BSD's, made over for 2.11BSD's file system, and had bugs
of its own, on the PDP-11 as here:

- 4.3BSD numbers its inodes from 0 to one short of their count, and
  2.11BSD from 1 to `imax`. `fsck` kept 4.3BSD's bounds, and so left
  the last inode out: its first pass stopped before it, and its check
  of a directory entry took that inode's number for one out of range.
  The kernel fills the superblock's list of free inodes in order and
  takes from the end, so where no more than 100 are free the next file
  made gets the highest-numbered, on a new file system the last. `fsck`
  then called the directory corrupted, and salvaging it threw away the
  file's entry and every entry after it in the same 512 bytes. Those
  files went to `lost+found`, and the file itself was lost, its blocks
  put on the free list while its inode still had them. Its bounds are
  `imax`'s now.
- It never put right the superblock's count of free inodes, which the
  kernel only adds to and takes from, so after a crash or a `clri` the
  count stayed wrong; with `-d` it compared the count, wrongly by one.
  Pass 5 now counts the inodes the passes before it left unallocated,
  and puts the count right as it does the count of free blocks ("FREE
  INODE COUNT WRONG IN SUPERBLOCK").
- When the superblock's list of free inodes named one in use, as it can
  after a crash, `fsck` cut the list one short of where it should, and
  to -1 when the first was in use. The kernel, the next time it freed an
  inode, stored its number over the list's count.

The disks' devices are the group `operator`'s, as upstream's `MAKEDEV`
makes them, and `disklabel` and `quot` run with that group, as upstream
installs them, so that an operator may run them; the swap areas' block
devices are `kmem`'s, for `ps`.

`fstat` lists the files each process has open, its working directory and
its program's text, as upstream's does. It finds a process's user
structure as `ps` does, in the kernel's slot or at `p_addr`, where the
PDP-11's read it from the swap device for a process swapped out; and
the network's sockets in the network's segment, as `netstat` does,
where the PDP-11's read them at the click `_netdata` gives
([programs that read the kernel](#programs-that-read-the-kernel)). It
went on with a zombie, which has no user structure: it listed the files
of the process before it, and took the zombie's exit status, which
shares a union with `p_textp`, for its text. It leaves zombies out now.

`makewhatis` is upstream's script, which makes `whatis` with
`makewhatis.sed` of the pages in a directory of them, `/usr/man` unless
it is given another ([nroff and the manual](#nroff-and-the-manual)).

`tests/programs` has a second disk of 3.4 MB, the AT disk controller's
second drive, with room for a file system in partition `a` and partition
`e` unused. `disklabel` must print the first disk's label, the same from
the kernel as from the disk, and be refused writing it. On the second
disk it must make `e` a file system with `-R`, after which `newfs` makes
one there and `mount` mounts it, where `mount` refused it before; refuse
a label in which `e` runs 7 sectors past the system's part, which
`disklabel` lets through itself; give the label a name with `-e` and an
editor that is a script; and write the boot block again after a sector
of the kernel has been written over it. `badsect` makes a file of a
sector in `e`, which `fsck -y` keeps; `icheck` must find that sector's
block in it, `ncheck` must name every file, `dcheck` must find nothing
wrong, and `quot` must add up two users' files, and name each file's
owner from `ncheck`'s list. In a file system of its own in `a`, `clri`
clears the inode of a file, which `dcheck` must notice and `fsck` mend,
with the count of free inodes. In another there, of 16 inodes, the
first file made must get the last of them, and `fsck` must find nothing
wrong once every one is in use; then, with the superblock put back as it
was before the last twelve were made, `fsck -y` must cut its list of
free inodes and put right its count. `fstat` must list what a `sleep`
has open, who has `/etc/motd` open, `syslogd`'s socket in the UNIX
domain and `inetd`'s on the Internet. Afterwards `tools/fsck.py` must
find nothing wrong with the file systems in `a` and `e`.

## Backups, and the rest of the administrator's

| Where | |
|---|---|
| `/sbin` | `dump`, `rdump`, `dumpdir`, `restor` |
| `/usr/sbin` | `rmt` |
| `/bin` | `chflags` |
| `/usr/ucb` | `gcore`, `window` |
| `/usr/new` | `la` |
| `/etc` | `dumpdates`, the group `operator`'s to write, as upstream installs it |

`dump` and `restor` are the Seventh Edition's, as 2.11BSD has them, with
the format of `protocols/dumprestor.h`: 1K records in blocks of ten, a
header before each file, and a map of the inodes in use and of those in
the dump. `dump` reads a file system through its raw device, which only
root and the group `operator` may, and copies all of it (level 0), or
what has changed since the last dump of a lower level, which
`/etc/dumpdates` keeps with `u`; `W` and `w` say from that file and
`fstab` what is due. There is no tape drive yet, so a dump goes to a
file, named with `f`, or with `f -` to the standard output, from which
`restor rf -` can read it in a pipe, as `restor(8)` shows for copying a
file system. `rdump`, which only root may run, since it takes a
privileged port, writes it on another machine through `rsh` and `rmt`
there; `rmt` opens a file without making it, as 4.3BSD's does, so the
file must be there first. A file is taken for a tape of 2300 feet at
1600 bpi, which holds about 38000 blocks: at that size `dump` asks at
the terminal for the next tape and opens the same name again. `B` gives the number of
records a volume holds instead, and a dump to the standard output has no
limit. `dump` reads and writes with three helper processes, which take
their turns writing with `flock` on files in `/tmp`, as upstream's do.

`restor r` puts a dump back on a file system, inode by inode: a level 0
on one `newfs` has just made, and each level after on top of it,
removing the files that went in between. `x` takes the files it is
given out of a dump, each into a file named by its inode number in the
directory it is run in, and `t` says when the dump was made and from
what date. `dumpdir` lists the files in a dump with their inode numbers,
from which to name them for `x`.

These had bugs, on the PDP-11 too:

- `dump` read the superblock as the size of its structure, which a raw
  device here refuses ([disks and file systems](#disks-and-file-systems)),
  and went on with what was in memory: an inode count it made up, read
  past the end of the partition, and asked at the terminal whether to go
  on. It reads the block now, as `icheck` does. When it gave up after
  that it named the disk with `%d`.
- A file with a hole where a whole block of addresses would be, as `dd
  seek=300` makes, lost its place. `dump` wrote nothing for the missing
  indirect block, and `restor`, counting the file's blocks in order,
  put what came after the hole that much nearer the start: a block
  written at 300K came back at 44K. `dump` writes a record of no blocks
  for each block of addresses a hole takes the place of now, while the
  file goes on after it. A dump made by upstream's has the fault.
- `B` never worked: given it, `dump` left the tape's length at zero, and
  asked for a new tape after the first block. It counts the records of
  each volume now.
- `restor` wrote the superblock back as a whole block from the structure,
  with whatever followed the structure in its memory; it keeps the whole
  block now. `dump`'s messages, `msg`, took five `int`s whatever they
  were given, and take a variable list now.

`chflags` sets the flags the kernel keeps for a file, which `ls -lo`
shows: `nodump`, which `dump` honours at level 1 and above unless `h`
says otherwise; `uchg` and `uappnd`, which keep a file from being
changed or let it only be added to, and which its owner may set; and
`schg` and `sappnd`, the same, which only root may set, and nobody take
off in multi-user, where the securelevel is 1. `la` prints the load
averages, which it gets with `getloadavg`.

`gcore` writes the core of a running process without stopping it, or
stopped for the while with `-s`. Upstream's read the PDP-11's user
structure, data and stack at the clicks in its `proc` entry. Here it
writes what the kernel's own `core` does: the user structure and kernel
stack, the process's table of segments, and each of the process's own
segments, not the shared code. It finds the user structure as `ps`
does, in the kernel's slot or at `p_addr`, and the table in `pseg`
([programs that read the kernel](#programs-that-read-the-kernel)), and
runs with the group `kmem`. As upstream's, it will not write a process
that is swapped out. It looked one entry past the end of the table of processes
before saying a process was not there.

`window` divides a terminal into windows, each with a shell on a pty of
its own, and is upstream's, which leaves each window only the lines it
shows (`local.h`). It keeps its screen and its windows at about 15
bytes for each place on the screen, with the window it draws frames in
and the one it draws boxes in each as big as the screen. In 64K of data
that was room at 80 by 24 for the two windows it starts with and a few
more, and none at 100 by 30, where it said "Out of memory." and stopped.
So it is the first program of the large model ([below](#the-large-model)):
its windows come from `malloc`, which has segments from the system, and
it runs on a terminal of 160 by 60. One line of a window must still fit
a segment, and `wwalloc` says there is no memory for a bigger one, where
its sum of 16 bits wrapped. It took what `sprintf` returns for the
name of `~/.windowrc`: the buffer in 4.3BSD's library, and a count of
characters in 2.11BSD's, so it never read the file. `error` took eight
`int`s, and its table of commands ended with an initializer the
compiler refused.

`tests/programs` first has a raw disk keep its device number after `dd`
with `seek` and an open with `O_TRUNC` ([disks](disks.md#devices)).
Then it makes a file system on the second disk's partition `a` with a
file of 145K, which takes a block of addresses, a file with a
hole before its one block, which takes two, a hard and a symbolic link,
a device, and a file of another user's. `dump` must copy it to a file,
with `u`, and `dump W` must then name it; `restor t` and `dumpdir` must
list the dump; `restor r` must put it on a new file system in `e`, which
`fsck` must find whole and which must list as the first does, inode for
inode, with the same files; and so must `dump` and `restor` in a pipe.
After a file is changed, one made and a link removed, a level 1 dump in
volumes of 1000 records must hold those, and put on the copy it must
make it the same again. A level 2 dump must leave out a file marked
`nodump` with `chflags`, which a level 0 keeps unless `h 0` says
otherwise. `restor x` must take two files out of the dump; `rdump` must
write one through `rmt` on the same machine, which `restor r` puts back
whole. `gcore` must write a core of a `sleep` that is the kernel's own
after the user structure, which the kernel then writes when the `sleep`
is killed with signal 6; `chflags uchg` must keep a file from being
removed and written to until `nouchg`; and `la` must print three
averages. At the terminal, it starts `window`, runs a command in a
window on a pty, and leaves it with ^P `q`.

## The large model

A large-model program's pointers are all far, and its data may be more
than a segment ([memory model](memory-model.md) says how it is laid
out). `tools/cc -ml` makes one, and so does the system's own compiler,
as `cc -ml` on the system and `tools/cc -ccom -ml` where the system is
built ([the compiler](toolchain.md#models)). The library for it is
built beside the others (`make -C usr/src/lib/libc/i286`, in
`obj/nlib/l`, and with `TOOLS=cross` in `obj/lib/l`): the same sources,
with the start-up, `setjmp`, the signal trampoline, `modf`, `alloca`,
the byte-order functions and the system call stubs each in the
model's form, and its own `malloc` (`gen/lmalloc.c`) and, for the
cross compiler, helpers for `__huge` pointers (`crt/pia.s`).

`malloc` there asks the system for a segment (`segalloc(2)`) and hands
out pieces of it, making it larger 8K at a time up to 64K and then
asking for another, so a program has as much as the system gives a
process, 2 MB, and no one block of more than 65508 bytes. A freed block
is joined to free neighbours, and `realloc` grows a block where it is
when what follows is free.

What breaks in old C is an argument of the wrong size where there is no
prototype to say so: a `0` for a pointer, which is two bytes of the
four; a pointer where an `int` is taken; and a function that returns a
pointer and is called with no declaration, so that half its result is
lost. `tools/lmlint.py FILE...` compiles each file for the model with
the compiler's warnings on and puts them beside the definitions, in
those files, in files named with `-k` and in the C library, to report
each such call. The library had them in `execvp`, `rcmd`, `getmntinfo`,
`daemon`, `ndbm`, `popen`, `scandir` and `strtoimax`, the last a
function whose result was cut to 16 bits in every model; `window` had
them where it walks its variables and aliases. `NULL` is `0L` in this
model for the same reason.

`tests/libc` runs its checks as a large-model program too, and has more
for it: 400K from `malloc` in blocks, a block of nearly a segment and
one too big, `realloc` in place, and 150,000 bytes behind a `__huge`
pointer, or with the system's own compiler, which has no `__huge`,
reached with `SEGADDR`. `tests/ccom` runs a program the system's
compiler made of the model on the system, with 200K from `malloc`, and
its 1781 checks of the code the compiler generates, as a second
program of the model there; and the system's own compiler makes
`window` of it, as it makes every program of the root by default.

## less, jove and tip

Three more of upstream's that are used at a terminal. `less` is from
upstream's `local`, and so in `/usr/local`, as the port's own programs
are ([upstream's programs, and newer
ones](#upstreams-programs-and-newer-ones)); its help is
`/usr/local/lib/less.help`, which its `h` shows by running another
`less` by name, which root's `.profile` does not have on its path. It
is version 61, of 1986, configured by its Makefile as upstream's is,
for BSD's terminal and regular expressions; a small-model program of
24K of code. Its notice (`version.c`) lets it be used and changed
freely, but not sold for profit without its author's written consent.

`jove`, from upstream's `new`, is in `/usr/new`, with its help, the
system's `.joverc`, `recover`, which finds what it was editing when the
system went down, and `portsrv` in `/usr/new/lib/jove`, and `teachjove`
beside it. It is of the medium model, where the PDP-11 had it in six
overlays: 88K of code in two segments, and 27K of data, which with its
8K of stack leaves about 29K for what it allocates. As it builds, its
Makefile compiles and runs `setmaps`, which makes the table of its keys
from `keymaps.txt`; where the system is built that is compiled for the
machine it is built on (`tools/build/sh.py`), with `getline` hidden from
that machine's `stdio.h`, which declares one where `jove` has its own.
Its Makefile named the shell `jove` runs as `SHELL`, `/bin/csh`; the
make where the system is built runs commands with `SHELL`, which the
build gives it, so the Makefile names that shell `JSHELL`.

`tip` and `cu`, one program by two names, are from `usr.bin`, set-user
`uucp`, which owns their locks in `/usr/spool/uucp`, as upstream
installs them. `/etc/remote` is this port's: `com2` is the second
serial port, `tty01`, which `/etc/ttys` leaves for it, at 9600 baud,
`com1` the first, and `tip -9600 number` dials with a Hayes modem on
the second ([the console](console.md)). `/etc/phones` is upstream's,
and `/usr/adm/aculog` is where the dialing is logged. `/etc/rc` takes
away the locks that a system that went down left.

For the cross compiler ([what this compiler
needed](#what-this-compiler-needed)), `less`'s `signal.c` undefines the
`off_t` that the Makefile defines as `long` before it includes
`signal.h`, whose `sys/types.h` declares the type; `jove`'s `TermError`,
called with an argument or without, is variadic, as its functions
written with `varargs.h` are; and `tip` casts the null it passes for a
`char *`.

`tests/programs` has `less` copy files through, as it does when it is
not at a terminal, and at the terminal find a line and show its help;
gives `jove` a line and has it write the file; and has `tip` take the
second serial port and let it go.

## Printers, and the rest of upstream's

Then what was left of upstream's that waits on nothing the 286 lacks:
the line printer's spooler, `rmail`, `error`, `sendbug`, `trpt` and
`trsp`, and from `local`, in `/usr/local`, `afio`, `cxref`, `ddd`,
`mtools`, `qterm`, `welcome`, and `sz`, `rz` and `crc`. All are
small-model programs.

`lpr`, `lpq` and `lprm` queue, list and take away jobs and `lpd` prints
them, set-user root and set-group `daemon`, which owns
`/usr/spool/lpd`, as upstream installs them; `lpc` stops and starts the
queues, `pac` adds up what each user printed, and `lptest` writes the
ripple of characters a printer is tried with. The filters for text,
`lpf` and `necf`, are in `/usr/libexec/lpr`; those for Versatec and
Varian plotters the filters' Makefile installs only when asked
(`installv`). `/etc/printcap` is upstream's, whose `lp` writes to
`/dev/lp0`, the PDP-11's line printer: the 286 has no driver for its
parallel port ([what is not ported](unported.md)), so its printer is one
on a serial line, as the file's second entry, commented out, is, or
another machine's, which `rm` names and `lpd` sends the job to.
`/etc/rc` starts `lpd` always, as upstream's does, having taken away
`/dev/printer`, the socket by which `lpr` tells it of a job, which an
`lpd` that died leaves behind. `lpd` logs through `syslogd` to
`/usr/adm/lpd-errs`, which the distribution makes, and each printer's
daemon writes its filters' complaints to the file the printer's `lf`
names, which `lpd` opens for each printer as it starts. Upstream's `lp`
named `/usr/spool/lpd/errs`, which nothing makes, and `lpd` complained
of it on the console in the middle of `rc`'s line; it names
`lpd-errs` now, as upstream's other entries do. `lpr` will not print a
program or an object file, which it knew by the VAX's magic numbers;
`lp.local.h` gives it the 286's (`A_MAGIC` and `A_OBJECT`). Taking a
job from another machine, `lpd` opens the block device of the spool's
file system to see how much room it has, which the kernel refuses
while the file system is mounted and the security level is 1, as
upstream's does; it says so in `lpd-errs` and takes the job without
looking. A printer's daemon set the mode of a lock it had just made,
the first time the printer printed, from what it had not managed to
`stat`, whatever was on the stack (`printjob.c`): a printer could be
turned off, or its queue, by printing; it reads the lock's mode from
the lock now. Accounting (`af`) is in a file that must be made, owned by
`daemon`, which the filters run as.

`rmail` hands `sendmail` a letter that came by UUCP, with the address
it came by; it is ported though UUCP is not. `error` reads a compiler's
messages and puts each into the source file it is about, as a comment,
or says which it could not. `sendbug` mails a report of a bug, made with
the editor from a form, to the maintainer of the 286's port,
`jeff@flitton.dev`, where upstream's mailed `bugs@2BSD.COM`; `bugfiler`,
which files the reports where they are received, and its `unixtomh` are
installed with it, as upstream installs them, with upstream's address.

`trpt` prints what the kernel kept of the last things done to TCP
connections that have `SO_DEBUG` set, as `rsh -d` sets it: on the 286
the last two (`TCP_NDEBUG`), each a segment sent or received or a
request of the connection's user, and the state it left the connection
in. The kernel keeps them whether it is compiled with `TCPDEBUG` or not,
which only has it print them on the console too. The trace is the
network's, in the network's own segment ([programs that read the
kernel](#programs-that-read-the-kernel)), which `trpt` reads through
`/dev/kmem` with that segment's number in the high 16 bits, where
upstream's read at the symbol's value alone. `trsp` does as much for
the connections of SPP, the Xerox protocols', which the kernel has not:
it says that `/unix` has no names for it. It is installed as upstream
installs it.

From `local`: `afio` writes and reads archives in `cpio`'s format,
with more care for a damaged one; `cxref` lists where each name and
constant of C files is used, with its three passes in
`/usr/local/lib/cxref`; `ddd` copies as `dd` does with two processes,
one reading while the other writes; `qterm` asks the terminal what it
is, by the answers in `/usr/local/lib/qtermtab`; `welcome` greets the
user with the load, the date and the time, drawn with `curses`; `sz` and
`rz` send and receive files over a serial line by ZMODEM, YMODEM or
XMODEM, and `crc` prints the 32-bit CRC of files. `mtools` reads and
writes MS-DOS's diskettes: `mdir`, `mcopy`, `mformat`, `mmd` and the
rest. Its table of drives (`devices.c`) is the 286's: `A:` and `B:` are
the two drives' raw devices, `/dev/rfl0` and `/dev/rfl1`, which it
reads and writes a track at a time; it reads the diskette's shape from
its boot sector, of whatever kind the driver has found the diskette to
be ([disks](disks.md)).

Their notices ([COPYRIGHT](../COPYRIGHT)): `afio`, `cxref` and `qterm`
may not be sold, and `ddd` is for non-commercial use; `sz` and `rz` are
free for non-commercial and educational use, and ask a fee for each
serial port of a commercial or administrative one.

What the 16-bit `int` broke ([what the PDP-11 let programs
assume](#what-the-pdp-11-let-programs-assume)): `afio` made a time and a
size from two 16-bit halves of an old binary header by shifting an
`int`, which lost the high half, and divided by 1024 * 1024, which is 0,
to print a size in megabytes. `mtools`' `mdir` printed the year less
1900, 126 for 2026, a digit more than its buffer has room for; it
prints the last two. `mtools` keeps a cluster of up to 8K on the stack
as it writes one, and `afio` 8K as it copies a file into a directory
(`-p`): with the 8K of stack a program has unless it asks for more
(`-stack`, [executable format](executable-format.md)), the stack ran
over the data below it, and `mtools` wrote at offsets that were not
the diskette's. Both
are linked with a stack of 16K, as `ftpd` and `Mail` are.

Two of `ddd`'s bugs are any machine's: it opened the file it writes
with `O_CREAT` and no mode, so that the file had whatever was on the
stack, and it said it had failed when the copy was done and the
process that read the end of it was not the one the shell waits for,
which of the two is a matter of chance.

`qterm`'s Makefile links it by running `ld` itself, with `crt0.o` and
the C library; where the system is built `tools/build/ld.py` stands in
for that `ld`, as the stand-ins for `as` and `cpp` do: the system's own
`ld` with the root's libraries, or for the cross compiler its `cc`,
which brings its own.

For the cross compiler ([what this compiler
needed](#what-this-compiler-needed)), the functions that pass on their
arguments to `printf`, each called with a different number, are
variadic: `lpr`'s `fatal`, `lpd`'s `status` and `frecverr`, `lpq`'s
`standout`, `error`'s `error` and `inquire`, and `sz` and `rz`'s `vfile`
and `zperr`. The plotters' filters declare `clear`'s parameter.
`trpt` and `trsp` cast the address of the control block they pass;
`trpt` prints the sequence numbers, which are `long`, with `%lx`, and
names a segment's flags with a string of its own for each, where its
macro had the flag's name put inside a string, as a preprocessor of
the time did and the system's does not.

`tests/programs` has `lptest` write its ripple, `crc` sum two files,
`cxref` list a program's names, `error` put a compiler's complaint into
the source, `afio` write an archive and read it back, `ddd` copy 160,000
bytes, and `welcome` greet; `mtools` format a diskette in the second
drive, copy files to it and from it, and make and take away a
directory, and the test then reads the file it left there from the
diskette's image as MS-DOS would; and at the terminal `qterm` ask the
terminal what it is. `tests/network` has a printer that is a file, and
the same printer by way of the network (`rm=localhost`, which
`/etc/hosts.lpd` lets in): `lpr` queues jobs to both, `lpq` lists them,
`lprm` takes one away, `lpc` stops and starts the printing, and `pac`
adds up the pages; `rmail` and `sendbug` mail a user; and `trpt` shows
the trace of an `rsh -d`.

## troff, graph and plot

`troff` is `nroff`'s twin for the C/A/T phototypesetter, made from the
same sources (`usr.bin/roff`). It reads the width tables of its fonts
from `/usr/share/font`, which come from the distribution tape
([the upstream branch](upstream.md)) as the tape has them: each is a
PDP-11 `a.out` whose 224 bytes of widths `troff` reads past the 16-byte
header, by `8 * sizeof (int)`, which the 286's `int` keeps.
`tools/build/tape.proto` puts them in the root. `troff -t` writes the
phototypesetter's codes to the standard output, and `troff -a` an
approximation in ASCII; without either it opens `/dev/cat`, which the
286 has not, and says "Typesetter busy." `tc` shows what `troff -t` wrote on a Tektronix 4015,
and `tk` pages `nroff`'s output on a 4014, waiting for a Return at the
end of each page and at the end. `vgrind` sets programs in type for
`troff`; `vgrind -f` writes what it would give `troff`. `mkfont`, which
makes a width table from a font's widths (`troff_font`), is made and,
as upstream's, not installed.

`graph` draws a graph of pairs of numbers as `plot(5)`'s commands, and
`plot` gives them to the filter for a terminal or plotter: `tek`,
`t4013`, `t300`, `t300s`, `t450`, `bgplot`, `gigiplot`, `aedplot`,
`hp7221plot`, `hpplot`, `implot`, `crtplot`, which draws with `curses`,
and `dumbplot`, which draws with characters on any terminal; `plottoa`
and `atoplot` turn `plot(5)` into text and back. `libplot` has
`plot(3)`'s routines for each device, `-lplot` for `plot(5)` itself, in
the three models, as `libmp` (`mp(3)`, integers of any size) and
`libvmf` (`vmf(3)`, virtual memory kept in a file, as 2.11BSD's `ld`
had it) are.

`tn3270` is a `telnet` that is a 3270 terminal to an IBM host, and
`mset` prints the keyboard's map for `MAP3270`. It is made with
`genbsubs.c`, the C that its Makefile has beside the PDP-11's assembly.
Its notice asks that the documentation say that copying and
distribution are by permission of the University of California and of
Gregory Glenn Minshall ([COPYRIGHT](../COPYRIGHT)). `ansitape` and
`vmsbackup` are not made: they wait on a tape driver
([what is not ported](unported.md)).

What the 16-bit `int` broke ([what the PDP-11 let programs
assume](#what-the-pdp-11-let-programs-assume)): `dumbplot` scaled by
`LINES * y`, more than an `int` holds once `y` is past 1365, and
`aedplot` by its screen's pixels shifted up 12 bits; both are done in
`long`s. `libmp` adds into a `long` and takes its two halves through a
structure, `struct half` in `mp.h`, whose high word came first, as the
PDP-11 keeps a `long`: the 286 keeps the low word first, and `mp.h`
says so for it.

Bugs that are any machine's, which showed on the 286:
`hpplot`'s macro that scales a point cast the difference alone to `int`,
and passed a `double` to a function that takes an `int` and two
pointers, which it then wrote through; `dumbplot`, `bgplot`, `gigiplot`
and `hp7221plot` declared `closepl` as a variable (`sig_t closepl;`),
and handed whatever was on the stack to `signal` for an interrupt;
`libvmf` called `calloc` undeclared, which in the large model lost the
segment of the pointer it returned. `t300`, `t300s` and `t450`, the
filters for DASI's terminals, fall off the end of `main`, so their
status is whatever was left, as upstream's is.

For the cross compiler ([what this compiler
needed](#what-this-compiler-needed)): `tn3270`'s `call` and
`printoption` are made as `telnet`'s are, its own `KS` and `KE` are named
apart from `curses.h`'s, and `count - = n` is `count -= n`; `tk`'s `ex`
is `void`, as `tc`'s is; `plottoa` casts the pattern it passes `dot`.

`tests/programs` has `libmp` work out 2 to the 100th and the rest in
each model, `libvmf` keep 40 segments in 4 buffers in each model, and a
program draw with `libplot` in each model, which `plottoa` and `atoplot`
turn into text and back; it links that program with each device's
library in each model. `graph` draws the squares, `dumbplot` shows the
graph, and each filter draws it; `troff -a` and `troff -t` set a line,
which `tc` shows; `vgrind` sets a program; `mset` prints a map; and at
the terminal `tk` pages a file and waits for the Return.

## ve

`ve` is a screen editor with the modes and the most used commands of
`vim` (`ve(1)` has the list): counts, registers, the operators with
motions and text objects, the visual modes, `.`, undo and redo of the
last 40 changes, searches with patterns, and the commands after a colon,
among them `:s`, `:g`, filters and reading from a command. It is this
port's own, in `usr/src/local/ve`, and compiles with either compiler.

- **The file is not in its data segment, nor all in memory.** The text
  is in blocks of 4K, each holding whole lines one after another; what
  is known of a block, how full it is, how many lines and its
  neighbours, is in a table, and a line is copied to a buffer in the
  program's own data to be looked at or changed (`text.c`). The blocks
  that are in memory are in segments `ve` asks the system for
  (`segalloc(2)`), and so is the table: `ve` is a medium-model program
  with far pointers in one file and five assembly routines, two to copy
  through them, one to look for a byte, and `read` and `write` with a
  far address (`fmem.s`). There are as many blocks in memory as the
  cache holds, 128K unless `:set cache` says otherwise, 32K to 960K;
  when there are more, those not used lately go to a file in `/tmp`,
  which has no name once it is open, and are read again when they are
  wanted. So `ve` is no larger with a file of megabytes than with one
  of 128K, which matters where the kernel swaps whole processes and
  several people are editing; and a file that fits in the cache, as
  most do, never has the file in `/tmp` made. The text may be 16 MB,
  the file, the registers and what undo keeps together.
- **`/tmp` filling is seen to where something can be done about it.**
  A new block is given its memory when it is made, another going to
  `/tmp` for it if need be, and when that cannot be written the text
  does not grow: a file is read as far as it went and marked read
  only, so that it is not written back cut short, and a change is made
  without being kept for undo, and `ve` says so. Reading a block back
  never needs room: one place in memory is kept free for it, and the
  block that then goes out takes the place in the file of the one that
  came in, if it has none of its own.
- **A file is read as it is wanted.** Enough of it to fill the window
  is read before the window is shown, and the rest while nothing is
  typed. What moves over the part that is there is done at once; a
  search reads as far as it goes; anything else, `G` or a change,
  waits for the rest, and ^C ends the wait. `:q`, `:e` and `:n` do not
  wait.
- **Undo keeps lines, not commands.** What a command changed is one run
  of lines: where it starts, how many it is now, and the lines it was.
  To undo it the two are exchanged, a line at a time so that it needs no
  memory, and the record is then what does the change again (`edit.c`).
  When there is no memory to keep a change it is made all the same, and
  `ve` says that it cannot be undone.
- **The screen is its copy of the terminal**, a byte for the character
  and one for whether it stands out, in two more segments, so a window
  may be as big as 65,000 places. A row is made as it should be and
  compared with the copy, and only what differs is written; a line put
  in or taken out, or the window moving over the file, is done with the
  terminal's scrolling region or its insert and delete line
  (`screen.c`). It uses termcap, and knows the usual sequences of the
  cursor keys, Home, End, Insert, Delete, Page Up and Page Down
  whatever the terminal's entry says. A long line is not folded: the
  window moves sideways.
- **It reads `$HOME/.verc`** for `:set` commands, and on a hangup writes
  what was being edited to *file*`.save`.

On the emulated 286, with a file of 20,000 lines and 960K and the cache
at its 128K, so that seven eighths of the file is in `/tmp`: the window
is there at once, and `:q` takes half a second; reading all of it takes
13 seconds, 8 for the file and 5 to put most of it in `/tmp`, and
writing it 13; taking out half its lines and undoing that, 40; a
search that goes through all of it and finds nothing until the end, up
to 25, where every line begins as the pattern does; `:%s/abc/X/g`,
which changes a line in four places or so, 74; and a `:g` that takes
out every third line, 110. Each pass over the file is a megabyte read
from `/tmp`, and what it changed written back. A file that the cache
holds has none of that; the editor's own time for what is done to it
is about a tenth more than when all the text was in memory, which is
what finding a block in the cache costs.

`tests/ve` checks it on the machine the system is built on, where it is
built from the same sources with `tests/ve/host` in place of the 286's
segments and terminal: 115 checks, each typing keys at it and comparing
the file it wrote, or the screen it drew as a terminal would show it.
`tests/programs` runs it on the system.

## Games

Then the games of `usr/src/games`: all 38 of upstream's, and 41 programs
with those that go with them. `games/pdp` is the PDP-11's assembly.

| Where | |
|---|---|
| `/usr/games` | `adventure`, `arithmetic`, `atc`, `backgammon`, `banner`, `battlestar`, `bcd`, `boggle`, `btlgammon`, `canfield`, `cfscores`, `cribbage`, `factor`, `fish`, `fortune`, `hack`, `hangman`, `hunt`, `mille`, `monop`, `morse`, `number`, `phantasia`, `ppt`, `primes`, `quiz`, `rain`, `robots`, `rogue`, `sail`, `snake`, `snscore`, `teachgammon`, `trek`, `warp`, `worm`, `worms`, `wump`, and the scripts `wargames` and `words` |
| `/usr/games/lib` | `hunt.driver`, which `hunt` starts; `busy`, which `snake` asks how busy the system is; `words1`, which writes the `sed` script `words` runs; and the games' data, scores and logs |
| `/usr/man/cat6` | their pages: those of `usr/src/man/man6` and of `atc`, `battlestar`, `hack`, `phantasia` and `sail`, which keep theirs with them |
| `/usr/dict` | `words`, which `hangman`, `words` and `boggle`'s dictionary are made of, and `look` searches; `connectives` and the `README` |

`/usr/dict` is not in upstream's source tree but on its `/usr` tape,
from which it was imported, unchanged, as `usr/dict` ([the upstream
branch](upstream.md)). The root has `words`, `connectives` and the
`README`, with their modes and owners on the tape. `web2` and `web2a`,
Webster's Second and its phrases, 3.4 MB together, are not on it:
nothing reads them unless it is asked to, and there is no room for them
in the root of the tests' disks. `refer`'s papers and `spell`'s lists
came with them ([what documents go
through](#what-documents-go-through)).

Each is installed as its Makefile installs it. `fortune`, `hack`,
`phantasia`, `robots` and `rogue` run as `daemon`, mode 4711, and `sail`
and `snake` as `daemon` with mode 4755; `warp` runs as `bin`; `mille`
and `rogue` are the group `staff`'s. The scores and logs of those that
run as another user are that user's, and all are made as their Makefiles
make them: empty where the Makefile copies `/dev/null`, as `cfscores`,
mode 777, and `battlestar.log`, 666, are. `/usr/games/lib` is the group
`staff`'s, as on upstream's tape, and so is what is put in it, as a new
file takes its directory's group. `atc`'s Makefile is 4.3BSD's, and
2.11BSD's games Makefile leaves it out: it installs `atc` as
`/usr/games/hide/atc`, mode 4700, for `dm`, the program that says when
games may be played, and makes `/usr/games/atc` a link to `dm`. There is
no `dm` in 2.11BSD; here `atc` is `/usr/games/atc` itself, set-user-id
`games` with mode 4711, and its games and scores are in
`/usr/games/lib/atc`, mode 700, as the Makefile has them. `atc`'s
scanner wants `yywrap` from lex's library, `libln`, which the Makefile
builds now.

Four are medium-model programs, as the PDP-11 overlaid them: `hack`,
with 141K of code in three segments and 43K of data; `rogue`, 71K and
25K; `phantasia`, 74K and 24K; and `warp`, 74K and 28K. The rest are
small; `sail`, with 51K of code, is the largest.

What the Makefiles make as they build is made here on the machine the
system is built on. Programs of the games' own that write text or C are
built for that machine, as `yacc` and `lex` are; what is not text is
written by a tool of this port's own, laid out as the 286 has it:

- `fortune`'s `fortunes.dat` is `scene` and `obscene` made into a table
  by `strfile -r`, which `tools/strfile.py` stands in for: the number of
  fortunes and the offset of each, longs, then the text. The `-r` puts
  each section's fortunes in a random order, which `fortune` keeps to,
  taking the next each time it is run; `strfile` seeds its generator
  with the time, and `tools/strfile.py` shuffles as it does with the C
  library's `rand()` from its first number, so that each build makes
  the same file. It is upstream's file but for that order.
- `monop`'s cards, `cards.pck`, are packed by `tools/initdeck.py` as
  `initdeck` packs them: the same bytes as upstream's file but for the
  order of a long's words, and a pointer, which `monop` sets as it reads
  the file and upstream's has as `initdeck`'s memory had it; `initdeck`
  writes it as 0 now.
- `phantasia`'s files are made by its `setfiles` as it is installed;
  `tools/setfiles.py` makes them: its file of characters, with the one
  `<null>`, and where the grail is, both structures with doubles, and
  the empty files. `setfiles` never seeds `rand()`, so where it puts
  them is the library's sequence from its start, which is the same here;
  what it did not set of a structure it wrote as its stack had it, and
  writes as 0 now.
  Its help, `phant.help`, is formatted with `tbl` and `nroff -Ttn300`.
  `tbl` kept the pointers three of its functions return in `int`s,
  which the machine the system is built on, whose pointers are longer,
  could not run; they are declared now, and the help has its two
  tables laid out there as on the 286.
- `battlestar` keeps the descriptions of its places and things in
  `/usr/games/lib/battle_strings`, which its own `mkstr` makes of three
  of its files as it takes the strings out of them, leaving each
  string's offset in the file. Here it is the same, byte for byte, as
  upstream's.
- `warp`'s maps of the stars are made by its `sm` from pictures of them,
  `cribbage`'s instructions by `nroff`, and `atc`'s grammar and scanner
  by `yacc` and `lex`; the maps and the instructions are the same as
  upstream's.
- `boggle`'s dictionary, `bogdict`, is the words of `/usr/dict/words`
  that it can use, `qu` written `q`, which its `sfile` has `sed` choose,
  each written by its `comp` as how many letters it shares with the one
  before and then the rest. It is the same, byte for byte, as upstream's.
- `hack`'s Makefile has each file run through the preprocessor and then
  its `crypt`, which shortens words in its messages, "experienced" to
  "xprncd", to save the PDP-11's room. The build does the same, its
  `cc -E` being the system's own preprocessor, `/lib/cpp`, where the
  system's compiler makes the programs. The
  system's headers give the cross compiler its conventions in `#pragma`
  lines with instructions in strings, and `crypt` took the "and" of
  `alloca`'s for a word; with `-Di286` it leaves lines for the
  preprocessor alone. `hack.onames.h`,
  which `makedefs` writes, is in the tree as it writes it.
- `hack` and `monop` also run their C through `xstr`, which gathers the
  strings into one array so that one copy of each is kept, as their
  Makefiles have it. `backgammon`'s Makefile has
  `ex` put the day it is built in `message.c`, which is compiled as the
  tree has it, with the day of upstream's own build.

`adventure` is made on the system, by its users, as it was on the
PDP-11. Its Makefile builds a program that, run once, reads the game's
text, `glorkz`, into its tables, saves itself with them as a new
program, `advent`, and puts the messages after it, where the game reads
them from its own file as it plays; and installs that as
`/usr/games/adventure`. The same `save` is how a player suspends a game,
as a program that takes it up again. The build makes the first program,
and the root has it as `/usr/games/lib/adventure/a.out`, with `glorkz`
and a Makefile of this port's (`tools/build/adventure.mk`) that does the
rest as upstream's does, `echo y | ./a.out` and `install -s`. So the
game is there once root has said

    cd /usr/games/lib/adventure; make install

and answered nothing: the Makefile says "y" to "got the data.  save
as "advent"?". `strip` leaves `advent` as it is, having no symbols,
and so with its messages.

`save` writes the PDP-11's, the VAX's or the Z8000's `a.out`, and
`adventure` has an `#ifdef i286` of its own for the 286's: the header
and the code from the program it was run as, and the data segment as it
is in memory, initialized throughout. On the PDP-11 the heap follows the
BSS, and was saved with it; here the stack and the arguments lie between
them, and the heap begins higher the more room the arguments and the
environment take. So the first run moves its break up by as much as
they can take, 3 times `NCARGS` and a click, before it reads `glorkz`;
`save` writes the heap after the segments; and when the saved program
starts it reads its heap back to where it was, below which any run's
break begins. `advent`, and a game suspended, is 99K with the messages,
as upstream's is on the PDP-11.

What the compiler needed:

- `arithmetic`'s `delete` was declared `void` and defined as an `int`;
  `number`'s `cprint` and `atc`'s `yyerror` declare that they take a
  string.
- `battlestar` switched on a pointer to a function, a signal's handler,
  which is an `if` now, and `trek` on a pointer to a function used as a
  number, which is cast. 11 of `trek`'s commands returned what `printf` or
  `out` returns in some places and nothing in others; they return
  nothing.
- `cribbage`'s `msg` and `addmsg`, and `hack`'s `panic`, walked their
  arguments from the address of the first; they are variadic, with
  `stdarg.h`.
- `hunt` declares its signal handlers `void` before they are used and
  casts a null pointer it passes as a string; `robots`, `atc` and
  `monop` cast handlers that take a `char` or return an `int`.
  `terminal.c` gives `hunt`'s `sendcom` the arguments it leaves off.
- `monop` reached a square's description through a `char *` with the
  members of the structure it is: the pointer is to that structure now.
  `teachgammon` declared an array it never used without a size.
- `libln`'s `reject.c` declared `lex`'s variables in its first function,
  which PCC let the next one use, and declares them outside it for the
  286 as for the PDP-11.

What the machine needed, in an `#ifdef i286` where the PDP-11 has code
of its own:

- `battlestar` takes the PDP-11's branch, which reads its descriptions
  from `battle_strings`: in its data they would not fit.
- `rain`, `worms`, `wump` and the `hunt` driver take the PDP-11's
  branch of their random numbers: the other divided `rand()` by 2^31, or
  shifted a 16-bit `int` right by 16, which is 0.
- `hangman` chooses a word at a random place in `/usr/dict/words`, and
  takes the PDP-11's branch, which makes the place a `long` of two
  `rand()`s. The other is one, which here is at most 32767, a sixth of
  the file: every word would have come before "ceramic".
- `monop` saves a game by writing all of its data from address 0 to the
  break, and restores one by reading it back over it. On the 286 the
  stack lies between the BSS and the heap, and reading it back would
  write over the stack it is running on: the data and BSS and the heap
  are written each by itself, with where each begins, and read back by
  the same `monop` only, as `sendmail` freezes its configuration
  ([mail](#mail)).
- `atc` kept 100 scores of 528 bytes each on the stack, 52K: they are
  allocated, as many as it keeps, 18.
- `canfield` declares `random()` a `long`. Undeclared, its result was
  taken from the low word, and so half the time negative, and an index
  into the deck.
- `quiz` takes the time as a `long` and its low word for the step
  between questions, where it read the second `int` of two, the PDP-11's
  low word and the 286's high one. `adventure` seeds `rand()` so too:
  with the high word every game played in the same 18 hours went the
  same way.
- `adventure`'s `ran` declared `rand()` a `long`. The PDP-11 returns a
  `long`'s high word where an `int` is returned, and so took the number
  as that and something else as the low word, and the result was still
  positive; here the number is the low word, the high word is what DX
  held, and the result was negative half the time, which every chance
  the game takes with `pct` would pass. `rand()` is an `int` now.
- `robots` flushed what had been typed with `TIOCFLUSH` and a null
  pointer, through which the kernel reads which of the terminal's queues
  to flush. Here it reads 0, which is both, and the screen `robots` had
  just drawn was thrown away before it had all been sent: there was
  half a field and no help beside it. It flushes the input, `FREAD`, as
  its comment says and as `mille` does.

Bugs, which the PDP-11 has too:

- `arithmetic` took a NUL for the end of its input, as `getchar`
  returned in the Sixth Edition, and so asked for ever once its input
  had ended. It, `wump` and `battlestar` left with `exit()` and so with
  whatever status was on the stack.
- `canfield` and `cfscores` gave `lseek` a user's place in the file of
  scores as an `unsigned`, where it takes a `long`, and printed the
  scores, which are `long`s, with `%d`.
- `backgammon` and `teachgammon` set the terminal's speed for the
  termcap from `old.sg_ospeed`, where `old` is an `int` holding the
  terminal's flags: PCC took the member's offset, and so the flags' high
  byte. They take the speed from the terminal's modes.
- `btlgammon` read up to five moves into an array of five and ended the
  list with a sixth.
- `monop`'s resignation to the bank looked at a square's type in the
  element of the player's list of property, beyond its end, to decide
  whether to take the houses off; it looks at the square. It asked
  `stat` about a file into an array smaller than `struct stat`, and gave
  `ctime` a time where it takes the time's address, and `wait` nothing
  where it takes a pointer.
- `hunt` gave `htonl` a user's number as an `int`. It looks for a game
  by broadcasting and waiting a second for an answer, in `recvfrom`
  with an alarm, and takes the call's being interrupted for no game;
  but `signal` has an interrupted call taken up again, and with no game
  running `hunt` waited for ever and never started one. The alarm
  interrupts the call now (`siginterrupt`).
- `battlestar` buffers its input with `setbuf`, and the library, 4.4BSD's
  since patch 499, writes out what waits to be written before it reads
  only for input that is unbuffered or buffered by the line: the prompt
  did not appear until a line had been typed. `battlestar` writes it out
  before it reads.
- `adventure`'s first run reads its "y" from a pipe, and so the library
  does not write out its question before it reads: the question was still
  waiting when `save` saved the program, and every game began with "got
  the data.  save as "advent"?". Upstream's own `advent`, on its tape,
  has it so; and a game suspended with what it wrote going to a file or
  a pipe saved that too. `save` writes out what waits before it saves.
- `hack`'s Makefile made its directory and put nothing in it; without
  `perm`, its lock, `hack` will not start, and without `data`, `help`,
  `hh` and `rumors` it can tell nothing. They are installed, with an
  empty `record` and the directory `save`, as hack's `READ_ME` says.

What upstream does that is left as it is:

- `hack` says "xprncd".
- `fortune` comes in its turn to the empty string after the last fortune
  of `scene`, and of `obscene`, and prints nothing.
- `rogue` locks its scores by making `/usr/games/lib/rogue.lock`, which,
  running as `daemon`, it cannot: the directory is root's, as on
  upstream's tape. It tries five times, says "cannot lock score file"
  each time and waits for a space after the first, and goes on without
  the lock.

`tests/programs` plays those that can be played from a file, 24 checks:
`banner`, `bcd`, `ppt` and `morse` must write a message as their
pictures and sounds, `number` name a number, `factor` and `primes` give
factors and primes, and `arithmetic` refuse a range too large; `fortune`
must give the first two fortunes, which are the build's order's; `quiz`
must list its subjects from its index, `wump` take a "y" for its
instructions, and `wargames` answer; `cfscores`, `snscore`, `robots`,
`hack` and `sail` must read their empty scores and logs, and `busy` find
the system idle; `atc` must list its games and the scores it comes with;
`phantasia` must list its monsters from their file and its characters
from the one `tools/setfiles.py` made; `words` must find the words that
can be made of the letters of another; and `boggle` must find every word
of its dictionary on a board it is given, all 113. `adventure` must be
made and installed by `make install` in `/usr/games/lib/adventure`, take
a few moves and say what it scores, and suspend a game, which must start
again as a program and tell the player to wait. `look` must find words
in `/usr/dict/words`, which it searches when it is given no file. At the
terminal, `battlestar` must describe where it starts, from
`battle_strings`, and leave when told to quit; `hack` must ask whether
the player is experienced, and leave at the end of its input; `rogue`
must show its scores, after it has failed to lock them; `robots` must
draw its field with its help beside it, and `hangman` its board, and
each leave when it is interrupted. `tests/network` has `hunt` look for a
game on the Ethernet, and find none.

## The compiler

`cc` and what it runs are on the system, with `ar`, `ranlib`, `nm`,
`size` and `lorder`, `/usr/include`, and the libraries for each model:
[the compiler, the assembler and the linker](toolchain.md) has what
each is and where it is from. `cc`, `/lib/cpp`, `ar`, `ranlib`, `nm`,
`size` and `lorder` are upstream's. What they needed:

- `cc` has `-ms`, `-mm` and `-ml` for the model, tells the passes, the
  assembler and the linker which it is, takes the start-up code from
  `/lib`, `/lib/m` or `/lib/l`, and runs the assembler without the
  PDP-11's `-V` and `-u`. In the large model it gives the preprocessor
  `__LARGE__`, and `NULL` as `0L`; in the medium, `__MEDIUM__`.
- `/lib/cpp` defines `i286` and nothing else of its own, which is what
  the system's programs were compiled with; it keeps its macros in
  memory, not in a file through `libvmf` as on the PDP-11; and what
  `#if` reckons in is a `long`, as it is when the PDP-11's compiler
  compiles it, where the cross compiler would make it 64 bits and call
  for routines the library has not got. The kind of a macro, a number
  up to 9, is in a field of four bits that is now said to be unsigned.
  A field is unsigned to the PDP-11's compiler and to the system's own,
  whatever it is declared; to the cross compiler a `char` or an `int`
  of four bits is signed, 8 was -8, and a macro without arguments was
  never found again. No other field in the tree holds more than a
  signed one can. An `#if` whose value was not 0 but whose low 16 bits
  were, such as `#if 0x10000`, was false: the `long` was made an `int`.
  A file's name with its directory was copied into 256 bytes, whatever
  its length. And it reads the lines it writes to say where it is,
  `# 12 "file"`, as `#line`, which hack's Makefile needs: it compiles
  what `cc -E` wrote.
- `/lib/cpp` is of the small model, and keeps the text of its macros and
  their names in two segments it asks the system for (`segalloc`), each
  grown 4K at a time to 64K, more than which is an error; its own data
  has the table of names, its buffers and its list of the macros not to
  be expanded again. Upstream's kept all of it in that data, its table a
  patricia tree, and ran out of memory on eight of tcsh's files, the
  kernel's `machdep.c`, and `top`'s, `ntpd`'s and `rwhod`'s: tcsh's
  headers and the system's give it 1,221 macros, whose names, text and
  nodes took 57K where the small model leaves about 52K. The table is a
  hash table now, 14 bytes a name in the program's data; the worst of
  those files takes some 30K of the 47K there is. It reads a file 1K at
  a time and writes 2K, where it was 246 bytes and 256, and passes over
  comments, what `#if` leaves out and identifiers a stretch of the
  buffer at a time, where it took each character from a function; a
  `\r` in the text is kept wherever it falls, where one at the start of a
  read was left out. Given on the 286 the files that it is given where
  the system is built, with their flags, it writes what it writes
  there, to the byte.
- `size` adds up the segments: the text is those of code, the data what
  the last has in the file, and the bss the rest of it. For a program
  of the medium model it gives each segment of text as well, as it
  gives the PDP-11's overlays.
- `ranlib` reads the table of segments with the header to find an
  object's symbols.
- `nm` needed nothing more than the headers ([programs that read the
  kernel](#programs-that-read-the-kernel)), and is given `ar`'s for a
  library. It shows a symbol's segment after its name in a program, as
  it shows the PDP-11's overlay.

`tests/programs` compiles a program of two files on the system, with
and without `-O`, through a library that `ar` and `ranlib` made there,
by way of `cc -S` and the assembler, and for the medium model and the
large, and runs each. `tests/ccom/self.sh` has the system make the compiler itself, and
all that goes with it, from the sources, twice ([the system makes its
own compiler](toolchain.md#the-system-makes-its-own-compiler)).

With the compiler came what waited for it:

- `liby`, for a parser with no `main` or `yyerror` of its own, in both
  models with the other libraries; and `libl.a`, a link to `lex`'s
  `libln.a` as upstream installs it, so that `-ll` finds it.
- `mkdep`, a script that has `cc -M` write a Makefile's dependencies,
  and `lint`, which is upstream's script that says why there is no
  `lint`.
- `calendar`: a program in `/usr/libexec` that writes patterns for
  today's date and tomorrow's, and a script that gives the user's
  `calendar` file to `/lib/cpp` and `egrep`. `/usr/adm/daily` runs it
  for everyone each night, as upstream's does.
- `learn`, with the lessons of `share/learn` in `/usr/share/learn`:
  files, the editor, `vi`, more about files, the `-ms` macros, `eqn`
  and C. A lesson in C has what is typed compiled by `cc` and run, and
  two of them link it with `getline.o` or `getnum.o`, which upstream
  compiles when the lessons are installed and the build here has the
  system's compiler make (`tools/ncc`). For the cross compiler `learn`
  needed three declarations put right: a `FILE *` that was given as
  `NULL`, a signal's function that returned nothing and was not `void`,
  and its own `system`, whose argument is `const` in `stdlib.h`.

The manual has `cc`, `cpp`, `ar`, `ranlib`, `nm`, `size`, `lorder`,
`mkdep`, `calendar` and `learn` as upstream wrote them, `cc` with a
paragraph for `-ms` and `-mm`. `as(1)`, `ld(1)` and `a.out(5)` are
written for the 286 (`bin/as/i286`, `bin/ld/i286`), upstream's being
of the PDP-11's assembler, its overlays and its format: `bin/as`'s and
`bin/ld`'s Makefiles install them, `a.out(5)` with `ld` as `ar(5)` is
with `ar`, and `man`'s have not upstream's.

## The root file system

It is what upstream's Makefiles install, as 2.11BSD's are made
([the system makes itself](self-hosting.md)): first `etc`'s
`distribution` (`usr/src/etc/Makefile`), which makes the root's
directories as `usr/src/etc/dirs` lists them, with their modes, owners
and groups, then `/etc`, and the devices with `MAKEDEV all`; then what
`make build` and `make installsrc` in `/usr/src` install, and the
kernel and `/boot` as their Makefiles install them. `/etc` has
upstream's `gettytab`, `master.passwd`, `group` and `shells`; this
port's own `rc`, `rc.conf`, `netstart`, `ttys`, `fstab` and `motd`,
which are in `usr/src/etc` in place of upstream's; root's `.profile`,
`.cshrc` and `.login`, and the short `/.profile` for the single-user
shell, which are upstream's (`usr/src/etc/root`), but for the erase
character, which `.profile` and `.login` make ^H only on the console's
screens ([the console](console.md)), and root's `.exrc`, this port's
own, which `etc/root`'s Makefile installs too; and what the Makefiles
make: `syserrlst`, the terminal capabilities, and `passwd` with the
password database. As
upstream has them, the capabilities are in `/usr/share/misc/termcap`,
and `/etc/termcap` is a symbolic link to that file. `fstab` names the
disk the root is to be on: `wd0`, the first on the AT disk controller,
unless `make root ROOTDISK=sd0` makes it the first SCSI disk. The database, `passwd.dir` and `passwd.pag`, is what
`getpwnam` and the rest look users up in; without it they read
`master.passwd`, which only root may, and nobody else can tell whose a
file is. Upstream makes it and `passwd` with `mkpasswd -p
master.passwd`, as `distribution` does; where the system is built
`tools/mkpasswd.py` does, and writes the same bytes
as `mkpasswd` on the 286, which `tests/system` checks. `/var/run/utmp`
is there from the start and readable by all, as upstream installs it:
otherwise `init` makes it under the kernel's umask, 026, and only root
can run `who`. The root has a `lost+found`, as a file system `mkfs`
makes does, a `/mnt`, and a `/home` for users. `/usr/skel` has a new
user's `.profile`, `.cshrc` and `.login`, which `adduser` copies;
upstream's `adduser(8)` names the directory but the tree has nothing in
it, so these are this port's own, which `adduser`'s Makefile installs
with it (`usr/src/local/adduser`), root's without
root's prompt and paths, and with the same erase character on the
console's screens.

Upstream's `rc` is the PDP-11's, with its buses and network interfaces,
and calls some things that are not built yet: the quota programs. This
port's checks the file
systems as upstream's does. At an automatic boot `fsck -p` checks each
in `fstab` that was not unmounted cleanly, the root first and alone; if
it had to change the root, which is mounted, `rc` starts the system
again with `reboot -n`, so that nothing the kernel held of the old is
written over what `fsck` wrote. From single user `rc` checks nothing,
that having been done by hand. `mount -a` mounts the rest of `fstab`,
and `dev_mkdb` and `ps -U` make the databases of devices and of the
kernel's symbols that `ps` reads ([programs that read the
kernel](#programs-that-read-the-kernel)), and the locks that `tip`
leaves in `/usr/spool/uucp` are taken away. Then `rc` reads
`/etc/rc.conf`, which names the machine and gives the
Ethernet interface's address, netmask and gateway, and runs
`/etc/netstart`, which sets up the loopback interface and, if `rc.conf`
gives an address, the Ethernet interface and the default route, as
upstream's `rc.conf` and `netstart` do for the PDP-11's interfaces; as
shipped there is no address, to be set for the machine. Then, in
upstream's order, it starts `syslogd`; has `savecore` save the dump a
panic left, if `/etc/crash` is a directory ([crash dumps](#crash-dumps));
has `expreserve` save what `ex` was editing when the system went down;
clears `/tmp`; starts `update`,
which has the kernel write what it holds for the disks every 30
seconds, and `cron`; turns accounting on ([the log, and what the clock
runs](#the-log-and-what-the-clock-runs)); and starts `inetd`, `lpd`
([printers](#printers-and-the-rest-of-upstreams)), and the
daemons `rc.conf` asks for ([the network's
programs](#the-networks-programs)), among them `named`, which it starts
before `inetd` as upstream's does, and `sendmail`'s ([mail](#mail)),
both off as shipped. `inetd.conf`, `hosts.allow` and
`hosts.equiv` are this port's own, as is a `hosts` that names only
`localhost`; `services`, `protocols`, `hosts.deny`, `ftpusers` and
`ntp.conf` are upstream's ([the network](network.md)).

The programs: `init`, `getty`, `login`, `sh`, `csh`, `ls`, `cat`, `cp`,
`mv`, `rm`, `ln`, `mkdir`, `rmdir`, `chmod`, `chgrp`, `cmp`, `dd`, `df`,
`du`, `echo`, `ed`, `grep`, `kill`, `nice`, `od`, `pr`, `pwd`, `stty`,
`sync`, `tee`, `test` (and `[`), `time`, `who`, `wall`, `write`, `date`,
`hostname`, `hostid`, `pagesize`, `passwd`, `chpass` (and `chfn` and
`chsh`), `ps`, `sysctl`, `tcsh`, `expr`, `awk` and `make`, and in
`/usr/bin` `sleep`, `iostat`, `find`, `crontab`, `at`, `atq`, `atrm`, `egrep`,
`bc`, `dc`, `yacc` and `lex`, and in `/sbin` `mount`, `umount`,
`mknod`, `fsck`, `mkfs`, `newfs`, `mkpasswd`, `dmesg`, `reboot` and
`halt`. `/usr/ucb` has `ex`, `vi` and `view`, one file under three
names, as upstream has them, `w` and `uptime`, one file under two,
`vmstat`, `top`, `logger` and `lastcomm`. `/usr/sbin` has `update`,
`vipw`, `chown`, `dev_mkdb`, `pstat`, `tracebuf`, `syslogd`,
`cron`, `accton`, `sa`, `zic`, `zdump`, `exrecover` and `expreserve`,
and `/usr/libexec` `atrun` and `acctd`, and in `find` `updatedb` and
the `bigram` and `code` it runs. `/usr/adm` has `daily`, `weekly` and
`monthly` ([the log, and what the clock
runs](#the-log-and-what-the-clock-runs)). `/usr/local` has the port's
own, `adduser`, `bench` and `ve`. For the network there are `ifconfig`, `ping` and `route` in
`/sbin`, `inetd` and `arp` in `/usr/sbin`, `telnetd` in `/usr/libexec`,
and `telnet` and `netstat` in `/usr/ucb`. The Makefiles make these
other names as links, and also `[` and `halt`, and install everything
as root's unless they say otherwise, as they do for `at`, `atq`,
`atrm` and their queue, which are `daemon`'s, and for the programs that
run with the group `kmem`. The programs that work on text and files are
listed [above](#programs-that-work-on-text-and-files), the
network's others [after them](#the-networks-programs), and mail's
[after those](#mail), with mail's files: `/etc/sendmail.cf`,
`/etc/aliases` and its database, an empty `/etc/sendmail.fc` and
`/var/log/sendmail.st`, `/etc/Mail.rc`, the help of `sendmail` and
`Mail` in `/usr/share/misc`, and the directories `/usr/spool/mail` and
`/usr/msgs`. `nroff`'s macros, its printers' tables and the manual are
[after those](#nroff-and-the-manual), and the zone files of
`/usr/share/zoneinfo` ([time zones](#time-zones)). The programs for disks and file
systems are [after the manual](#disks-and-file-systems), those for
backups and the rest of the administrator's [after
those](#backups-and-the-rest-of-the-administrators), and the games
[after those](#games).

The programs are in it without their symbol tables, as upstream's
`install -s` leaves them. `obj/nbuild/syms`, or `obj/build/syms` for
the cross compiler's, has each as it was linked, with its symbols, by
the name it is installed by, which is where to look up the address a
program faulted at: where the system is built `install -s` keeps that
copy before it strips the root's (`tools/build/install.py`). The kernel loads neither the symbols nor their
names (`kern_exec.c` reads the header, the table of segments and the
segments), so a stripped program takes the same memory and starts the
same; what it saves is an eighth of the file. `make root STRIP=no`
installs the programs as they are, for when the symbols are wanted on
the system itself. `/unix` keeps its symbols always: `ps` and the
rest look them up there ([programs that read the
kernel](#programs-that-read-the-kernel)). The system's own `strip` makes
of a file what `tools/strip.py` does, which `tests/programs` checks on
the kernel.

The file system is 24000 blocks of 1K, of which the system takes 16334.
The programs are 6.3 MB of that, and 7.4 MB with their symbols; the
manual's pages are 2.5 MB in about 2750 blocks; the games and their
files take about 3300 blocks, 1600 of them `fortune`'s;
`/usr/include` and the libraries take about 1080; `learn`'s lessons
830; and `/usr/dict/words` takes 206. It was 8000 blocks until the manual came, 12000 until the
games did, 15000 until the compiler, and 16400, which was all that the
tests' disks of 20 MB had room for, until `learn`'s lessons came: what
was left free was then less than the system writes in a night, the
accounts that `/usr/adm/daily` keeps being 832 blocks by themselves
(`tests/programs`). The tests' disks are of 40 MB now. A machine's
own disk is made with as many as it has room for (`make root
ROOTSIZE=...`, or `tools/mkfs.py -s`).

## adb

`adb` is upstream's, with what it knew of the PDP-11 written again for
the 286: the disassembler (`opset.c`), which prints the 286's and the
287's instructions in `as`'s syntax, NASM's, and was compared with
`ndisasm` over every opcode with each form of operand; the registers,
from the trap frame at the top of the process's user structure
(`machine/frame.h`), and the 287's, which `$f` prints; breakpoints, an
INT 3 in place of an instruction's first byte; the stack, which `$c`
follows by BP, the frame every function of the system's compiler keeps,
with far returns in the medium and large models, and the cross
compiler's frames too, whose functions may save SI and DI before BP,
and whose static functions are called near and have no symbols: a
frame is taken to be where a call returns to, by the instruction
before it, and when its function's code does not say how many words it
saved, the function the call is to does;
and the maps of a program, a core file and the kernel, which are of
segments. Overlays, and `$a`, ALGOL 68's backtrace, went with the
PDP-11.

An address is as a far pointer has it, the selector in its high word and
the offset in its low. One whose selector is 0 is in the segment its
command takes by default, the code segment for `?` and the data segment
for `/`, so in a small-model program a 16-bit address means what it did
on the PDP-11 with separate I and D; a symbol's value carries its
segment. Numbers are hexadecimal, read and printed, where upstream's
were octal: `$o` and `$d` make what is typed octal or decimal, and `$x`
hexadecimal again. `P` prints a far pointer by its symbol. `*`, the word
at an address, keeps that address's segment when it is not the default
one, as a near pointer kept in the network's data segment points into it
([the network](network.md)): `*_ifnet$<ifnet` prints the first
interface. A selector's low two bits, its privilege level, do not change
its segment; the kernel marks the network's functions among the
callouts with one (`SUPERFUN`).

A core file is the user structure, the process's table of segments, and
each segment of its own (`core(5)`), and is named `prog.core`, as
upstream's kernel names it; upstream's `adb` still looks for `core`, and
so does this one: `adb prog prog.core`. `adb -k /unix /dev/kmem` reads
the running kernel, whose segment N is at N × 64K in `/dev/kmem`. The
scripts in `/usr/share/adb` print the kernel's structures as the 286's
compiler lays them out: `*_uptr$<u` the running process's user
structure, `_proc$<proc` the first slot of the process table, `$<callout`
the clock's callouts, `*_mfreelist$<mbuf.nxt` the free mbufs.
`$c` follows a stack in the kernel from a frame given it:
`*(_uslot+118)$c` is the swapper's, from the BP in its `u_rsave`, the
first slot of user structures being the swapper's alone
(`machine/machdep.c`). `adb -k` reads a crash dump as well, and `$c`
there starts where the dump was taken ([crash dumps](#crash-dumps)).
`callout.next` ends its list with `#` as 4.3BSD's `adb` has it, logical
negation, which 2.11BSD's had not, so it printed only the first callout;
`#` before anything but a hexadecimal digit is now negation.

The kernel's side: `ptrace` takes the address in the traced process as a
far pointer, whatever the caller's model (`syscalls.tab`), and has the
287 save its registers in the user structure (`fpsync`) before it reads
or writes them there, as a core dump has.

## Crash dumps

Upstream's `savecore` saves the dump a panic leaves in the swap area
([disks](disks.md#crash-dumps)), and the kernel that took it, when
`/etc/rc` runs it at the next start, which it does if `/etc/crash` is a
directory or a link to one, as upstream's does:

```sh
mkdir /usr/crash
ln -s /usr/crash /etc/crash
```

It copies `/unix` to `unix.N` and the dump to `core.N`, N being the
number in `bounds` there, which it then makes one more; writes in
`/usr/adm/shutdownlog`, which `etc`'s Makefile installs as upstream's
does, when the system started again and after what panic; and clears
the time in the dump, so that the next start does not save it again.
A dump is as big as the memory, the hole from 640K to 1M with it: 4.4
MB on a machine of 4 MB. If there is a number in `minfree`, it saves
nothing when the file system has fewer kilobytes free than that.
Upstream's found what it reads in the dump, the time, the panic's
message and the kernel's version, at the addresses of their symbols,
the PDP-11's kernel's data being at the start of memory; this one goes
by the kernel's segments, whose places the global descriptor table
saved in the dump has, and takes the dump's length from the dump's
header (`machine/seg.h`), where upstream's took the size of the memory.

```
adb -k /usr/crash/unix.0 /usr/crash/core.0
*_panicstr/s	the panic's message
$c		the stack, from where the dump was taken: _dumpsys, _boot,
		_panic and what called it, back to the system call
*_uptr$<u	the user structure of the process that was running
```

`ps` and `pstat` read a dump with `-k`, as upstream's do: `ps -alx -k
/usr/crash/core.0:/usr/crash/unix.0` lists the processes there were,
and `pstat -p -k /usr/crash/unix.0:/usr/crash/core.0` the process
table. On the PDP-11 a dump has the kernel's data where `/dev/kmem` has
it, at its start; here `adb`, `ps` and `pstat` know a dump by its
header and read each of the kernel's segments where the descriptors
saved in the dump say. `crash(8)` is upstream's with the PDP-11's
traps, registers and console switches written again for the 286.

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
RAM disk. The program is built for each model; the small-model one is
`init`, and when it is done it runs the medium-model one, whose code is
packed into 8K segments so that it has four of them, and then the
large-model one, with ten checks more ([the large
model](#the-large-model)). Each checks, 74 times: printf and scanf
with integers, strings
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
`syslogd`, clear `/tmp`, and start `update`, `cron`, accounting,
`inetd` and `lpd`, which the console's screen shows. The system goes
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
each of the 162 checks in `tests/programs/checks` it runs the check's
lines with the shell in an empty directory and compares what they wrote
with what the check says they must, and says `ok` or shows what was
written. A check is of one program, by its name, or of one use of it:
`sort` has one for its options and one that sorts 6000 lines, more than
it holds in memory, so that it merges files; `diff` has four, among
them one of directories and one against an empty file; `nroff` formats
three of the manual's pages and must make of each what the build did
([nroff and the manual](#nroff-and-the-manual)); `zic` must make the
zone files the build made ([time zones](#time-zones)); `daily`,
`weekly` and `monthly` are run, and must move the logs along and make
`whatis` and `find`'s list; `disklabel` and
the programs that read a file system directly work on a second disk
([disks and file systems](#disks-and-file-systems)), and `dump` and
`restor` copy a file system there to a file and back to another
([backups](#backups-and-the-rest-of-the-administrators)); and the games
that can be played from a file are played ([games](#games)). The test
fails unless all 157 say `ok`. Then, at the terminal, it runs `tty` and `mesg`;
pages through a file and through a pipe with `more`, and through `ls`'s
page with `man`; records a shell
with `script`, which it leaves with ^D, and finds the command it typed
in the file; locks the terminal with `lock`, gives it a wrong key and
then the right one; has `tset` name the terminal and `clear` write the
sequence that clears a VT100; starts five games and leaves each
([games](#games)); runs a command in a window of `window`'s and leaves
it; and stops the system with `shutdown -h now` and a
message, which must be shown before the system halts. Afterwards
`tools/fsck.py` must find nothing wrong with the root.

`tests/network` boots it with the Ethernet card, logs in as root on
the serial port, adds the users `guest` and `ftp`, and runs the 37
checks of `tests/network/checks` in the same way. The first names the
machine, brings its interface up at the address the emulator's network
expects, and has `inetd` read its table again with `rexecd`, `tftpd`,
`ntalkd` and `identd` turned on. The rest use each program against its
daemon on the machine itself: `finger` through `fingerd`; `tcpdmatch`
and `tcpdchk` on the rules, `try-from` run by `rsh` to say what `tcpd`
knows of a client, its user among it, which `identd` tells, and an
`ftp` that `tcpd` refuses and logs once the rules leave `ftpd` out;
`rsh` with a command's output, its errors, its input, and a user who
has no `.rhosts`; `rcp` each way and of a directory;
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
with its trace; `hunt -q`, which must find no game, having broadcast
for one on the Ethernet and waited ([games](#games)); `named`,
`identd` and `rdist` ([above](#named-identd-and-rdist)); and the 12 of
mail ([mail](#mail)). Then root uses
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

- `window` is the only large-model program. A program moved to it
  wants looking over first (`tools/lmlint.py`), and so does each
  library it uses but the C library, which has been. One that reads
  the kernel's structures with pointers in them, as `top`, `ps` and
  `w` do, cannot be moved: the kernel's pointers are two bytes.
- `csh`, `ex`, `tcsh`, `sendmail`, `jove`, `hack`, `phantasia`,
  `rogue` and `warp` are the medium-model programs so far. The others
  that the PDP-11 overlays (`uucp`, `kermit`, `rn` and the rest [not
  ported](unported.md)) are the ones meant for it. `csh`'s file name
  completion, and `tcsh` as a login shell, have not been tried.
- `ex`'s saving of what it was editing when its terminal hangs up has
  not been tried, only its `preserve` command, which runs the same
  `expreserve`.
- Many programs are not built yet. [What is not ported](unported.md)
  lists them, with what each waits on.
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
  writes files and a diskette (`tests/floppy`), not a tape.
- `lex -r`, for Ratfor, `awk`'s
  saving and restoring of itself (`-S` and `-R`), and `make` with
  archives, with its built-in rules or with anything but the simplest
  description file, have not been tried. Of the tree's other grammars,
  `eqn`, `rdist`, `struct` and `named`'s tools wait for their programs.
- Of the games, those played from a file are given a move or two, and
  five are started at a terminal and left; none is played through.
  `hunt` has not had a player, `sail` two ships, or `monop` a game
  saved and restored.
- `/usr/adm/daily`, `weekly` and `monthly` have been run by hand, not
  by `cron` at their times, and `zic -l` and `TZ` tried only with the
  zones' names. `acctd` stops when the file system is nearly full;
  `syslogd` does not, and the logs are trimmed only as often as the
  scripts run.
- `iostat`'s seek times (`msps`) are 0. An IDE or SCSI drive seeks by
  itself, so its driver counts no seeks, as upstream's MSCP driver
  counts none; upstream's drivers count them where they give the drive
  a seek to do.
- `newfs -i 65536`, which its manual allows, gives `mkfs` `-i 0`:
  65536 does not fit in an `unsigned int` of 16 bits. `mkfs` prints its
  usage and makes nothing. Upstream's, and not put right.
- `fsck` keeps its tables in what is left of 56K of data, as on the
  PDP-11, an eighth of a byte for each block and two and a quarter for
  each inode: room for a file system of 24 MB with an inode for each
  4K, `newfs`'s default, or one of 48 MB with 6000 inodes and not with
  6848. A bigger one needs a scratch file, `fsck -t`, which `/etc/rc`
  names on the root, and so the root cannot be one.
- A program that overflows its stack runs into its data without a fault
  ([processes](processes.md)).
- With every call printed (`sctrace=2`), the kernel's printing can stall
  the serial port's interrupt-driven output, and the terminal stops.
- Most of it has run in the emulator only. The system has also booted
  on a real machine and been logged in to over the network, and made
  diskettes there (`flformat`, `newfs`, `fsck`).
