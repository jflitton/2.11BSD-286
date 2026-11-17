#!/usr/bin/env python3
"""Fetch the third-party material listed in reference/MANIFEST.

usage: fetch-reference.py [--pin] [--list] [WORD ...]

With no WORD, fetches every entry that is missing or doesn't match its
checksum or commit.  With WORDs, only entries whose path contains one.

Each MANIFEST line is: KIND PATH ID URL

  file  download URL to reference/PATH; ID is its sha256
  zip   download URL into the directory reference/PATH and unpack it
        there; ID is the archive's sha256
  git   shallow checkout of URL in reference/PATH; ID is the commit

An ID of "-" means not pinned yet.  --pin fetches such entries and writes
the checksum or commit it got back into MANIFEST.

Everything fetched is third-party and stays out of git; see .gitignore.
"""
import hashlib
import os
import shutil
import subprocess
import sys
import zipfile

TOP = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(TOP, "reference")
MANIFEST = os.path.join(REF, "MANIFEST")


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def download(url, dest):
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    tmp = dest + ".part"
    r = subprocess.run(["curl", "-fL", "--retry", "2", "-sS", "-o", tmp, url])
    if r.returncode != 0:
        if os.path.exists(tmp):
            os.remove(tmp)
        return None
    return tmp


def fetch_file(dest, ident, url):
    """Returns (status, ident actually found)."""
    if os.path.isfile(dest) and ident != "-" and sha256(dest) == ident:
        return "ok", ident
    tmp = download(url, dest)
    if tmp is None:
        return "FAILED download", ident
    got = sha256(tmp)
    if ident != "-" and got != ident:
        os.rename(tmp, dest + ".mismatch")
        return "FAILED checksum (kept as .mismatch, got %s)" % got, ident
    os.rename(tmp, dest)
    return "fetched", got


def fetch_zip(dest, ident, url):
    archive = os.path.join(dest, os.path.basename(url))
    status, got = fetch_file(archive, ident, url)
    if status != "fetched":
        return status, got
    with zipfile.ZipFile(archive) as z:
        for name in z.namelist():
            if name.startswith("/") or ".." in name.split("/"):
                return "FAILED unsafe name in archive: %r" % name, ident
        z.extractall(dest)
    return status, got


def git(*args, cwd=None):
    return subprocess.run(("git",) + args, cwd=cwd, text=True,
                          capture_output=True)


def fetch_git(dest, ident, url):
    if os.path.isdir(os.path.join(dest, ".git")):
        head = git("rev-parse", "HEAD", cwd=dest).stdout.strip()
        if ident != "-" and head == ident:
            return "ok", ident
        shutil.rmtree(dest)
    os.makedirs(dest)
    git("init", "-q", cwd=dest)
    git("remote", "add", "origin", url, cwd=dest)
    r = git("fetch", "-q", "--depth", "1", "origin",
            "HEAD" if ident == "-" else ident, cwd=dest)
    if r.returncode != 0:
        return "FAILED fetch: " + r.stderr.strip()[-200:], ident
    r = git("-c", "advice.detachedHead=false", "checkout", "-q", "FETCH_HEAD",
            cwd=dest)
    if r.returncode != 0:
        return "FAILED checkout: " + r.stderr.strip()[-200:], ident
    return "fetched", git("rev-parse", "HEAD", cwd=dest).stdout.strip()


FETCH = {"file": fetch_file, "zip": fetch_zip, "git": fetch_git}


def main():
    args = sys.argv[1:]
    pin = "--pin" in args
    only_list = "--list" in args
    words = [a for a in args if not a.startswith("--")]

    lines = open(MANIFEST).read().split("\n")
    failed = 0
    changed = False
    for i, line in enumerate(lines):
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        if len(fields) != 4 or fields[0] not in FETCH:
            sys.exit("MANIFEST line %d: expected KIND PATH ID URL" % (i + 1))
        kind, path, ident, url = fields
        if words and not any(w in path for w in words):
            continue
        if only_list:
            print("%-4s %s" % (kind, path))
            continue
        if ident == "-" and not pin:
            print("%-40s not pinned (run with --pin)" % path)
            failed += 1
            continue
        status, got = FETCH[kind](os.path.join(REF, path), ident, url)
        print("%-40s %s" % (path, status))
        if status.startswith("FAILED"):
            failed += 1
        elif got != ident:
            lines[i] = "%s  %s  %s  %s" % (kind, path, got, url)
            changed = True
    if changed:
        with open(MANIFEST, "w") as f:
            f.write("\n".join(lines))
        print("MANIFEST updated with new checksums and commits")
    if failed:
        sys.exit("%d entries failed" % failed)


if __name__ == "__main__":
    main()
