#!/usr/bin/env python3
"""usage: check.py VE CHECKS [NAME ...]
       check.py --screen FILE

Run the checks of ve in the file CHECKS (its start says how they are
written) with the editor VE, each in an empty directory of its own, and
say which did not write what they are to; with names, those alone.

With --screen, show the screen a terminal would have after what is in
FILE was written to it, but for what ve does to the last row as it
leaves: VEROWS rows of VECOLS columns, each as a line, what stands out
between { and }, and then where the cursor is.  The terminal is the one
the checks tell ve it has (TERMCAP below).
"""
import os, re, shutil, subprocess, sys, tempfile

TERMCAP = (r"vt|the terminal of ve's tests:co#80:li#24:cm=\E[%i%d;%dH:"
           r"ce=\E[K:cl=\E[H\E[2J:so=\E[7m:se=\E[m:cs=\E[%i%d;%dr:"
           r"sf=^J:sr=\EM:")

PRELUDE = r'''
k() { keys=$1; shift; printf "$keys" | "$VE" "$@" >/dev/null 2>&1; }
s() { keys=$1; shift; printf "$keys" | "$VE" "$@" >.screen 2>&1
      "$PYTHON" -I "$CHECK" --screen .screen; rm -f .screen; }
'''


def screen(path):
    rows = int(os.environ.get("VEROWS", 24))
    cols = int(os.environ.get("VECOLS", 80))
    s = open(path, "rb").read().decode("latin-1")
    # not what ve does to the last row as it leaves
    s = re.sub(r"(\x1b\[m)?(\x1b\[%d;1H)?\x1b\[K$" % rows, "", s)
    scr = [[(" ", 0)] * cols for _ in range(rows)]
    r = c = attr = 0
    top, bot = 0, rows - 1

    def up(t, b):
        del scr[t]
        scr.insert(b, [(" ", 0)] * cols)

    def down(t, b):
        del scr[b]
        scr.insert(t, [(" ", 0)] * cols)

    i = 0
    while i < len(s):
        ch = s[i]
        i += 1
        if ch == "\x1b":
            m = re.match(r"\[([0-9;]*)([A-Za-z])", s[i:])
            if m:
                i += m.end()
                a = [int(x) if x else 0 for x in m.group(1).split(";")]
                k = m.group(2)
                if k == "H":
                    r = max(a[0], 1) - 1
                    c = (max(a[1], 1) if len(a) > 1 else 1) - 1
                elif k == "J":
                    scr[:] = [[(" ", 0)] * cols for _ in range(rows)]
                elif k == "K":
                    scr[r][c:] = [(" ", 0)] * (cols - c)
                elif k == "m":
                    attr = 1 if a[0] == 7 else 0
                elif k == "r":
                    top, bot = max(a[0], 1) - 1, (a[1] if len(a) > 1 else rows) - 1
                    r = c = 0
                else:
                    sys.exit("screen: ESC [ %s %s is not known" % (m.group(1), k))
            elif s[i:i + 1] == "M":
                i += 1
                if r == top:
                    down(top, bot)
                else:
                    r = max(0, r - 1)
            else:
                sys.exit("screen: ESC %r is not known" % s[i:i + 1])
        elif ch == "\r":
            c = 0
        elif ch == "\n":
            if r == bot:
                up(top, bot)
            else:
                r = min(rows - 1, r + 1)
        elif ch == "\a":
            pass
        elif ch < " ":
            sys.exit("screen: character %d is not known" % ord(ch))
        else:
            if c >= cols or r >= rows:
                sys.exit("screen: written past the edge, row %d" % (r + 1))
            scr[r][c] = (ch, attr)
            c += 1
    for line in scr:
        t, on = "", 0
        for ch, a in line:
            if a != on:
                t += "{" if a else "}"
                on = a
            t += ch
        if on:
            t += "}"
        print(t.rstrip())
    print("cursor %d,%d" % (r + 1, c + 1))


def main():
    if sys.argv[1] == "--screen":
        return screen(sys.argv[2])
    ve, checks, only = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3:]
    files, tests = {}, []
    cur = None
    for line in open(checks).read().split("\n")[:-1]:
        if line.startswith("=file "):
            cur = files.setdefault(line[6:], [])
        elif line.startswith("== "):
            tests.append((line[3:], [], []))
            cur = tests[-1][1]
        elif line == "--" and tests and cur is tests[-1][1]:
            cur = tests[-1][2]
        elif cur is None:
            if not line.startswith("#"):
                sys.exit("%s: a line before any file or check" % checks)
        else:
            cur.append(line)
    env = dict(os.environ, VE=ve, TERM="vt", TERMCAP=TERMCAP,
               PYTHON=sys.executable, CHECK=os.path.abspath(__file__))
    for v in ("VEROWS", "VECOLS", "SHELL"):
        env.pop(v, None)
    failed = ran = 0
    for name, cmds, want in tests:
        if only and name not in only:
            continue
        ran += 1
        d = tempfile.mkdtemp(prefix="ve.")
        try:
            for f, text in files.items():
                with open(os.path.join(d, f), "w") as o:
                    o.write("".join(t + "\n" for t in text))
            p = subprocess.run(["sh", "-c", PRELUDE + "\n".join(cmds)], cwd=d,
                               env=dict(env, HOME=d), stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               timeout=60)
            got = p.stdout.decode("latin-1").split("\n")
            if got and got[-1] == "":
                got.pop()
        except subprocess.TimeoutExpired:
            got = ["(it did not end)"]
        finally:
            shutil.rmtree(d, ignore_errors=True)
        if got != want:
            failed += 1
            print("%s: FAILED" % name)
            for t in want:
                print("    wanted |%s" % t)
            for t in got:
                print("    got    |%s" % t)
    print("ve: %d checks, %d failed" % (ran, failed))
    return 1 if failed else 0


sys.exit(main())
