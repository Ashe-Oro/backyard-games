# Wii toolchain notes

Status: not installed or pinned yet. P0 does that (IMPL_PLAN.md task 1).

| Piece | Choice | Pin rule |
|---|---|---|
| Compiler | devkitPro **devkitPPC** | Exact `devkitpro/devkitppc:<tag>` in `docker/Dockerfile`; record the tag in this file |
| Platform library | **libogc** v3.2.x (WPAD, GX, ASND, libfat) | Record the exact version. libogc2 is a separate fork: don't mix |
| 2D (optional) | Own ortho batcher + 8×8 font; GRRLIB 4.6.x only if it builds against the pinned libogc | — |
| Disc image | **WIT** (Wiimms ISO Tools) composes the ISO from `dolphin/iso-root/` | Exact version and command line in the Dockerfile and Makefile |
| Host tests | Host gcc/clang builds `source/core/` for `make host-test` | In the same image |
| Tools | Node 20+, `npm ci --prefix arcade` (provides `three@0.170.0` for the scene exporter) | Lockfile in `arcade/` |

Build output contract (P0):
- `make` → `boot.elf` → `elf2dol` → `boot.dol`
- `make iso` → complete tree in `dolphin/iso-root/` (`sys/boot.bin` generated with ID `BYCEAO` and USA region, a **redistributable** `sys/apploader.img` with `dolphin/PROVENANCE.md`, `sys/main.dol` = `boot.dol`, `files/`) → `BackyardCornhole-BYCEAO.iso`
- `make release` → `dist/`: HBC zip, ISO, `BYCEAO.ini`, `INSTALL-KINHANK.md`, licenses, `SHA256SUMS`; checks that `main.dol` in the ISO is byte-identical to `boot.dol` and that every version source agrees

Video: NTSC only. `TVNtsc480Prog` when progressive output is available, else `TVNtsc480IntDf`.

Never bundle Nintendo retail files (no retail apploader, no copied `boot.bin`).
Install on the dev box only. Never on the arcade cabinet or the Kinhank.
