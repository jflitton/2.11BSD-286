#!/usr/bin/env python3
"""Run a test program in an emulator.

usage: runtest.py NAME SOURCE...
       runtest.py [--hd IMAGE,CYLINDERS,HEADS,SECTORS[,BUS]]
                  [--hd2 IMAGE,CYLINDERS,HEADS,SECTORS] [--until TEXT]
                  [--resets N] [--net CARD[:PORT:IRQ]] [--term TYPE:COLSxROWS]
                  [--mark TEXT] [--profile KERNEL[:MS]]
                  [--type AFTER TEXT | --key AFTER TEXT | --telnet AFTER TEXT]...
                  --boot NAME [EXECUTABLE [FILESYSTEM]]

The first form builds a small-model program that runs in real mode.  Each
.c is compiled with tools/cc286 and each .s assembled with NASM; they are
linked behind tests/harness/start.s and converted with tools/ne2aout.py, as
any program is.  With TOOLS_PROGRAMS=native in the environment it is the
system's own compiler, assembler and linker that make it, as they are
built for this machine (tools/ncc, obj/host/ccom/as and ld -real).

The second takes an executable already built, such as a kernel, that
starts itself as docs/boot.md describes.  A file system image given with
it goes on the diskette after it, for the kernel to load as its RAM disk.
With --hd the machine has a hard disk as well: IMAGE is its contents, as
tools/mkdisk.py makes them, and is changed by whatever the program writes.
BUS says what it is on: "ide", the default, the first drive of an AT disk
controller; "xtide", the same with the XTIDE Universal BIOS beside it,
since this machine's own BIOS does not boot from that controller; or "scsi:PORT:IRQ:DMA:ID:BIOS", an Adaptec AHA-1542CF at that
I/O address (hexadecimal), interrupt request and DMA channel, the disk at
that SCSI id, and the adapter's BIOS at that address (hexadecimal, such as
DC000) or, with 0, off.  86Box only.  With --hd and no EXECUTABLE there
is no diskette, and the machine boots from the hard disk as it would by
itself.  --hd2 gives the AT disk controller a second drive, the slave,
whose contents are IMAGE likewise; the first must be on that controller.

With --until the run does not end at the "@exit" line but when the program
has written TEXT after it: a system has its disks to bring up to date
before it stops, and says when it has.  With --resets the machine may be
reset N times, as a system starting itself again does, before a reset ends
the run; from each until the machine next writes to the serial port, which
it does once the BIOS has done and the boot program runs, its exceptions
and resets are the BIOS's and not counted.  With --type, once the machine
has written AFTER to the serial port, TEXT and a carriage return are typed
at it there; a second --type waits for its AFTER in what is written after
that, and so on.  --key types on the keyboard instead, once the screen has
changed since the last --key and shows AFTER just before the cursor, or,
if AFTER begins with "*", once the rest of it is on the screen more often
than it was when the last --key began.  TEXT is typed and then Return,
except that a TEXT that is only a key's name in braces, such as {Alt-F2},
is that key alone; TEXT may name keys so anywhere (keyname() says which),
and its control characters are typed with Ctrl.  Each screen a --key
waited for is saved in screen.txt, and the last one when the run ends.
The steps are taken in the order given, whichever kind they are.  86Box
with the automation socket only.

With --net the machine has an NE1000 or NE2000 of 86Box's: CARD is its
name there (novell_ne1k, ne1k, novell_ne2k, ne2k or ne2k8), at that I/O
address (hexadecimal) and interrupt request, 300 and 3 if not given.  It
is on 86Box's SLiRP, a network of its own, where the machine is to be
10.0.2.15 and 10.0.2.2 answers for the host; and a port of the host's,
free when the run starts, is forwarded to the machine's port 23, the
telnet server's.  86Box listens on it on every interface the host has,
so it is reachable from other machines while the run lasts, but only
while the machine's interface is up.  --telnet TEXT is typed on a
telnet connection to that port, opened at the first --telnet, once the
connection has shown AFTER since the last --telnet; what the machine
writes on it is printed after what it wrote to the serial port, under a
line "telnet:".  86Box with the automation socket only.  The connection
refuses to give a terminal type or a window size unless --term is
given, and then gives TYPE, and a window COLS wide and ROWS high.

With --mark, each time the machine writes TEXT to the serial port, the
emulated time, to the millisecond, goes to marks.txt as a line: the time
in milliseconds since the machine started, where in serial.txt the TEXT
is, and what the machine had written on that line before it.  A program
that writes TEXT just before and just after a piece of work has that
work timed by a clock that is not the system's own.  Text the machine
echoes counts, so a typed line that has TEXT in it must have it divided
by quotes.  With --profile the machine is stopped every MS milliseconds
of emulated time on average, 5 if not given, and where its processor is
goes to profile.txt with the name of the program that is running, which
is read from the memory of KERNEL, the kernel that is running, with its
symbol table.  tools/kprof.py takes the samples and reports on them.
Each costs a few milliseconds of the host's time and none of the
machine's.  The samples are from 1 to 2 MS - 1 milliseconds apart, by a
generator of random numbers that starts the same each run: at even
intervals they would keep step with the system's clock (kprof.py).
The time the --key steps take is not sampled.  Both are for 86Box with
the automation socket only.

A program given is put on a diskette image by tools/mkfd.py, behind the
boot sector, usr/src/sys/i286stand/fdboot.s, and the emulator boots it.
The boot sector is NASM's work, or with TOOLS_BOOT=native in the
environment the system's own assembler's, as -b (obj/host/ccom/as).
What the program writes to the serial port is printed.  The program ends by
writing a line "@exit NN", two hex digits, which become the exit status;
start.s writes it with the value main returns.  Everything built goes to
obj/tests/NAME.

The emulator is chosen by the environment variable EMU:

  86box  (default) An IBM PS/2 Model 30 286 with a 287: a real 286, which
         faults on anything a 286 lacks.  The program is the path in BOX86,
         or toolchain/86Box.app, or 86Box on PATH or in /Applications.
         An 86Box built with -D AUTOMATION=ON is driven through its
         automation socket: no window, emulated time as fast as the host
         runs it, and when a test gives no result, the machine's screen
         and where it was executing are printed.  If that 86Box also has
         the exceptions request, which is a change of ours to it, a test
         ends as soon as the machine is reset, and the exceptions the
         program took are printed too.  Any other 86Box opens a window,
         runs in real time and has COM1 on a pseudo-terminal.
  qemu   qemu-system-i386: a later processor, so it proves less.

TIMEOUT in the environment changes the limit on a run from 120 seconds.

Needs `. tools/env.sh` and nasm.
"""
import json
import mmap
import os
import random
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import termios
import time

