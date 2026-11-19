# Performance

How the system's speed is measured, where its time is found to go, what
the first measurements were, and what has been done about what they
showed.

## What upstream has

No benchmark. For watching a running system it has `time`, `vmstat`,
`iostat`, `pstat`, `netstat`, `ps`, `w` and the accounting (`sa`,
`lastcomm`), which were built already ([the C library and
programs](userland.md)), and three more that are built now:

- `top` shows the processes that are using the processor, and the
  totals above them.
- `tracebuf` prints the kernel's counts of what its buffers saved the
  disk: blocks found there and not, written, read ahead.
- `sysctl` prints and sets what the kernel offers by that call.

The kernel has an option to profile itself, `PROFILE` in
`sys/conf/GENERIC`. It is the PDP-11's alone: it wants a second,
programmable clock (the KW11-P) that interrupts above everything else,
and keeps its counts in the supervisor's address space. Upstream's own
comment says it does not work and will not be mended, and the tree has
no program that reads the counts out. The port has nothing of it.

The kernel's counters are in: it is built with `UCB_METER`, which is
what `vmstat`, `iostat`, `top` and `tracebuf` read.

A program can be profiled as far as the kernel goes: `profil` works.
The C library's side of it (`mcrt0`, `monitor`, `mcount`) is PDP-11
assembly and is not ported, so `prof` is not built.

## bench

`bench` is this port's own (`usr/src/i286/bench`, installed as
`/usr/bin/bench`). Each test does a fixed amount of one kind of work
and prints how long it took by the system's clock, how much of that
was the processor's in the program and its children and how much in
the kernel for them, how many pieces of work that was, and what one
comes to.

| test | the work | counted in |
|---|---|---|
| `sieve` | the primes below 16384 by the sieve BYTE printed in 1981: the processor, in the program's own code | µs a pass |
| `long` | a 32-bit multiplication, division and remainder, which the compiler's library does | µs a round |
| `getpid` | the system call that does least: the way into the kernel and out | µs |
| `write` | a byte written to `/dev/null`: a system call that goes through a file | µs |
| `stat` | `stat` of `/usr/libexec/telnetd`: a name looked up, three directories deep | µs |
| `pipe` | a byte to another process by one pipe and back by another: two switches between processes | µs a round |
| `pipeio` | data through a pipe to another process, 4K at a time | K/s |
| `fork` | `fork`, `exit` and `wait` | µs |
| `exec` | `fork`, then `exec` of a program that does nothing, `exit` and `wait` | µs |
| `shell` | `sh -c "ls /bin \| wc -l"`: a command as a user types one | µs |
| `creat` | a file made in `/tmp` and removed | µs |
| `fswrite` | a file written 4K at a time, until the disk has it | K/s |
| `fsread` | a file read from one end to the other, just after it was written | K/s |
| `pty` | lines written to a pseudo-terminal and read from its other side, as the telnet server reads what a user's program writes | K/s |
| `tcp` | data over TCP to a process on the same machine | K/s |
| `tcprr` | a byte over TCP to a process on the same machine and one back: a key and its echo | µs a round |
| `telkey` | 4 users logged in by telnet at once, each typing lines a key at a time and waiting for each echo: TCP, `telnetd`, the pseudo-terminal and back | µs a key |
| `telcmd` | the same 4 users each running `cat` of a screenful, 24 lines of 80: the shell, a program, and its output through the terminal, `telnetd` and TCP | µs a command |

`bench` with no arguments runs them all; `bench stat pipe` runs those;
`-s N` does N times the work, for a machine too quick for a clock that
ticks 60 times a second. The work is the same on every run, so two runs
differ only as the system or the machine does. On a real machine it
needs nothing but itself.

The last two are users on the network. Each user is a process of
`bench`'s that connects to the machine's own telnet server, logs in,
and types as fast as the system answers; so `inetd` must be running,
the users cost the machine what their own machines would have done, and
nothing goes through a network interface. `-u N` makes it N users, 1 to
8, where it is 4; `-l NAME` logs them in as NAME where it is root: an
account with no password, whose shell is `sh`.

`-m` is for a harness: the test writes `@@` as its work begins and
again as it ends, after waiting until the terminal has sent everything
written before, so that each mark leaves the machine as it is written.

