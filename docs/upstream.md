# The upstream branch

`upstream` holds 2.11BSD as Steven Schultz distributes it, with no changes
of ours, so that `git diff upstream main` shows the port and nothing else.

## Where it comes from

The patch level 500 install tape ("35th Anniversary Edition", November
2026), `PL500.tap.bz2`, from
<https://www.tuhs.org/Archive/Distributions/UCB/2BSD/2.11BSD/Patches/>.
Patches are published one file per patch in the same directory. The
maintainer's own site is `ftp.2bsd.com`.

The tape is a SimH `.tap` image of nine tape files:

| file | contents | in `upstream` |
|------|----------|---------------|
| 0 | boot blocks | no |
| 1–4 | standalone `disklabel`, `mkfs`, `restor`, `icheck` (PDP-11 binaries) | no |
| 5 | the root file system, as a `dump` | no |
| 6 | tar of `/usr`: binaries, manual pages, `/usr/doc`, `/usr/include`, Ingres | only `include/`, as `usr/include` |
| 7 | tar of `/usr/src/sys` | yes, as `usr/src/sys` |
| 8 | tar of the rest of `/usr/src` | yes, as `usr/src` |

`/usr/include` is not under `/usr/src` in 2.11BSD, which is why it comes
from file 6. Most of it is links into `/sys`, and `/sys` is itself a link to
`usr/src/sys`; the import adds that one link at the top so the others
resolve.

Not imported, and still only on the tape: `/usr/doc` (the troff sources of
the manuals), `/usr/ingres`, the parts of `/usr/share` that are not built
from `/usr/src/share`, and everything in the root dump (`/etc`, `/dev`,
`/VERSION`, the `GENERIC` kernel). The port will need some of these when it
builds a root file system. Reading the root dump needs a reader for the old
`dump` format, which we don't have yet.

What git does not keep from the tape: file modes beyond the executable bit,
ownership, time stamps, and 27 empty directories (each library's
`profiled/`, where the Makefiles put profiled objects, and
`usr.sbin/sendmail.MX/cf.named`). A build has to create the `profiled/`
directories.

## Checking it

```sh
tools/fetch-reference.py PL500
tools/import-upstream.sh                  # builds the branch upstream-check
git diff --stat upstream-check "$(git rev-list --max-parents=0 upstream)"
git branch -D upstream-check
```

No output from the diff means the first `upstream` commit matches the tape.

## Taking a new patch

This has not been done yet; patch 501 will be the first.

1. Add the patch to `reference/MANIFEST` as `upstream/patches/NNN` and fetch
   it.
2. Read it. A 2.11BSD patch is a mail message: a description, then
   instructions, then some mix of context diffs, shell archives and
   uuencoded tars. The instructions assume a running system and paths from
   `/`.
3. Check out `upstream` in a separate work tree
   (`git worktree add ../2.11BSD-upstream upstream`) and carry the
   instructions out there by hand, with the work tree as `/`. Steps that
   rebuild or install things don't apply. Changes to files outside
   `usr/src` and `usr/include` have nowhere to go; list them in the commit
   message.
4. Commit as "2.11BSD patch NNN", then merge `upstream` into `main`.