TOP = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HARNESS = os.path.join(TOP, "tests", "harness")
# Seconds: emulated ones where the emulator counts them.
TIMEOUT = int(os.environ.get("TIMEOUT", "120"))
MAC_86BOX = "86Box.app/Contents/MacOS/86Box"


def run(*cmd, **kw):
    r = subprocess.run(cmd, text=True, capture_output=True, **kw)
    if r.returncode != 0:
        sys.exit("FAILED: %s\n%s%s" % (" ".join(cmd), r.stdout, r.stderr))
    return r.stdout + r.stderr


def tools(part):
    """Whose tools make that part, native or cross: what the environment
    says of it, or of everything (TOOLS), or the system's own."""
    return os.environ.get(part) or os.environ.get("TOOLS") or "native"


def build_native(name, sources, out):
    """The program made by the system's own tools: returns its path."""
    host = os.path.join(TOP, "obj", "host", "ccom")
    objs = []
    for src in [os.path.join(HARNESS, "start.s")] + sources:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(out, base + ".o")
        if src.endswith(".c"):
            run(os.path.join(TOP, "tools", "ncc"), "-ms", "-o", obj, src)
        else:
            run(os.path.join(host, "as"), "-ms",
                "-I" + os.path.dirname(src) + "/", "-o", obj, src)
        objs.append(obj)
    exe = os.path.join(out, name)
    log = run(os.path.join(host, "ld"), "-ms", "-real", str(0x1000), "-M",
              "-o", exe, *objs)
    with open(os.path.join(out, name + ".map"), "w") as f:
        f.write(log)
    return exe


def build(name, sources, out):
    """Returns the path of the diskette image."""
    if tools("TOOLS_PROGRAMS") == "native":
        exe = build_native(name, sources, out)
        return build_image(name, exe, out)
    objs = []
    for src in [os.path.join(HARNESS, "start.s")] + sources:
        base = os.path.splitext(os.path.basename(src))[0]
        obj = os.path.join(out, base + ".o")
        if src.endswith(".c"):
            log = run(os.path.join(TOP, "tools", "cc286"), "-ms", "-o", obj, src)
            if "Error" in log:
                sys.exit(log)
        else:
            run("nasm", "-f", "obj", "-I", os.path.dirname(src) + "/", "-o", obj, src)
        objs.append(obj)

    ne = os.path.join(out, name + ".ne")
    mapfile = os.path.join(out, name + ".map")
    log = run("wlink", "op", "q", "format", "os2", "name", ne,
              "op", "nodefaultlibs,map=" + mapfile, "file", ",".join(objs))
    if "Error" in log:
        sys.exit(log)
    exe = os.path.join(out, name)
    run(sys.executable, "-I", os.path.join(TOP, "tools", "ne2aout.py"),
        "--model", "s", "--real", "0x1000", "--map", mapfile, ne, exe)
    return build_image(name, exe, out)


def build_image(name, exe, out):
    """The diskette image of the program EXE: returns its path."""
    # start.s is linked first, so the entry point is the start of the code.
    # The code and the data are two segments, and far variables more.
    b = open(exe, "rb").read()
    magic, model, nseg, eseg, eoff = struct.unpack_from("<HHHHH", b, 0)
    if nseg < 2 or (eseg, eoff) != (1, 0):
        sys.exit("%s: want two segments or more and entry 1:0000, have %d "
                 "and %d:%04x" % (name, nseg, eseg, eoff))
    img = os.path.join(out, name + ".img")
    mkfd(exe, img, out, "--spread")
    return img