## tests/bench

```sh
. tools/env.sh
tests/bench/run.sh                      # everything
tests/bench/run.sh pipe pty             # those tests
MS=1 tests/bench/run.sh tcp             # with a sample every millisecond
BASE=before.txt tests/bench/run.sh      # compared with an earlier run
```

It builds the system, boots it in the emulator to multi-user, logs in
as root on the serial port and runs `bench -m`. The harness
(`tests/harness/runtest.py`) does two things meanwhile, both new:

- `--mark TEXT` notes the emulated time, to the millisecond, each time
  the machine writes TEXT. So each test is timed twice, by the system's
  clock and by the emulator's, and `tests/bench/report.py` fails the
  run if the two differ by more than a twentieth and 50 ms. A kernel
  that kept interrupts off long enough to lose clock ticks would be
  caught there, with every test looking quicker than it is.
- `--profile KERNEL[:MS]` takes a sample of where the processor is
  every MS milliseconds on average (below).

What comes out, in `obj/tests/bench`: `results.txt`, the table, which
is also printed; `kprof.txt`, where each test's time went; and
`profile.txt` and `marks.txt`, the samples and the times of the marks,
from which `tools/kprof.py` can make other reports.

To see what a change did, copy `results.txt` aside before it and give
the copy as `BASE` after: the table gains what each test came to then
and the change in percent, positive for quicker.

A run takes the host about two and a half minutes. It also tries
`tracebuf`, `sysctl` and `top` and a `bench` with tests named and the
work doubled, and fails if any of those does not do what it should;
it does not fail for a test being slow.

## Where the time goes: kprof

Every few milliseconds of emulated time the harness stops the machine
and asks the emulator for the processor's registers, and reads the name
of the running program out of the kernel's memory (`u.u_comm`; the
kernel has a variable, `ucomm`, that says where in a user structure
that is, and nothing else for this). `tools/kprof.py` looks each place
up: in the kernel's symbol table if the processor was in ring 0, and
in the program's, from `obj/bin`, if it was in a process. The report
says how the time divides between processes, the kernel and the idle
loop, and lists the functions that had most of it:

```
tcp: 435 samples in 2.128 s
  system 100.0%
  bench            user   0.0%  system  99.5%
  (no process)     user   0.0%  system   0.5%
   68.5%    298  kernel: in_cksum
    5.1%     22  kernel: tcp_input
    4.8%     21  kernel: copy
    4.1%     18  kernel: tcp_output
```

The machine is stopped while it is looked at, so sampling costs the
system no time and changes nothing it does, and a sample can fall
anywhere: in an interrupt handler, or where interrupts are off. It
needs the emulator, 86Box with its automation socket; nothing of it
runs on a real machine.

Its limits:

- A function is the nearest symbol at or below the place, and the
  tables have only names that are not static. A static function's time
  goes to the function before it, and the kernel has some. One
  matters: `trap` in a report includes `userret`, the way back to a
  process from every trap and system call, which follows it in
  `sys/i286/trap.c`. It was tried global and put back: a system call
  took 1% longer, which is what a far call in place of a near one
  would cost. The kernel's assembly
  had labels with no symbol at the busiest places, where traps and
  interrupts enter and where `copyin`, `copyout` and `pcopy`, which
  copies between any two places in memory, share a loop; those now have
  symbols (`vec0`, `allsys`, `alltraps`, `allintr`, `raise`, `lower`,
  `runpend`, `copy`, `str`, `window`), so `copy` in a report is that
  loop. `bench`'s functions are not static for the same reason.
- A request to the emulator takes over a millisecond of the host's
  time, and a sample is three. At 5 ms between samples a run goes at
  about the speed of the real machine, and at 1 ms four times slower.
- A sample can only be taken a whole number of milliseconds after the
  machine started, and the clock interrupts every 16.6665 ms. So a
  sample falls at one of 50 distances from the last tick, a third of a
  millisecond apart, and they shift by a whole third only in half a
  minute. What the system does at every tick in less time than that,
  the clock's own handler above all, is seen too often or too seldom
  in a part shorter than that. Samples at even intervals were far
  worse, 5 ms apart at one of 10 distances: an idle system showed 10%
  of its time in the clock interrupt. So they are taken at uneven
  ones, 1 to 2 MS - 1 milliseconds apart, by a generator of random
  numbers that starts the same each run.
