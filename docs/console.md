# The console and the terminals

The system has four terminals on the display and keyboard, which
Alt-F1 to Alt-F4 choose between, and the two serial ports. All of it
runs in the emulator under `tests/system` and `tests/vt`.

| device | major, minor | what |
|--------|--------------|------|
| `/dev/console` | 0, 0 | the first screen |
| `/dev/ttyv1` to `ttyv3` | 0, 1 to 3 | the other three |
| `/dev/tty00`, `tty01` | 10, 0 and 1 | COM1 and COM2 |

The screens are minors of the console's driver, as on the PDP-11 the
console's driver also has the extra DL-11s (`ttyl1` to `ttyl7` in
upstream's `MAKEDEV`). `/etc/ttys` runs `getty` on the four screens and
on `tty00`, with `TERM` `pccons` on the screens and `vt100` on the port;
`tty01` is off.

The kernel prints its messages on the first screen and on COM1, without
interrupts, so that they get out whatever state the kernel is in. A
panic brings the first screen to the display.

A machine with no display finds no display memory and has no screens:
opening one fails with `ENXIO`. It is used through a serial port. Making
`/dev/console` that port's node (`mknod /dev/console c 10 0`) makes the
port the console, where `init` puts the single-user shell and `/etc/rc`
writes; turn `tty00` off in `/etc/ttys` then, or two `getty`s will
share the port. `tests/kernel` and `tests/libc` make their
`/dev/console` so, because the harness reads what a system writes on
COM1.

## The screens

`usr/src/sys/i286/cons.c`. The display is a VGA, colour or monochrome,
which the BIOS leaves in its 80 by 25 text mode; the kernel never calls
the BIOS, and does not change the mode. Its text memory, 32K at B8000
(B0000 for monochrome), has room for eight screens of 4,000 bytes, and
each terminal gets one. Choosing which is shown is one write to the CRT
controller's start address: nothing is copied, and a screen keeps what
is on it while another is shown. One segment descriptor of the kernel's
covers all of them. The first screen keeps what the BIOS and the boot
programs wrote, and the kernel goes on below it; the others start
clear.

A screen draws what is written to it as soon as it is written. The
start routine, which the tty layer calls when there is output, empties
the output queue into the terminal emulator 64 characters at a time,
at the level it was called at: in a process writing, that is the
process's own, so no interrupt waits on the drawing. When the call
comes from an interrupt, as echoing what is typed does, it is short.
`TS_BUSY` keeps a second call for the same screen out while one is
drawing; what it would have drawn is in the queue, and the first call
draws it.

The hardware cursor follows the shown screen's. The bell is the
speaker, driven by channel 2 of the timer, for an eighth of a second.

## The terminal

`usr/src/sys/i286at/vt.c` draws on a screen what a DEC VT102 would,
taking the VT102's user guide as the reference and DEC's later manuals
where it leaves a case open. Linux's console was the checklist of what
a PC console must get right; the code is written from DEC's manuals.
It touches no hardware: it is given a screen's memory and what is
written to it, and calls back to ring the bell and to answer.

What it has of the VT102: cursor movement and addressing, origin mode
and the scrolling region, index, reverse index and next line, inserting
and deleting lines and characters, insert mode, erasing in the line and
the display, tab stops, saving and restoring the cursor, the character
sets G0 and G1 with the DEC special graphics, SO and SI, the graphic
renditions (bold, underline, blink, reverse), reverse screen, autowrap,
newline mode, the cursor key and keypad modes, the answers to DA,
DECID, DSR, CPR and DECREQTPARM, the screen alignment pattern, reset,
and VT52 mode. What it does not have is what a PC's display cannot
show: double-width and double-height lines, 132 columns and smooth
scrolling; and the printer port.

From later terminals, because programs written for PC consoles expect
them, as Linux's console has them: colours (SGR 30 to 49), ICH, ECH,
CHA, VPA, CNL, CPL, HPR, VPR, the SCO cursor save and restore (CSI s
and u), and hiding the cursor (DECTCEM).

Where a choice was made:

- A character in the last column leaves the cursor there, and the next
  goes on the next line, as a VT100 does. A carriage return or line
  feed in between cancels the wrap. Backspace from there goes to the
  column before.
- Inserting or deleting lines moves the cursor to the first column, as
  DEC's later terminals and xterm do; the VT102's guide does not say.
- Erasing leaves the background colour, as Linux and xterm do; the
  VT102 had none.
- A colour display cannot underline, so underlined text is cyan, unless
  it has a colour of its own. On a monochrome display it is underlined.
- The DEC special graphics are drawn with the PC font's (code page 437)
  line drawing characters. The few symbols the font lacks are a small
  square.
- Codes from 0200 up show the font's own characters.
- The kernel's messages go on the screen plainly, whatever modes a
  program has set, without disturbing an escape sequence it has half
  written.
- Anything malformed, or unknown, is ignored whole: a sequence with an
  intermediate character or a private marker it does not know, too many
  parameters (16 are kept), a parameter over 9999.

The terminal's capabilities are the termcap entry `pccons`, also called
`vt102-25`, in `usr/src/share/termcap/termcap.local`; upstream's
`reorder` script moves it to the front of the file, where `tgetent`,
which reads from the beginning, finds it first. It is a VT102 with 25
lines and no padding. Its `ks` sets only the cursor keys' mode, and not
the keypad's, so that the keypad's digits stay digits in `vi`.

## The keyboard