def mkfd(exe, img, out, *options):
    """Make the diskette image IMG that boots the executable EXE."""
    bootbin = os.path.join(out, "fdboot.bin")
    src = os.path.join(TOP, "usr", "src", "sys", "i286stand", "fdboot.s")
    if tools("TOOLS_BOOT") == "native":
        run(os.path.join(TOP, "obj", "host", "ccom", "as"), "-b", "-o",
            bootbin, src)
    else:
        run("nasm", "-f", "bin", "-o", bootbin, src)
    run(sys.executable, "-I", os.path.join(TOP, "tools", "mkfd.py"),
        *options, bootbin, exe, img)


def finished(text):
    """The exit status from an "@exit NN" line, or None."""
    i = text.rfind("@exit ")
    if i < 0 or len(text) < i + 8:
        return None
    try:
        return int(text[i + 6:i + 8], 16)
    except ValueError:
        return None


UNTIL = None            # --until: what must follow the "@exit" line
RESETS = 0              # --resets: how many resets are not failures
TYPE = []               # --type, --key, --telnet: [kind, after, text]
NET = None              # --net: (card, port, irq, the host's port)
TERM = None             # --term: (type, columns, rows)
MARK = None             # --mark: the text whose every writing is timed
PROFILE = None          # --profile: (kernel, milliseconds between samples)


class Typist:
    """Types each TEXT of --type when its AFTER has been written, and of
    --key when the screen shows its AFTER."""

    def __init__(self):
        self.todo = list(TYPE)
        self.since = 0          # where in the output the last was typed
        self.before = None      # the screen the last --key waited for
        self.netsince = 0       # where in the telnet text the last was

    def kind(self):
        """What the next step types on: "serial", "key", "telnet", or
        None."""
        return self.todo[0][0] if self.todo else None

    def nextnet(self, text):
        """What to send on the telnet connection now that TEXT has come
        on it, or None."""
        if self.kind() != "telnet" or self.todo[0][1] not in text[self.netsince:]:
            return None
        self.netsince = len(text)
        return (self.todo.pop(0)[2] + "\r\n").encode("latin-1")

    def next(self, text):
        """What to type on the serial port now that TEXT has been
        written, or None."""
        if self.kind() != "serial" or self.todo[0][1] not in text[self.since:]:
            return None
        self.since = len(text)
        return (self.todo.pop(0)[2] + "\r").encode("latin-1")

    def nextkey(self, rows, cursor):
        """What to type on the keyboard now that the screen is ROWS with
        the cursor at CURSOR, or None.  The screen the last --key
        waited for is what this one is judged against.  A prompt counts
        only on a different screen: until what was typed there has been
        echoed, the old prompt is still before the cursor.  "*" text
        counts only if there is more of it than there was on that
        screen, since the program may have drawn it while the typing
        went on."""
        if self.kind() != "key":
            return None
        after = self.todo[0][1]
        if after.startswith("*"):
            count = lambda rs: sum(r.count(after[1:]) for r in rs)
            if count(rows) <= (count(self.before[0]) if self.before else 0):
                return None
        elif ((rows, cursor) == self.before or not 0 <= cursor[0] < len(rows)
              or not rows[cursor[0]][:cursor[1]].endswith(after)):
            return None
        self.before = (rows, cursor)
        return self.todo.pop(0)[2]


def screen(box):
    """The text screen as the display shows it: its rows, and the
    cursor's row and column.  Where the display starts and where the
    cursor is come from the CRT controller, whose index register is put
    back as it was: the BIOS's notes, which the text request goes by,
    are out of date once a system shows screens of its own."""
    index = box.ask("ior 0x3d4 1")["values"][0]
    regs = []
    for r in (0x0C, 0x0D, 0x0E, 0x0F):
        box.ask("iow 0x3d4 1 %d" % r)
        regs.append(box.ask("ior 0x3d5 1")["values"][0])
    box.ask("iow 0x3d4 1 %d" % index)
    start = regs[0] << 8 | regs[1]
    data = bytes.fromhex(box.ask("memr %d %d" % (0xB8000 + 2 * start, 25 * 160))["data"])
    rows = [data[i * 160:(i + 1) * 160:2].decode("cp437") for i in range(25)]
    return rows, divmod((regs[2] << 8 | regs[3]) - start, 80)


# Set-1 scan codes: the keys of the US layout, and the named ones.
KEYROWS = [("1234567890-=", "!@#$%^&*()_+", 0x02),
           ("qwertyuiop[]", "QWERTYUIOP{}", 0x10),
           ("asdfghjkl;'`", 'ASDFGHJKL:"~', 0x1E),
           ("\\zxcvbnm,./", "|ZXCVBNM<>?", 0x2B)]
NAMEDKEYS = {"Esc": 0x01, "Backspace": 0x0E, "Tab": 0x0F, "Enter": 0x1C,
             "Up": 0x148, "Down": 0x150, "Left": 0x14B, "Right": 0x14D,
             "Home": 0x147, "End": 0x14F, "PgUp": 0x149, "PgDn": 0x151,
             "Ins": 0x152, "Del": 0x153, "F11": 0x57, "F12": 0x58}
