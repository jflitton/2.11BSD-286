#!/usr/bin/env python3
"""Make the table of a run of bench(1) under the harness, and check it.

usage: report.py [-b RESULTS] DIRECTORY [TEST ...]

DIRECTORY has serial.txt and marks.txt, as tests/harness/runtest.py --mark
"@@" leaves them when the machine ran "bench -m".  Each test's line gives
the time it took by the system's own clock; the two marks it wrote, one
as its work began and one as it ended, give the same by the emulator's.
The table has both, and goes to results.txt in DIRECTORY as well.  It is
an error if a TEST named, or with none named any of bench's, has no line;
or if the two times of one differ by more than a twentieth and 50 ms,
which a system that loses clock interrupts would show.

With -b, RESULTS is the results.txt of another run, and the table says
of each test how much quicker or slower it is than it was then.
"""
import os
import re
import sys

LINE = re.compile(r"^(\w+) +@@@@ *(\d+\.\d+) +(\d+\.\d+) +(\d+\.\d+) +(\d+) +(\d+) (us|K/s)\s*$")
ALL = ["sieve", "long", "getpid", "write", "stat", "pipe", "pipeio", "fork",
       "exec", "shell", "creat", "fswrite", "fsread", "pty", "tcp", "tcprr",
       "telkey", "telcmd"]


def base(path):
    """What each test came to in another run: {name: (each, unit)}."""
    was = {}
    for line in open(path):
        f = line.split()
        if len(f) >= 7 and f[6] in ("us", "K/s") and f[5].isdigit():
            was[f[0]] = (int(f[5]), f[6])
    return was


def main():
    args = sys.argv[1:]
    was = None
    if len(args) > 1 and args[0] == "-b":
        was = base(args[1])
        del args[:2]
    if not args:
        sys.exit(__doc__)
    out, want = args[0], args[1:] or ALL
    rows = {}
    order = []
    for line in open(os.path.join(out, "serial.txt"), encoding="latin-1", newline=""):
        m = LINE.match(line.replace("\r", "").rstrip("\n"))
        if m:
            rows[m.group(1)] = m.groups()[1:]
            order.append(m.group(1))
    marks = []
    for line in open(os.path.join(out, "marks.txt")):
        f = line.split(None, 2)
        marks.append((int(f[0]), f[2].split()[0] if len(f) > 2 and f[2].split() else "?"))
    outside = {}
    for (t0, name), (t1, _n) in zip(marks[0::2], marks[1::2]):
        outside[name] = t1 - t0
    head = "%-8s %7s %7s %7s %7s %9s     %8s" % (
        "test", "real", "user", "sys", "count", "each", "outside")
    if was is not None:
        head += "  %9s %7s" % ("was", "change")
    table = [head]
    bad = []
    for name in order:
        real, user, system, count, each, unit = rows[name]
        if name not in outside:
            bad.append("%s: no marks" % name)
            continue
        ms = outside[name]
        row = "%-8s %7s %7s %7s %7s %9s %-3s %8.3f" % (
            name, real, user, system, count, each, unit, ms / 1000)
        if was is not None and name in was and was[name][1] == unit and was[name][0]:
            old = was[name][0]
            # Quicker is fewer microseconds each, or more kilobytes a second.
            gain = (old / int(each) if unit == "us" else int(each) / old) if int(each) else 0
            row += "  %9d %+6.1f%%" % (old, (gain - 1) * 100)
        table.append(row)
        if abs(ms - float(real) * 1000) > 50 + ms / 20:
            bad.append("%s: %s s by the system's clock, %.3f s by the emulator's"
                       % (name, real, ms / 1000))
    for name in want:
        if name not in rows:
            bad.append("%s: no result" % name)
    text = "\n".join(table) + "\n"
    with open(os.path.join(out, "results.txt"), "w") as f:
        f.write(text)
    sys.stdout.write(text)
    if bad:
        sys.exit("bench: " + "\nbench: ".join(bad))


if __name__ == "__main__":
    main()