`usr/src/sys/i286at/kbd.c`, on the PS/2's keyboard controller, which
turns the keyboard's scan codes into the PC/XT's (set 1). The layout is
the US one. What a key sends depends on the modes the program on the
shown screen has set:

| key | sends |
|-----|-------|
| letters, digits, punctuation | the character; Caps Lock changes letters |
| Ctrl with a letter or `@[\]^_` | the control character; Ctrl-Space and Ctrl-2 NUL, Ctrl-6 `^^`, Ctrl-- and Ctrl-/ `^_`, Ctrl-? DEL |
| Alt with a key | ESC, then what the key sends |
| Backspace | BS (^H), as the VT102's does |
| Enter | CR, or CR LF in newline mode |
| cursor keys | `ESC [ A` to `D`; `ESC O A` to `D` in cursor key mode; `ESC A` to `D` in VT52 mode |
| F1 to F4 | `ESC O P` to `S`, the VT102's PF1 to PF4 (`ESC P` to `S` in VT52 mode) |
| F5 to F12 | `ESC [ 15 ~` to `ESC [ 24 ~`, the VT220's F6 to F14 |
| Home, Insert, Delete, End, Page Up, Page Down | `ESC [ 1 ~` to `ESC [ 6 ~`, the VT220's editing keys |
| keypad, Num Lock off | the cursor and editing keys printed on it |
| keypad, Num Lock on | its characters; in keypad mode `ESC O p` to `y` for the digits, and for `.` `-` `+` `*` `/` Enter `ESC O` `n` `m` `k` `j` `o` `M`, as xterm sends them (`ESC ?` for a VT52) |
| Alt-F1 to Alt-F4 | show that screen; what is typed goes there |
| Scroll Lock | the stop or the start character, whichever the output needs, as the VT102's NO SCROLL key |

Caps Lock and Num Lock have their lights; Num Lock starts on. A key held
down repeats unless the program has turned that off (DECARM).

Whether Backspace erases is the terminal modes' business, and as in
4.3BSD, which 2.11BSD's terminal driver, `getty`, `login`, `stty` and
`tset` come from, the system sets nothing up for a CRT. The kernel's
erase character is DEL, and `login` puts every line back to it, with
none of the CRT modes, whatever `getty` set for the line. A user's
`.profile` or `.login` does the rest, with `stty dec` or `stty crt` for
erasing that backs up over the character, and `erase ^H`. Root's
(`usr/src/etc/root`) do both, but upstream's set `erase ^H` on every
terminal, and these only where `TERM` is `pccons`: a terminal emulator
that logs in with `telnet` sends DEL for Backspace, as a VT100's
DELETE key does on a serial port. On a screen, a login without such
files sees Backspace echoed as `^H` and not obeyed.

## The serial ports

`usr/src/sys/i286at/com.c`: COM1 at 3F8 with interrupt request 4, and
COM2 at 2F8 with 3, each there if its line control register keeps what
is written to it. Speeds are set with `stty`, from 50 to 9600, with
19200 and 38400 as `exta` and `extb`. A port always sends eight bits
with no parity, and the tty layer makes parity in software when asked
for it, as before. Its lines are not watched: an open port is taken to
have a terminal on it. Closing it with `hupcl` set drops DTR. It moves a
character at a time; a 16550's FIFO is not used.

The speed's divisor is written only when the speed changes. Every
`TIOCSETP` and `TIOCSETN` gives a speed, and `tcsh`'s line editor sends
one at each prompt; an 8250 whose divisor is written while a character
is going out can send it wrong. While the divisor is being written, the
port's data register is the divisor's low byte, so interrupts are off
then: the driver first wrote it with them on, and when the port's
interrupt came in that moment, `comintr`, finding the port still ready
since nothing had gone, put the rest of the output queue into the
divisor. It lost the tail of whatever was being sent when `tcsh` gave
its prompt, about one prompt in fifteen in a test that typed the next
command as the last one's output began.

## Tests

- `tests/vt` builds `vt.c` with the C compiler of the machine the system
  is built on and runs it there: 302 checks of what a sequence leaves on
  the screen, where it leaves the cursor and what the terminal answers,
  then random bytes, weighted to escape sequences. Guard cells before
  and after the screen show that nothing writes outside it. Breaking the
  emulator on purpose makes the checks fail.
- `tests/system` logs in on COM1 as before, and then on the first screen
  with the keyboard, where it erases a character with Backspace, runs
  `vi`, opens a line above the first
  (which the terminal does by inserting a line), moves with the cursor
  keys and deletes a line; switches to the second screen with Alt-F2,
  logs in there, erases a line with ^U and types every shifted
  character; and goes back with Alt-F1. Then it checks what the screens
  showed, among them `vi`'s, all 25 lines, and that `who` lists the
  three logins.

The harness reads the screen through the VGA's own registers and
memory: 86Box's `text` request goes by the BIOS's notes, which are out
of date once the kernel shows screens of its own.

## Not done

- Other keyboard layouts, loadable key maps, and Ctrl-Alt-Del, which
  does nothing.
- Scrolling back, blanking the screen, fonts, more than 25 lines, a
  mouse.
- Modem control on the serial ports, and a 16550's FIFO.
- A kernel message printed from an interrupt while a program's output
  to the first screen is being drawn can land in the middle of it on
  the screen.
- None of it has run on a real machine: not the PS/2's own keyboard
  controller or display, and not the bell, which no test can hear.