NAMEDKEYS.update(("F%d" % n, 0x3A + n) for n in range(1, 11))
MODIFIERS = {"Shift": 0x2A, "Ctrl": 0x1D, "Alt": 0x38}


def keyname(name):
    """The scan codes to hold down for a key named as in {Alt-F2}: any of
    Shift-, Ctrl- and Alt-, then a name in NAMEDKEYS.  None if NAME is
    not one."""
    *mods, key = name.split("-")
    if key not in NAMEDKEYS or any(m not in MODIFIERS for m in mods):
        return None
    return [MODIFIERS[m] for m in mods] + [NAMEDKEYS[key]]


def keystrokes(text):
    """TEXT as keys to press, each a list of scan codes held together."""
    out = []
    i = 0
    while i < len(text):
        c = text[i]
        end = text.find("}", i)
        if c == "{" and end > i and keyname(text[i + 1:end]):
            out.append(keyname(text[i + 1:end]))
            i = end + 1
            continue
        i += 1
        codes = {" ": [0x39], "\r": [0x1C], "\t": [0x0F],
                 "\033": [0x01]}.get(c)
        for plain, shifted, first in KEYROWS:
            if c in plain:
                codes = [first + plain.index(c)]
            elif c in shifted:
                codes = [0x2A, first + shifted.index(c)]
            elif "\001" <= c <= "\032" and chr(ord(c) + 96) in plain:
                codes = [0x1D, first + plain.index(chr(ord(c) + 96))]
        if codes is None:
            sys.exit("--key: no key types %r" % c)
        out.append(codes)
    return out


def typekeys(box, text):
    """Type TEXT on the keyboard: then Return, unless it is only a key's
    name in braces."""
    keys = keystrokes(text)
    m = text.startswith("{") and text.endswith("}") and keyname(text[1:-1])
    if not m:
        keys.append([0x1C])
    for codes in keys:
        box.ask("key down " + " ".join("0x%x" % c for c in codes))
        box.ask("key up " + " ".join("0x%x" % c for c in reversed(codes)))


def ended(text):
    """Has the program written all it is going to?"""
    if finished(text) is None:
        return False
    return UNTIL is None or UNTIL in text[text.rfind("@exit "):]


def run_qemu(img, out):
    serial = os.path.join(out, "serial.txt")
    if os.path.exists(serial):
        os.remove(serial)
    p = subprocess.Popen(
        ["qemu-system-i386", "-display", "none", "-no-reboot",
         "-drive", "if=floppy,format=raw,file=" + img, "-boot", "a",
         "-serial", "file:" + serial],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    text = ""
    deadline = time.time() + TIMEOUT
    try:
        while time.time() < deadline and p.poll() is None:
            time.sleep(0.2)
            if os.path.exists(serial):
                text = open(serial, errors="replace").read()
                if ended(text):
                    break
    finally:
        p.kill()
        p.wait()
    return text


class Telnet:
    """The host's end of a telnet connection to the machine.  The machine
    may echo and suppress go-ahead, and with TERM, (type, columns, rows),
    may have the terminal's type and window size; any other option it
    offers, and any it asks for, is refused."""

    IAC, DONT, DO, WONT, WILL, SB, SE = 255, 254, 253, 252, 251, 250, 240
    ECHO, SGA, TTYPE, NAWS = 1, 3, 24, 31
    IS, SEND = 0, 1

    def __init__(self, port, term=None):
        self.term = term
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=10)
        self.sock.setblocking(False)
        self.pending = b""      # a command not yet all here
        self.text = ""          # what the machine has written
        self.answered = set()   # (command, option) already answered
        self.closed = False

    def poll(self):
        if self.closed:
            return
        try:
            data = self.sock.recv(4096)
        except BlockingIOError:
            return
        except OSError:
            data = b""
        if not data:
            self.closed = True
            self.sock.close()
            return
        buf = self.pending + data
        out = bytearray()
        i = 0
        while i < len(buf):
            if buf[i] != self.IAC:
                out.append(buf[i])
                i += 1
                continue
            if i + 1 >= len(buf):
                break
            cmd = buf[i + 1]
            if cmd == self.IAC:
                out.append(self.IAC)
                i += 2
            elif cmd in (self.DO, self.DONT, self.WILL, self.WONT):
                if i + 2 >= len(buf):
                    break
                opt = buf[i + 2]
                if cmd == self.DO and self.term and opt in (self.TTYPE,
                                                            self.NAWS):
                    if self.answer(self.WILL, opt) and opt == self.NAWS:
                        size = struct.pack(">HH", *self.term[1:])
                        self.sock.sendall(bytes([self.IAC, self.SB, opt]) +
                                          size.replace(b"\xff", b"\xff\xff") +
                                          bytes([self.IAC, self.SE]))
                elif cmd == self.DO:
                    self.answer(self.WONT, opt)
                elif cmd == self.WILL:
                    self.answer(self.DO if opt in (self.ECHO, self.SGA)
                                else self.DONT, opt)
                i += 3
            elif cmd == self.SB:
                end = buf.find(bytes([self.IAC, self.SE]), i)
                if end < 0:
                    break
                if self.term and buf[i + 2:end] == bytes([self.TTYPE, self.SEND]):
                    self.sock.sendall(bytes([self.IAC, self.SB, self.TTYPE, self.IS]) +
                                      self.term[0].encode("latin-1") +
                                      bytes([self.IAC, self.SE]))
                i = end + 2
            else:
                i += 2
        self.pending = buf[i:]
        self.text += out.decode("latin-1")

    def answer(self, cmd, opt):
        """Answer CMD for OPT unless that has been; whether it was now."""
        if (cmd, opt) in self.answered:
            return False
        self.answered.add((cmd, opt))
        self.sock.sendall(bytes([self.IAC, cmd, opt]))
        return True

    def send(self, data):
        self.sock.setblocking(True)
        self.sock.sendall(data.replace(b"\xff", b"\xff\xff"))
        self.sock.setblocking(False)

    def close(self):
        if not self.closed:
            self.closed = True
            self.sock.close()


