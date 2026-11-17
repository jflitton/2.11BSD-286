# 2.11BSD for the 80286

A port of 2.11BSD, at its current patch level, to the 286 in protected mode.
The aim is a system that maps cleanly onto the 286 and runs larger programs
than the PDP-11 one could, by giving a process more than one data segment.

Target machine: an IBM PS/2 Model 30 286, which is an AT-compatible, with an
80287, an Adaptec AHA-1542CP SCSI adapter and an NE2000-compatible network
card. The 287 is required.

This repository must live on a case-sensitive file system: the 2.11BSD tree
has file names that differ only by case.

## Map

| path | what | in git |
|------|------|--------|
| `usr/src/` | the 2.11BSD source tree, at its real path | yes |
| `usr/src/sys/` | the kernel; `sys/`, `h/`, `net*/` are machine-independent, `pdp*/` and `mdec/` are the PDP-11 | yes |
| `usr/include/` | the system's headers; mostly links into `usr/src/sys` | yes |
| `sys` | link to `usr/src/sys`, as on the running system; the links in `usr/include` go through it | yes |
| `tools/` | host-side tools (below) | yes |
| `docs/` | our notes | yes |
| `reference/MANIFEST` | what third-party material we use, where it comes from, checksums | yes |
| `reference/upstream/` | the install tape, patches, setup guide | no |
| `reference/manuals/` | Intel, IBM, Adaptec and DEC manuals; Ralf Brown's Interrupt List | no |
| `reference/prior-art/` | MINIX, RetroBSD, DiscoBSD, ELKS, Coherent sources | no |
| `obj/`, `images/`, `vm/` | build output, disk images, emulator state (none yet) | no |

Where the 286 code will go, beside its PDP-11 counterpart in each case:

| new | beside | holds |
|-----|--------|-------|
| `usr/src/sys/i286/` | `pdp/` | startup, traps, segments and tasks, 287, copy routines |
| `usr/src/sys/i286at/` | `pdpuba/` | AT-bus drivers: SCSI, console, serial, clock |
| `usr/src/sys/i286if/` | `pdpif/` | network interfaces |
| `usr/src/sys/i286stand/` | `pdpstand/`, `mdec/` | boot blocks and standalone programs |
| `usr/src/sys/PS2_30/` | `GENERIC/` | the kernel configuration |
| `usr/src/lib/libc/i286/` | `lib/libc/pdp/` | libc's machine-dependent part |

The kernel picks its machine through the link `usr/src/sys/machine`, which
points at `pdp` upstream.

## Branches

- `upstream` is 2.11BSD exactly as distributed, one commit per patch level.
  Nothing of ours goes on it.
- `main` branches from it. `git diff upstream main -- usr sys` is the port.

How `upstream` was made and how to take a new patch: [docs/upstream.md](docs/upstream.md).

## Reference material

```sh
tools/fetch-reference.py            # fetch whatever is missing
tools/fetch-reference.py --list     # what there is
tools/fetch-reference.py manuals    # only entries whose path contains "manuals"
```

`reference/MANIFEST` says what each item is for. To add one, add a line with
`-` as its checksum and run `tools/fetch-reference.py --pin`.

2.11BSD's own papers are the documentation for the code we keep. They are
on the tape rather than in git; after a fetch,
`tar xf reference/upstream/PL500/tape/file6 -C reference/upstream/PL500/usr doc`
unpacks them as troff source. The ones that matter here:

| under `reference/upstream/PL500/usr/doc/` | what |
|------|------|
| `2.10/kchanges.2.11`, `kchanges.2.10`, `setup.2.11` | what the 2.11BSD kernel changed, and how the system is set up |
| `2.10/ovpap`, `ovtutorial` | how overlays work, in the kernel and in user programs |
| `2.10/assembler` | the PDP-11 assembler `sys/pdp/*.s` is written in |
| `smm/02.config` | kernel configuration |
| `smm/14.fastfs`, `smm/15.net` | the file system and the networking implementation |
| `smm/19.porttour` | a tour of the portable C compiler |

The AHA-1542CP is the Plug and Play member of the 1540C series. Its guide
says it also runs in machines without Plug and Play, at the settings on its
switches and in SCSISelect (defaults: port 330h, IRQ 11, DMA 5), so the
1540C technical reference describes what the driver talks to.

Not found: the *IBM PS/2 Model 30 286 Technical Reference*. No copy has
turned up on line. (Held's *IBM PS/2 User's Reference Manual*, 1989, is a
DOS and OS/2 user's guide with nothing on ports or interrupts; not kept.)

## Tools

| tool | does |
|------|------|
| `tools/fetch-reference.py` | fetches `reference/` from the manifest |
| `tools/untap.py` | splits a SimH `.tap` tape image into its files |
| `tools/tar2git.py` | commits tar archives to a branch without unpacking them |
| `tools/import-upstream.sh` | rebuilds the `upstream` import from the tape, to check it |

## Status (2026-11-17)

- `upstream` holds patch level 500, checked file by file against the tape.
- Reference material is listed and fetched.
- Nothing is ported yet. The first decision is the C compiler: the tree's
  own (`usr/src/lib/ccom`) generates PDP-11 code only, and more than one
  data segment per process means far pointers.