- A program is known by its name: two with the same name are one.

## The first numbers

86Box's IBM PS/2 Model 30 286, a 10 MHz 286 with a 287, 4 MB, an IDE
disk; the system in multi-user with its daemons, nobody else logged in.
`outside` is the emulator's clock, in seconds.

```
test        real    user     sys   count      each      outside
sieve       3.46    3.46    0.00      40     86675 us     3.466
long        3.00    3.00    0.00   20000       150 us     3.005
getpid      3.10    0.26    2.85   20000       155 us     3.109
write       3.08    0.23    2.85   10000       308 us     3.086
stat        2.21    0.05    2.16    2000      1108 us     2.217
pipe        3.76    0.03    1.81     500      7534 us     3.763
pipeio      2.81    0.01    2.78    2048       727 K/s    2.807
fork        3.50    0.00    3.26     300     11666 us     3.489
exec        2.71    0.05    2.55     100     27160 us     2.713
shell       2.50    1.23    1.11      10    250000 us     2.489
creat       2.25    0.03    0.93     100     22500 us     2.253
fswrite     3.16    0.00    2.98    1024       323 K/s    3.166
fsread      2.71    0.01    2.68    1024       377 K/s    2.718
pty         7.95    0.10    0.03      16         2 K/s    7.945
tcp         2.13    0.00    2.13     256       120 K/s    2.128
tcprr       3.28    0.03    1.48     500      6566 us     3.272
```

These are the emulator's notion of a 286: how near its count of each
instruction's clocks, and its disk, come to the real machine's has not
been checked. They are for comparing one build of the system with
another, and `bench` on the real machine will say what they are worth
as figures.

Three runs gave each test the same time to within 10 ms, most to
within 2, but for `fork` and `shell`, which varied by 2 and 3%, and
`pty`, by half a percent. The emulated machine does the same thing each run but for its
clock chip, which has the host's time of day, and that moves what
`cron` and `update` do.

## What the first profile shows

What the samples say, test by test. The causes are from reading the
code where one is given, and none has been tried by changing anything
yet.

- **`pty`: 2 K/s, with the machine idle 94% of the time.** A
  pseudo-terminal's output queue may hold 2000 characters before the
  writer is made to wait (`tthiwat`, at the speed a pseudo-terminal
  has), and the kernel has 68 blocks of 30 characters for every
  terminal's queues together (`NCLIST`, `sys/PC/param.c`): 2040. So the
  writer runs out of blocks before it reaches the mark, and what
  `ttwrite` does then is sleep until the next second begins
  (`tty.c`, on `lbolt`). The reader empties the queue at once and
  nothing wakes the writer. 2 K/s is one fill of all the blocks a
  second. This is what a user on telnet gets from any program that
  writes more than 2K at once.
- **`tcp`: 68% of the time is the checksum**, `in_cksum`, which is
  plain C here (`sys/i286/in_cksum.c`). Upstream's comment on it asks
  for one written for each processor.
