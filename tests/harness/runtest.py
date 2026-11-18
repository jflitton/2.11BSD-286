#!/usr/bin/env python3
"""Run a test program in an emulator.

usage: runtest.py NAME SOURCE...
       runtest.py --boot NAME EXECUTABLE

The first form builds a small-model program that runs in real mode.  Each
.c is compiled with tools/cc286 and each .s assembled with NASM; they are
linked behind tests/harness/start.s and converted with tools/ne2aout.py, as
any program is.

The second takes an executable already built, such as a kernel, that
starts itself as docs/boot.md describes.

Either way tools/mkfd.py puts the program on a diskette image behind the
boot sector, usr/src/sys/i286stand/fdboot.s, and the emulator boots it.
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

Needs `. tools/env.sh` and nasm.
"""
import json
import mmap
import os
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
TIMEOUT = 120           # seconds: emulated ones where the emulator counts them
MAC_86BOX = "86Box.app/Contents/MacOS/86Box"


def run(*cmd, **kw):
    r = subprocess.run(cmd, text=True, capture_output=True, **kw)
    if r.returncode != 0:
        sys.exit("FAILED: %s\n%s%s" % (" ".join(cmd), r.stdout, r.stderr))
    return r.stdout + r.stderr


def build(name, sources, out):
    """Returns the path of the diskette image."""
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

    # start.s is linked first, so the entry point is the start of the code.
    b = open(exe, "rb").read()
    magic, model, nseg, eseg, eoff = struct.unpack_from("<HHHHH", b, 0)
    if nseg != 2 or (eseg, eoff) != (1, 0):
        sys.exit("%s: want two segments and entry 1:0000, have %d and %d:%04x"
                 % (name, nseg, eseg, eoff))
    img = os.path.join(out, name + ".img")
    mkfd(exe, img, out, "--spread")
    return img


def mkfd(exe, img, out, *options):
    """Make the diskette image IMG that boots the executable EXE."""
    bootbin = os.path.join(out, "fdboot.bin")
    run("nasm", "-f", "bin", "-o", bootbin,
        os.path.join(TOP, "usr", "src", "sys", "i286stand", "fdboot.s"))
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
                if finished(text) is not None:
                    break
    finally:
        p.kill()
        p.wait()
    return text


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


def start_86box(box, img, out, com1, args=(), env=None):
    """Start 86Box on a copy of the harness's machine, with the diskette
    image in drive A and COM1 as the configuration lines COM1 say.
    Returns the process and the machine's directory."""
    vm = os.path.join(out, "86box")
    shutil.rmtree(vm, ignore_errors=True)
    os.makedirs(os.path.join(vm, "nvr"))
    cfg = open(os.path.join(HARNESS, "86box.cfg")).read()
    with open(os.path.join(vm, "86box.cfg"), "w") as f:
        f.write(cfg + "fdd_01_fn = %s\n\n[Ports (COM & LPT)]\n%s" % (img, com1))
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


def describe(box, mark):
    """What can be seen of a machine that gave no result.  MARK is where
    the program's events start, or None."""
    lines = []
    try:
        rows = [bytes.fromhex(row).decode("cp437").replace("\0", " ").rstrip()
                for row in box.ask("text")["chars"]]
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
        with open(img, "rb") as f:
            boot = f.read(16).hex()
        if auto.ask("wait mem %d 0x7c00 %s" % (TIMEOUT * 1000, boot))["matched"]:
            mark = scan = events(auto, 1 << 62)[1]
        else:
            mark = None
            seen = "the BIOS did not load the diskette\n" + describe(auto, None)
        pos = 0
        while not seen and finished(raw.decode("latin-1")) is None:
            late = auto.ask("step 200")["ms"] >= TIMEOUT * 1000
            r = auto.ask("serial read 1 %d" % pos)
            raw += bytes.fromhex(r["data"])
            pos = r["next"]
            evs, scan = events(auto, scan)
            if any(e["type"] == "reset" for e in evs or []):
                seen = "the machine was reset\n" + describe(auto, mark)
            elif late:
                seen = describe(auto, mark)
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
    return raw.decode("latin-1"), seen


def run_86box_pty(box, img, out):
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
            if finished(text) is not None:
                break
        text = raw.decode("latin-1")
    finally:
        if fd is not None:
            os.close(fd)
        p.send_signal(signal.SIGKILL)       # 86Box ignores SIGTERM
        p.wait()
    return text, None


def run_86box(img, out):
    box = find_86box()
    run = run_86box_socket if has_automation(box) else run_86box_pty
    text, seen = run(box, img, out)
    with open(os.path.join(out, "serial.txt"), "w", newline="") as f:
        f.write(text)
    return text, seen


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    emu = os.environ.get("EMU", "86box")
    if emu not in ("86box", "qemu"):
        sys.exit("EMU must be 86box or qemu")
    if sys.argv[1] == "--boot":
        if len(sys.argv) != 4:
            sys.exit(__doc__)
        name = sys.argv[2]
        out = os.path.join(TOP, "obj", "tests", name)
        os.makedirs(out, exist_ok=True)
        img = os.path.join(out, name + ".img")
        mkfd(sys.argv[3], img, out)
    else:
        name, sources = sys.argv[1], sys.argv[2:]
        out = os.path.join(TOP, "obj", "tests", name)
        os.makedirs(out, exist_ok=True)
        img = build(name, sources, out)
    # The program's output, and what the emulator can say of a failure.
    text, seen = run_86box(img, out) if emu == "86box" else (run_qemu(img, out), None)
    sys.stdout.write(text.replace("\r", ""))
    sys.stdout.flush()
    if seen:
        print("\n" + seen, file=sys.stderr)
    status = finished(text)
    if status is None:
        sys.exit("\n%s: no result on %s (the limit is %d seconds)" % (name, emu, TIMEOUT))
    print("%s: program returned %d on %s" % (name, status, emu))
    sys.exit(0 if status == 0 else 1)


if __name__ == "__main__":
    main()
