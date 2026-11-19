# ASND ucode for Dolphin 5.0-19870 HLE

Vendored so Backyard Cornhole boots with no DSP HLE warning on stock
Dolphin 5.0-19870 (Kinhank Android build, commit `032c77b`, August 2023).

## Why

libogc 3.2.1's `libasnd.a` ships the March 2024 ASND ucode (ector CRC
`0x5dbf8bf1`). Dolphin only added that CRC in PR #12665 (merged 31 March
2024, first in 5.0-21460), so 5.0-19870 treats it as unknown and pops
"Unknown ucode (CRC = 5dbf8bf1) - forcing AXWii."

The last official pre-2024 libasnd is in libogc 2.5.0-2. Its ucode is
the June 2020 mixer:

- unpadded 0x0606 bytes: ector CRC `0xdbbeeb61` (`ASndUCode::HASH_2020`)
- padded 0x0620 bytes: ector CRC `0xbad876ef` (`ASndUCode::HASH_2020_PAD`)

Both CRCs are in Dolphin 5.0-19870's `UCodes.cpp` / `ASnd.h`. Linking
the entire 2.5.0 `libasnd.a` against libogc 3.2.1 fails (`DCFlushRange`,
`DCInvalidateRange`, `gettime` are no longer exported). The ASND C API
is otherwise unchanged (same 27 `ASND_*` symbols).

## What we ship

`lib/wii/libasnd.a` is spliced from two official devkitPro packages:

| Member | Source |
|---|---|
| `asndlib.o` | `libogc-3.2.1-1-any.pkg.tar.zst` (`lib/wii/libasnd.a`) |
| `asnd_dsp_mixer_bin.o` | `libogc-2.5.0-2-any.pkg.tar.zst` (`asnd_dsp_mixer.bin.o`, renamed to the 3.2.1 member name) |

Also stored: the official 2.5.0 mixer object and the raw 0x0606-byte
ucode (`asnd_dsp_mixer.bin`). The Makefile links
`lib/wii/libasnd.a` ahead of libogc 3.2.1. `make` runs
`tools/check-ucode.c` on `boot.dol` and fails if CRC `0x5dbf8bf1` is
present or a 5.0-19870-recognized ASND CRC is missing.

The 2020 ucode has the long-standing libogc stereo left/right swap that
the 2024 ucode fixed. Gameplay is unchanged; stereo MP3s may have
swapped channels compared with 0.1.0 on hardware or on newer Dolphin.

## Rebuild the archive

```sh
powerpc-eabi-ar x /opt/devkitpro/libogc/lib/wii/libasnd.a
cp asnd_dsp_mixer.bin.o asnd_dsp_mixer_bin.o   # this directory's official 2.5.0 object
powerpc-eabi-ar rcs lib/wii/libasnd.a asnd_dsp_mixer_bin.o asndlib.o
```

## Checksums

| File | SHA-256 |
|---|---|
| `libogc-3.2.1-1-any.pkg.tar.zst` (pkg.devkitpro.org) | `2ece827018582723e040247b815e142551c034e12a8808a889923a5481eb7a4b` |
| `libogc-2.5.0-2-any.pkg.tar.zst` (pkg.devkitpro.org) | `f05ae0bea740f1a200cfc5ff09c915ccc8f240b5a0b3cca0c6612d9dc498ae59` |
| Official 3.2.1 `lib/wii/libasnd.a` | `25a73a09a2f6ad44ee562c69966eb1293df98d1ada97322807affe25cc726a2f` |
| Official 2.5.0 `asnd_dsp_mixer.bin.o` | `e595835c7a63d37ae1e2f50fb3941fdb353e494123a748f6ecbf473078a0f330` |
| `asnd_dsp_mixer.bin` (0x0606 bytes from that object) | `7699c25e2544d2d9defee29ae1ebb82a27d58c020087e892e24c132bfbd433a0` |
| Spliced `lib/wii/libasnd.a` | `f7da42b464bd1e21790fc387b02d66f73dd05ad9c2c029bfa8583419d5dc55a8` |

License: libogc custom (zlib-style). Do not replace the mixer object
with libogc 2.6.0 or later: those ship CRC `0x5dbf8bf1` again.