NETCARDS = {"novell_ne1k": "Novell NE1000", "ne1k": "NE1000 Compatible",
            "novell_ne2k": "Novell NE2000", "ne2k": "NE2000 Compatible",
            "ne2k8": "NE2000 Compatible 8-bit"}


def free_port():
    """A TCP port of the host's that nothing is listening on now."""
    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def find_86box():
    for box in (os.environ.get("BOX86"),
                os.path.join(TOP, "toolchain", MAC_86BOX),
                shutil.which("86Box"),
                os.path.join("/Applications", MAC_86BOX)):
        if box and os.path.exists(box):
            return box
    sys.exit("86Box not found; set BOX86 to it, or use EMU=qemu")


def has_automation(box):
    """Whether this 86Box was built with the automation socket.  Asking it
    is no use: one built without answers an option it doesn't know with a
    dialog."""
    with open(box, "rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as m:
        return m.find(b"--automation-hold") >= 0


HD = None               # --hd: (image, cylinders, heads, sectors, bus)
HD2 = None              # --hd2: (image, cylinders, heads, sectors)


def start_86box(box, img, out, com1, args=(), env=None):
    """Start 86Box on a copy of the harness's machine, with the diskette
    image, if there is one, in drive A, COM1 as the configuration lines
    COM1 say, and the hard disk HD if there is one.  Returns the process
    and the machine's directory."""
    vm = os.path.join(out, "86box")
    shutil.rmtree(vm, ignore_errors=True)
    os.makedirs(os.path.join(vm, "nvr"))
    cfg = open(os.path.join(HARNESS, "86box.cfg")).read()
    scsi = HD[4].split(":")[1:] if HD and HD[4].startswith("scsi") else None
    ide = "hdc_1 = ide_isa\n"
    assert ide in cfg
    if scsi:
        cfg = cfg.replace(ide, ide + "scsicard_1 = aha154xcf\n")
    elif HD and HD[4] == "xtide":
        cfg = cfg.replace(ide, "hdc_1 = xtide_at_ps2_1ch\n")
    with open(os.path.join(vm, "86box.cfg"), "w") as f:
        f.write(cfg + ("fdd_01_fn = %s\n" % img if img else "")
                + "\n[Ports (COM & LPT)]\n" + com1)
        if HD:
            f.write("\n[Hard disks]\nhdd_01_parameters = %d, %d, %d, 0, %s\n"
                    "hdd_01_fn = %s\n"
                    % (HD[3], HD[2], HD[1], "scsi" if scsi else "ide",
                       os.path.abspath(HD[0])))
            if scsi:
                f.write("hdd_01_scsi_location = 0:%02d\n\n"
                        "[Adaptec AHA-154xCF #1]\nbase = %04X\nirq = %d\n"
                        "dma = %d\nbios_addr = %05X\n"
                        % (int(scsi[3]), int(scsi[0], 16), int(scsi[1]),
                           int(scsi[2]), int(scsi[4], 16)))
            else:
                f.write("hdd_01_ide_channel = 0:0\n")
            if HD2:
                f.write("hdd_02_parameters = %d, %d, %d, 0, ide\n"
                        "hdd_02_fn = %s\nhdd_02_ide_channel = 0:1\n"
                        % (HD2[3], HD2[2], HD2[1], os.path.abspath(HD2[0])))
        if NET:
            card, port, irq, hostport = NET
            f.write("\n[Network]\nnet_01_card = %s\nnet_01_net_type = slirp\n\n"
                    "[%s #1]\nbase = %04X\nirq = %d\nmac = 12:34:56\n\n"
                    "[SLiRP Port Forwarding #1]\n0_protocol = tcp\n"
                    "0_external = %d\n0_internal = 23\n"
                    % (card, NETCARDS[card], port, irq, hostport))
    shutil.copy(os.path.join(HARNESS, "ibmps2_m30_286.nvr"), os.path.join(vm, "nvr"))
    with open(os.path.join(vm, "86box.out"), "w") as o:
        p = subprocess.Popen([box, "-N", "-P", vm, "-L", os.path.join(vm, "86box.log")]
                             + list(args), env=env, stdout=o,
                             stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
    return p, vm


class Automation:
    """86Box's automation socket: a request is a line of words and the
    reply a line of JSON.  doc/specifications/automation.md in 86Box's
    source describes the requests."""

    def __init__(self, path, proc):
        deadline = time.time() + 30
        while True:
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            try:
                self.sock.connect(path)
                break
            except OSError:
                self.sock.close()
                if proc.poll() is not None or time.time() > deadline:
                    raise EOFError
                time.sleep(0.05)
        self.sock.settimeout(300)
        self.file = self.sock.makefile("r", encoding="latin-1", newline="\n")

    def ask(self, line):
        self.sock.sendall(line.encode("latin-1") + b"\n")
        reply = self.file.readline()
        if not reply:
            raise EOFError
        reply = json.loads(reply)
        if not reply["ok"]:
            raise RuntimeError("86Box: %s: %s" % (line, reply["error"]))
        return reply

    def close(self):
        self.file.close()
        self.sock.close()


VECTORS = {0: "divide error", 1: "debug trap", 2: "NMI", 5: "bound range",
           6: "invalid opcode", 7: "no coprocessor", 8: "double fault",
           9: "coprocessor segment overrun", 10: "invalid TSS",
           11: "segment not present", 12: "stack fault",
           13: "general protection", 16: "coprocessor error"}


def events(box, since):
    """The exceptions the processor took and its resets, from position SINCE
    on, and the position after them; or None from an 86Box without the
    exceptions request, which 86Box's own source doesn't have."""
    try:
        r = box.ask("exceptions %d" % since)
    except RuntimeError:
        return None, since
    return r["events"], r["next"]


def bios_move(e):
    """Is this reset the BIOS's own way back to real mode, after moving a
    block of extended memory for a program (INT 15h, AH = 87h)?  A 286
    has no other way back.  The BIOS does it from the code segment that
    call has it make, the fifth descriptor of the caller's table."""
    return e["cs"] == 0x20 and e.get("source") == "kbc"


def describe(box, mark):
    """What can be seen of a machine that gave no result.  MARK is where
    the program's events start, or None."""
    lines = []
    try:
        box.ask("text")         # which fails in a graphics mode
        rows = [row.replace("\0", " ").rstrip() for row in screen(box)[0]]
        rows = ["  | " + row for row in rows if row]
        lines += ["its screen, less blank lines:"] + rows if rows else ["its screen is blank"]
    except RuntimeError:
        lines.append("its screen is in a graphics mode")
    r = box.ask("regs")
    lines.append("executing at %04x:%04x, in %s mode"
                 % (r["cs"], r["eip"], "protected" if r["cr0"] & 1 else "real"))
    evs = events(box, mark)[0] if mark is not None else None
    if evs:
        lines.append("the program's last exceptions and resets:")
    runs = []           # [line, times] for each run of the same event
    for e in evs or []:
        if e["type"] == "reset":
            what = "reset (%s)" % e["source"]
        else:
            what = "%d, %s" % (e["vector"], VECTORS.get(e["vector"], "?"))
            if "error" in e:
                what += ", error %04x" % e["error"]
            if "during" in e:
                what += ", delivering %d" % e["during"]
        what = "at %04x:%04x  %s" % (e["cs"], e["eip"], what)
        if runs and runs[-1][0] == what:
            runs[-1][1] += 1
        else:
            runs.append([what, 1, e["ms"]])
    for what, times, ms in runs[-12:]:
        lines.append("  %8.3f s  %s%s" % (ms / 1000, what, (", %d times%s" % (times, " or more" if times >= 256 else "")) if times > 1 else ""))
    return "\n".join(lines)


def run_86box_socket(box, img, out):
    # The socket's path has to fit a sockaddr_un, which OUT may not.
    tmp = tempfile.mkdtemp(prefix="86box")
    path = os.path.join(tmp, "s")
    env = dict(os.environ)
    env.setdefault("QT_QPA_PLATFORM", "offscreen")      # no window
    # Held, the machine runs only when asked to, and then unpaced.
    p, vm = start_86box(box, img, out, "serial1_device = automation\n",
                        ["--automation", "unix:" + path, "--automation-hold"], env)
    raw = b""
    seen = None
    try:
        auto = Automation(path, p)
        # The BIOS has done with its own exceptions and resets once the
        # boot sector is in memory.
        with open(img or HD[0], "rb") as f:
            boot = f.read(16).hex()
        if auto.ask("wait mem %d 0x7c00 %s" % (TIMEOUT * 1000, boot))["matched"]:
            mark = scan = events(auto, 1 << 62)[1]
        else:
            mark = None
            seen = ("the BIOS did not load the %s\n" % ("diskette" if img else "disk")
                    + describe(auto, None))
        pos = 0
        typist = Typist()
        tel = None
        marks = []              # (emulated ms, where in the output)
        markpos = 0
        sampler = None
        if PROFILE:
            sys.path.insert(0, os.path.join(TOP, "tools"))
            import kprof
            sampler = kprof.Sampler(auto.ask, PROFILE[0], os.path.join(out, "profile.txt"))
            spread = random.Random(1)
        now = sample = auto.ask("status")["ms"]     # when the next one is due

        def advance(ms):
            """Run the machine for MS of emulated time, a sample's worth
            at a time if it is being profiled, and note each MARK that
            comes.  Returns the emulated time."""
            nonlocal now, markpos, sample
            end = now + ms
            while now < end:
                if sampler and sample <= now:
                    sample = now + spread.randint(1, 2 * PROFILE[1] - 1)
                n = (min(end, sample) if sampler else end) - now
                if MARK:
                    r = auto.ask("wait serial %d 1 %d %s" % (n, markpos, MARK))
                    if r["matched"]:
                        marks.append((r["ms"], r["at"]))
                        markpos = r["at"] + len(MARK)
                else:
                    r = auto.ask("step %d" % n)
                now = r["ms"]
                if sampler and now >= sample:
                    sampler.sample(now)
            return now
        resets = RESETS
        booting = None          # where the serial output was at a reset
        screens = open(os.path.join(out, "screen.txt"), "w", encoding="utf-8")
        def keep(rows):
            screens.write("".join(r.rstrip() + "\n" for r in rows) + "=" * 80 + "\n")
        while not seen and not ended(raw.decode("latin-1")):
            late = advance(200) >= TIMEOUT * 1000
            r = auto.ask("serial read 1 %d" % pos)
            raw += bytes.fromhex(r["data"])
            pos = r["next"]
            # A character at a time, as a slow typist would: what reads
            # it may be echoing each with the BIOS.
            for c in typist.next(raw.decode("latin-1")) or b"":
                auto.ask("serial write 1 %02x" % c)
                advance(20)
            if typist.kind() == "key":
                rows, cursor = screen(auto)
                text = typist.nextkey(rows, cursor)
                if text is not None:
                    keep(rows)
                    typekeys(auto, text)
            if typist.kind() == "telnet" and tel is None:
                try:
                    tel = Telnet(NET[3], TERM)
                except OSError as e:
                    seen = "no telnet connection to port %d: %s\n" % (NET[3], e)
            if tel:
                tel.poll()
                data = typist.nextnet(tel.text)
                if data:
                    tel.send(data)
            evs, scan = events(auto, scan)
            if booting is not None:
                if len(raw) > booting:
                    booting, mark = None, scan
                evs = None
            if any(e["type"] == "reset" and not bios_move(e) for e in evs or []):
                if resets:
                    resets -= 1
                    booting = len(raw)
                else:
                    seen = "the machine was reset\n" + describe(auto, mark)
            elif late:
                seen = describe(auto, mark)
        if typist.before is not None:
            keep(screen(auto)[0])
        screens.close()
        if sampler:
            sampler.close()
        if MARK:
            # What the machine wrote after the last look is still to read.
            r = auto.ask("serial read 1 %d" % pos)
            raw += bytes.fromhex(r["data"])
            with open(os.path.join(out, "marks.txt"), "w") as f:
                for ms, at in marks:
                    line = raw[raw.rfind(b"\n", 0, at) + 1:at].decode("latin-1")
                    f.write("%d %d %s\n" % (ms, at, line.replace("\r", "")))
        auto.ask("quit")
        auto.close()
    except (EOFError, OSError):
        seen = "86Box stopped or did not answer; see " + os.path.join(vm, "86box.out")
    finally:
        try:
            p.wait(10)
        except subprocess.TimeoutExpired:
            p.kill()
            p.wait()
        shutil.rmtree(tmp, ignore_errors=True)
    if tel:
        tel.poll()
        tel.close()
    return raw.decode("latin-1"), seen, tel.text if tel else None


def run_86box_pty(box, img, out):
    if any(t[0] in ("key", "telnet") for t in TYPE):
        sys.exit("--key and --telnet need an 86Box with the automation socket")
    if MARK or PROFILE:
        sys.exit("--mark and --profile need an 86Box with the automation socket")
    p, vm = start_86box(box, img, out,
                        "serial1_passthrough_enabled = 1\n\n"
                        "[Serial Passthrough Device #1]\nmode = 0\n"
                        "baudrate = 9600\ndata_bits = 8\nstop_bits = 1\n")
    boxout = os.path.join(vm, "86box.out")
    text = ""
    fd = None
    deadline = time.time() + TIMEOUT
    try:
        pty = None
        while pty is None and time.time() < deadline and p.poll() is None:
            time.sleep(0.2)
            for line in open(boxout, errors="replace"):
                if "Slave side is " in line:
                    pty = line.split("Slave side is ")[1].split()[0]
        if pty is None:
            sys.exit("86Box did not report a serial pseudo-terminal; see " + boxout)
        fd = os.open(pty, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        attr = termios.tcgetattr(fd)
        attr[0] = attr[1] = attr[3] = 0          # raw: no translation, no echo
        attr[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        termios.tcsetattr(fd, termios.TCSANOW, attr)
        raw = b""
        typist = Typist()
        while time.time() < deadline and p.poll() is None:
            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                chunk = b""
            if not chunk:
                time.sleep(0.05)
                continue
            raw += chunk
            text = raw.decode("latin-1")
            for c in typist.next(text) or b"":
                os.write(fd, bytes([c]))
                time.sleep(0.05)
            if ended(text):
                break
        text = raw.decode("latin-1")
    finally:
        if fd is not None:
            os.close(fd)
        p.send_signal(signal.SIGKILL)       # 86Box ignores SIGTERM
        p.wait()
    return text, None, None


def run_86box(img, out):
    box = find_86box()
    run = run_86box_socket if has_automation(box) else run_86box_pty
    text, seen, teltext = run(box, img, out)
    with open(os.path.join(out, "serial.txt"), "w", newline="") as f:
        f.write(text)
    if teltext is not None:
        with open(os.path.join(out, "telnet.txt"), "w", newline="") as f:
            f.write(teltext)
    return text, seen, teltext


def main():
    global HD, HD2, UNTIL, RESETS, NET, TERM, MARK, PROFILE
    emu = os.environ.get("EMU", "86box")
    if emu not in ("86box", "qemu"):
        sys.exit("EMU must be 86box or qemu")
    if len(sys.argv) > 2 and sys.argv[1] == "--hd":
        try:
            image, c, h, s, bus = (sys.argv[2] + ",ide").split(",")[:5]
            HD = (image, int(c), int(h), int(s), bus)
            if bus not in ("ide", "xtide") and (
                    bus.count(":") != 5 or not bus.startswith("scsi:")):
                raise ValueError
        except ValueError:
            sys.exit(__doc__)
        if emu != "86box" or os.path.getsize(image) != HD[1] * HD[2] * HD[3] * 512:
            sys.exit("--hd: needs 86Box, and an image of the size its geometry gives")
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--hd2":
        try:
            image, c, h, s = sys.argv[2].split(",")
            HD2 = (image, int(c), int(h), int(s))
        except ValueError:
            sys.exit(__doc__)
        if not HD or HD[4] not in ("ide", "xtide") or \
                os.path.getsize(image) != HD2[1] * HD2[2] * HD2[3] * 512:
            sys.exit("--hd2: needs --hd on the AT disk controller, and an "
                     "image of the size its geometry gives")
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--until":
        UNTIL = sys.argv[2]
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--resets":
        try:
            RESETS = int(sys.argv[2])
        except ValueError:
            sys.exit(__doc__)
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--net":
        try:
            card, port, irq = (sys.argv[2] + ":300:3").split(":")[:3]
            if card not in NETCARDS:
                raise ValueError
            NET = (card, int(port, 16), int(irq), free_port())
        except ValueError:
            sys.exit(__doc__)
        if emu != "86box":
            sys.exit("--net: needs 86Box")
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--term":
        try:
            name, size = sys.argv[2].rsplit(":", 1)
            cols, rows = size.split("x")
            TERM = (name, int(cols), int(rows))
        except ValueError:
            sys.exit(__doc__)
        if not NET:
            sys.exit("--term: needs --net")
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--mark":
        MARK = sys.argv[2]
        del sys.argv[1:3]
    if len(sys.argv) > 2 and sys.argv[1] == "--profile":
        try:
            kernel, _, ms = sys.argv[2].partition(":")
            PROFILE = (kernel, int(ms or "5"))
            if PROFILE[1] < 1:
                raise ValueError
        except ValueError:
            sys.exit(__doc__)
        del sys.argv[1:3]
    if (MARK or PROFILE) and emu != "86box":
        sys.exit("--mark and --profile: need 86Box")
    kinds = {"--type": "serial", "--key": "key", "--telnet": "telnet"}
    while len(sys.argv) > 3 and sys.argv[1] in kinds:
        if sys.argv[1] == "--telnet" and not NET:
            sys.exit("--telnet: needs --net")
        TYPE.append([kinds[sys.argv[1]]] + sys.argv[2:4])
        del sys.argv[1:4]
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    if sys.argv[1] == "--boot":
        if len(sys.argv) not in (4, 5) and not (len(sys.argv) == 3 and HD):
            sys.exit(__doc__)
        name = sys.argv[2]
        out = os.path.join(TOP, "obj", "tests", name)
        os.makedirs(out, exist_ok=True)
        img = None
        if len(sys.argv) > 3:
            img = os.path.join(out, name + ".img")
            mkfd(sys.argv[3], img, out,
                 *(["--rd", sys.argv[4]] if len(sys.argv) == 5 else []))
    else:
        name, sources = sys.argv[1], sys.argv[2:]
        out = os.path.join(TOP, "obj", "tests", name)
        os.makedirs(out, exist_ok=True)
        img = build(name, sources, out)
    # The program's output, and what the emulator can say of a failure.
    text, seen, teltext = (run_86box(img, out) if emu == "86box"
                           else (run_qemu(img, out), None, None))
    sys.stdout.write(text.replace("\r", ""))
    if teltext is not None:
        sys.stdout.write("\ntelnet:\n" + teltext.replace("\r", ""))
    sys.stdout.flush()
    if seen:
        print("\n" + seen, file=sys.stderr)
    status = finished(text)
    if status is None:
        sys.exit("\n%s: no result on %s (the limit is %d seconds)" % (name, emu, TIMEOUT))
    if not ended(text):
        sys.exit("\n%s: gave its result on %s, but never wrote \"%s\"" % (name, emu, UNTIL))
    print("%s: program returned %d on %s" % (name, status, emu))
    sys.exit(0 if status == 0 else 1)


if __name__ == "__main__":
    main()