- **`getpid`: 155 µs for a system call that does nothing**, 1550 of
  the processor's clocks, of which the trap into the kernel and the
  return are 133 ([memory model](memory-model.md)). `syscall` itself has 33% of the samples, `userret`, the
  way back to the process, 16% (under `trap`'s name), and `setpri`
  6%.
- **`pipe`: 7.5 ms a round, 21% of it reading the disk** (`pinsw`, the
  disk driver's read of a sector). A pipe is a file here, as in
  2.11BSD, but a byte written and read back should not leave the
  buffers. The same test run alone, after the others, took 3.1 ms a
  round and read almost nothing.
- **`fork`: 11.7 ms, half of it in `copy`**, the copying of the
  parent's segments; and `exec` 27 ms, with `copy` 25% and `pclear`,
  which zeroes memory, 20%.
- **`stat`: 1.1 ms, half of it in `namei`.**
- **`shell`**: `ls` has more of the processor than the shell and the
  kernel's work for it together, and most of that is in `printf`
  (`svfprintf`) and `qsort`.
- `long`: 61% in the library's signed division (`I4D`).

## What was done about it

Each of these is a commit of its own, with its numbers. `was` is the
table above; the work of each test is unchanged.

```
test        each         was     change
sieve      86650 us    86675
long          83 us      150     +81%
getpid       130 us      155     +19%
write        283 us      308      +9%
stat         975 us     1108     +14%
pipe        2766 us     7500    +171%
pipeio       792 K/s     727      +9%
fork       11276 us    11390      +1%
exec       26170 us    27160      +4%
shell     245000 us   243400      -1%
creat      21660 us    22500      +4%
fswrite      341 K/s     321      +6%
fsread       415 K/s     379      +9%
pty           56 K/s       2     28 times
tcp          301 K/s     120    +151%
tcprr       5300 us     6566     +24%
```

- **`pty`.** A writer that finds no character blocks waits for its own
  terminal's output to go, and is woken when it has, where it slept
  until the next second (`ttyclwait`, `sys/tty.c`). And no terminal's
  output may hold more than a quarter of the blocks (`cinit`): with
  the mark a pseudo-terminal had, one program writing to one took them
  all, a second program's output waited until the first had finished,
  and there were none for what anyone typed. Two programs writing at
  once now pass 29 and 27 K/s. The mark costs one writer a tenth of
  what it would pass alone, 60 K/s, because it and its reader take
  turns four times as often. (There are blocks enough for the mark a
  pseudo-terminal had now, and one writer passes 59 K/s: see the users
  on telnet, below.)
- **`tcp`.** The checksum is assembly (`i286/mch_cksum.s`), with the C
  beside it as the reference; the two were run together on every
  packet of the network's tests.
- **`pipe`.** A pipe written at the start of a block takes a buffer
  without reading the block from the disk first (`rwip`).
- **System calls.** Three of the far calls `syscall` made for every
  call are gone (`i286/trap.c`): the copy of the argument registers,
  `setpri`, and an `spl0` that most often had nothing to do.
- **The spl routines** are in line (`machparam.h`), in the network's
  code as in the kernel's. They were 12% of `tcprr` and 9% of `pipe`:
  a far call and return for four instructions, 334 times in the
  kernel's text.
- **`stat`.** `namei`'s loop over the characters of a component, a
  quarter of the test, and `scanc`, which `ttwrite` passes a
  terminal's output through, are assembly (`mch_xxx.s`).
- **`long`.** The compiler's routine for dividing longs does a divisor
  over 16 bits with one hardware divide and a correction
  (`libc/i286/crt/i4d.s`).

What the measuring found that was not about speed: removing a file
with a double indirect block left everything under that block
allocated, when the scratch buffer the kernel took for it was the
first in the buffers' segment (`indirtrunc`; a far pointer compared
with the entry before the first). `tools/fsck.py`, run on the disk
after every `bench`, reported it.

What was tried and did nothing: compiling the kernel for speed in
place of size (`-ot`) moved every test by less than 2%, most by
nothing. The compiler's `-ox`, which `cc286 -Ot` gives, does not
include `-ot`.

A comparison is only good between runs of the same tests: `exec`,
`shell`, `creat` and the file tests go by where the disk's blocks and
the buffers were left, and `update`'s sync every 30 seconds falls in
one test or another. In a run of four of the tests `exec` took 32 ms
where the whole table has 27.

## Users on telnet

`telkey` and `telcmd` came after the rest, to see what several users
at once cost. The first numbers:

| | 1 user | 4 users |
|---|---|---|
| `telkey`, a key and its echo | 10.9 ms | 11.5 ms |
| `telcmd`, `cat` of a screenful | 198 ms | 165 ms |

A key costs the same however many are typing: the slots that processes
share (`NSLOT`) do not show. Of a key's time `telnetd` has 65%, nearly
all of it in the kernel for it, and `bench`'s user 24%, which a real
user's own machine would have. `telnetd`'s loop looks for input from
one side only when it has nothing waiting to go out the other, so by
its code a key and its echo are four calls of `select` with something
to do and one that waits, and `selscan` is the first function in the
profile, 7%. It now sends what it has without asking `select` first,
since neither side blocks, which makes that two calls and one that
waits: a key is 9.8 ms and a command 154 ms with four users, 17% and 7%
quicker. A command is quicker with four users than with one because
one user's waits are another's turn. Of its time `telnetd` has 37%,
15% in its own loop over each byte, the shell 26% and `cat` 17%.

Two things it found that are not speed. Four users logging in at the
same moment, one of them often got no `login:` and waited for ever:
`telnetd` set the pseudo-terminal's modes after it had started `login`,
which on that side of a pseudo-terminal throws away what is waiting to
be read, and on a busy machine that was the prompt. It sets them before
now ([network](network.md)). And six users by the loopback ran the
network out of memory: of its 170 mbufs none were left, 48 requests
were refused, and connections ended. Each connection has both its ends
on the machine there, so six are twelve. The pool was the size the
PDP-11 had room for, in a segment that here was 39K of 64K; it is 256
mbufs and 20 clusters now, six users' commands refuse none, and the
tests with four users and TCP's are as they were.

With memory enough, six users still failed, three times of three: one
of them typed a command that never came to its shell. What is typed
when no character block is free is lost, without a word (`ttyinput`),
and at a quarter of the 68 blocks for each terminal's output six users
running `cat` held them all. At an eighth six users passed three times
of three and eight once. It costs what the quarter did again: one writer to a
pseudo-terminal passes 47 to 50 K/s where it passed 53, and a command
with four users is 164 ms where it was 154. Eight terminals at an eighth can
still hold every block, so this made it rare and did not end it.

More blocks did. The kernel's data segment had 1.8K to spare, so they
are in a segment of their own now, 1024 of them where there were 68
(`NCLIST`, `sys/PC/param.c`): enough for every pseudo-terminal, screen
and serial line to have its output at its mark and all it may hold of
what was typed, at once, so no terminal can take what another needs
and the eighth binds nothing. The PDP-11 kernel could do the same
(`UCB_CLIST`), mapping the blocks in for each of the routines that
touch them (`sys/tty_subr.c`); here a pointer to a block, or to a
character in one, is an offset in that segment (`CLSPACE`,
`i286/machparam.h`), and the compiler loads ES once in each routine.
With the 68 blocks moved and nothing else changed, no test moved: a
key and a command with four users were each 0.3% slower, which is
less than two runs differ by. With 1024:

| | 68, an eighth each | 1024 |
|---|---|---|
| `pty`, one writer | 47 K/s | 59 K/s |
| `telkey`, four users | 9.8 ms | 9.7 ms |
| `telcmd`, four users | 164 ms | 135 ms |
| `telcmd`, six users | 167 ms | 136 ms |
| `telcmd`, eight users | | 143 ms |

A command is quicker than it was with a quarter each, 154 ms: the
screenful is 1920 characters, under the 2000 of a pseudo-terminal's
mark, so `cat` writes it and is gone, where it waited for `telnetd` to
take each part. It costs 32K of memory, and gives back the 2.2K of the
kernel's data that the 68 had.

## What is left

In the order the profile gives them.

- **Far calls.** A call and return between two of the kernel's
  functions is about 50 clocks before anything is done, and a 1 ms
  profile of `write` or of TCP shows no one place: the time is spread
  over every function on the way. Short routines called from
  everywhere are the ones to put in line, as the spl routines were.
- **Shifts of longs.** The compiler shifts a long right by a constant
  with a loop, a bit at a time: about 130 clocks for the 10 places of
  `lblkno`, in `rwip`, `bmap`, `namei` and the disk drivers. A routine
  in line that moves bytes first would be a fifth of that.
- **A pipe is a file.** A byte through one costs `rwip`, `bmap` and a
  buffer each way, and that is most of `pipe`'s 2.8 ms now.
- **`fork` and `exec`** are the copying and clearing of memory they
  were: `copy` is half of `fork`, and `rep movsw` already.
- **`ls`** is still most of `shell`: `printf` and `qsort` in the C
  library, which nothing here has looked into.
- **A switch between processes** goes by way of the swapper, as
  upstream's does, and by the samples in `pipe` is some 250 us.

