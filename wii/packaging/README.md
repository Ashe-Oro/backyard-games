# Wii packaging and smoke test

Build tools: devkitPPC + libogc and `elf2dol` under `/opt/devkitpro`, Python 3
(standard library only), GNU make, and Wiimms ISO Tools (`wit`). On Debian:
`sudo apt-get install wit dolphin-emu xvfb ffmpeg ripgrep`. The tested versions are
recorded in the smoke report; this is a developer pipeline, not a pinned SDK image.

From the repository root:

```sh
# Independent video + WPAD stub; no game source dependencies.
make -C wii/packaging
# Package the real game when its ELF is available; use a separate output directory.
make -C wii/packaging ELF=/absolute/path/game.elf OUT=/workspace/uploads/wii-game
# Boot an existing ISO for ten seconds and capture an X11 screenshot and logs.
wii/packaging/smoke.sh /workspace/uploads/wii-stub/BackyardCornhole-BYCEAO.iso
```

Default output: `/workspace/uploads/wii-stub/`. Outputs include `boot.dol`,
`apps/backyardcornhole/{boot.dol,meta.xml,icon.png}`, the USA/NTSC `BYCEAO` ISO,
`GameSettings/BYCEAO.ini`, `SHA256SUMS`, and a release zip containing all those
release files and installation instructions. The input ELF, generated apploader
and `iso-root/` remain as build intermediates outside Git. The stub explicitly
selects NTSC 480p or 480i; a supplied game ELF must also select NTSC/60 Hz.
Metadata uses `VERSION` from `wii/Makefile`. The icon is an original generated
128×48 geometric placeholder. No downloaded artwork or retail disc dump is used.

`package.py` generates boot/BI2 headers and builds a Wii data partition with
WIT, IOS58 and USA region. It extracts the completed ISO's `sys/main.dol` and
checks byte identity with the HBC `boot.dol` before making the release zip.
ISO composition follows [WIT's FST specification](https://wit.wiimm.de/info/composing.html).
`apploader.c` is original CC0 code implementing the
[Dolphin apploader ABI](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/Boot/Boot_BS2Emu.cpp).
It loads the DOL sections and clears BSS. It is a minimal homebrew loader; it
does not load an FST for disc filesystem access. This ELF-only packaging target
expects assets linked into the executable. Real Wii disc loading is untested;
use the HBC package on a softmodded Wii. WIT generates partition metadata and
uses its own default certificate chain; no retail files are supplied to it.

Smoke requires `dolphin-emu-nogui` with X11 support (or `dolphin-emu`), `Xvfb`,
`ffmpeg`, and Python 3. It creates an isolated Dolphin user directory under
`/workspace/docs/cornhole/wii-port/pr/smoke/` and uses Mesa software OpenGL.
Real Wiimotes are disabled only in that isolated profile, so the smoke does not
scan Bluetooth hardware. Release settings still enable both real Wiimotes.
`SMOKE_OUT`, `SMOKE_SECONDS` (default 10), and `DOLPHIN` can override defaults.
The script returns failure if Dolphin exits early, capture fails, or the log
contains a known boot failure. A screenshot still needs visual inspection:
process survival alone does not establish that a game rendered correctly.

No ADB or hardware operations are part of building or smoke testing.

Verified on this Linux box on 2026-11-15 with devkitPPC GCC 16.2.0, libogc
3.2.1, WIT 3.01a (Debian 3.01a-5), and Dolphin 2503. The ten-second ISO smoke
displayed the title, frame counter 563 and approximately 60 FPS. Evidence and
the detailed report are in `/workspace/docs/cornhole/wii-port/pr/smoke/`.
The supplied-ELF path was also checked using the stub ELF in a separate output
directory. Android Vulkan and real Wii Remotes remain hardware checks for Retro.
