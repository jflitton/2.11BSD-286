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
| 6 | tar of `/usr`: binaries, manual pages, `/usr/doc`, `/usr/include`, Ingres | what has no source in `usr/src`: `include/`, `dict/`, `doc/`, `ingres/`, `pub/` and `adm/`'s scripts |
| 7 | tar of `/usr/src/sys` | yes, as `usr/src/sys` |
| 8 | tar of the rest of `/usr/src` | yes, as `usr/src` |

`/usr/include` is not under `/usr/src` in 2.11BSD, which is why it comes
from file 6. Most of it is links into `/sys`, and `/sys` is itself a link to
`usr/src/sys`; the import adds that one link at the top so the others
resolve.

The first commit of `upstream` has the source trees: files 7 and 8, and
`/usr/include`. The second has the rest of `/usr` that has no source in
`usr/src`, from file 6:

| path | contents |
|------|----------|
| `usr/dict` | the word lists `words`, `web2`, `web2a` and `connectives`, and `refer`'s papers |
| `usr/doc` | the troff sources of the manuals |
| `usr/ingres` | INGRES: its source, documents and demonstration |
| `usr/pub` | `ascii`, `eqnchar` and `greek` |
| `usr/adm` | `daily`, `weekly` and `monthly`, the scripts `cron` runs |

Left on the tape: the PDP-11's programs and libraries, INGRES's `bin/` and
`lib/` among them; what is made from `usr/src` or from what is imported,
the manual's formatted pages, the games' files, `/usr/share`, `spell`'s
hashed lists in `/usr/dict` and `refer`'s indexes (`Ind.*`);
`/usr/dict/spellhist`, a link to `/dev/null`; the logs in `/usr/adm`; and
everything in the root dump (`/etc`, `/dev`, `/VERSION`, the `GENERIC`
kernel). Reading the root dump needs a reader for the old `dump` format,
which we don't have yet.

What git does not keep from the tape: file modes beyond the executable bit,
ownership, time stamps, and 29 empty directories (each library's
`profiled/`, where the Makefiles put profiled objects,
`usr.sbin/sendmail.MX/cf.named`, and INGRES's `data/base`). A build has to
create the `profiled/` directories.

## Checking it

```sh
tools/fetch-reference.py PL500
tools/import-upstream.sh                  # builds the branch upstream-check
git diff --stat upstream-check~1 "$(git rev-list --reverse upstream | sed -n 1p)"
git diff --stat upstream-check "$(git rev-list --reverse upstream | sed -n 2p)"
git branch -D upstream-check
```

No output from either diff means the two import commits match the tape.

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
   rebuild or install things don't apply. Changes to files that `upstream`
   does not hold have nowhere to go; list them in the commit message.
4. Commit as "2.11BSD patch NNN", then merge `upstream` into `main`.
