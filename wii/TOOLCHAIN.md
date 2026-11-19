# Wii toolchain notes

Status: **installed and used** on the dev box (2026-11-15). `make`, `make host-test`, `make iso` and `make release`
all run there.

| Piece | Version used | Pin |
|---|---|---|
| Compiler | devkitPPC **r51** (`powerpc-eabi-gcc` 16.2.0, binutils 2.47) | `docker/Dockerfile`: `devkitpro/devkitppc:20261102` |
| Platform library | **libogc 3.2.1** (GX, VIDEO, WPAD, SYS), **libfat-ogc 2.1.0** | same image; libogc2 is a separate fork — not used |
| Tools | `elf2dol` (gamecube-tools 1.0.7), `wiiload` 0.5.3, general-tools 1.4.4 | same image |
| 2D | Own ortho quads + the libogc console font (`console_font_8x16`, CP437) | GRRLIB not used (not in the devkitPro repos; SPEC allows raw GX) |
| Disc image | **Wiimms ISO Tools 3.05a r8638** | 3.01a can't compose without `sys/bi2.bin`; Makefile prefers `/opt/wit/bin/wit` |
| Host tests | system `cc` (gcc), Node 20+ | — |

Audio: MP3 playback still uses libogc ASND + libmad. The 3.2.1 `libasnd.a` is **not** linked; the Makefile uses a vendored official `libasnd.a` from libogc 2.5.0-2 so the embedded ucode CRC is `0xdbbeeb61` (Dolphin 5.0-19870 HLE), not `0x5dbf8bf1`. See `third_party/asnd-libogc-2.5.0/PROVENANCE.md`.

How the dev box got them (no Docker there, and its Debian `make`/apt mirrors were down):
- Package databases and archives fetched from `pkg.devkitpro.org` (`dkp-libs.db`, `linux/x86_64/dkp-linux.db`), each archive's
  SHA-256 checked against the database entry, unpacked into `/opt/devkitpro`.
- GNU make 4.4.1 built from the ftp.gnu.org tarball.
- WIT from wit.wiimm.de (SHA-256 above in `dolphin/PROVENANCE.md`) into `/opt/wit`.

Build outputs:
- `make` → `build/boot.elf` → `build/boot.dol`, copied to `hbc/apps/backyardcornhole/boot.dol` (git-ignored).
- `make iso` → `dolphin/iso-root/` (`sys/boot.bin` generated, our own `sys/apploader.img`, `sys/main.dol` = boot.dol,
  `files/`) → `dist/BackyardCornhole-BYCEAO.iso` (truncated) + `dist/GameSettings/BYCEAO.ini`; fails unless the ISO's
  `main.dol` hashes equal to `boot.dol`.
- `make release` → `dist/`: HBC folder/zip, ISO, `GameSettings/BYCEAO.ini`, `KINHANK_INSTALL.md`, full release ZIP and `SHA256SUMS`. Use `make -C wii release OUT=/workspace/uploads/wii-game` from the repo root; Python 3.11+ stages the release.
- `make soak` → `build/soak/boot-soak.dol`: dev-only auto-throw soak with OSReport logging. Never shipped.

Video: NTSC only. `TVNtsc480Prog` when progressive output is available, else `TVNtsc480IntDf`; 16:9 when the console
is set to widescreen. There is no PAL/EURGB60 branch.

Never bundle Nintendo retail files. Install on the dev box only — never on the arcade cabinet or the Kinhank.
