# Backyard Cornhole — Wii

Native Wii homebrew port of Backyard Cornhole Arcade **1.0.3** (C, devkitPPC + libogc, GX, WPAD).

**Status: input layer with host tests and an independent packaging smoke stub; the game is not playable yet.** The platform-free swing detector, player-slot routing and WPAD adapter are implemented. See [INPUT.md](INPUT.md) for game integration and provisional tuning. Packaging can now accept a built game ELF; see [packaging/README.md](packaging/README.md).

| Target | Package |
|---|---|
| Real softmodded Wii | Homebrew Channel: `sd:/apps/backyardcornhole/{boot.dol,meta.xml,icon.png}` |
| Kinhank X5 Pro (Dolphin Android 5.0-19870) | `BackyardCornhole-BYCEAO.iso` + `GameSettings/BYCEAO.ini` (real Wiimotes via DolphinBar Mode 4). Retro installs and tests |
| Dev loop | Dolphin desktop/Android, same binary |

Standing rules:
- **Port from the 1.0.3 snapshot only:** `arcade/cabinet-snapshots/1.0.3/resources-app/dist/site/`. The top-level `arcade/trackball.mjs` and `arcade/controller.js` are retired distance-power code.
- **Toss power = swing speed** (accelerometer peak over a short window). Never stroke distance.
- **NTSC / 60 Hz only.** No PAL or EURGB60 branch.
- One game binary: HBC `boot.dol` is byte-identical to the ISO's `sys/main.dol`.
- Game ID `BYCEAO` (pending a GameTDB collision check).
- Shared rules data lives in [`shared/cornhole/`](../shared/cornhole/). The Wii baseline is `overrides/arcade-1.0.3.json`.

## Layout

```
wii/
  Makefile                 VERSION source and host-test
  packaging/               ELF → DOL, HBC, ISO, release zip; independent stub and Dolphin smoke
  TOOLCHAIN.md             devkitPPC / libogc / WIT notes and pinning rules
  CHANGELOG.md
  docker/                  pinned build image (P0)
  source/core/             platform-free C: physics, rules, wind, CPU, swing detector (host-testable)
  source/wii/              GX renderer, WPAD input, ASND audio, libfat saves, main loop
  tools/                   Node scripts: scene exporter, WIN baker, golden-vector oracle
  data/                    generated assets (scene, textures, font atlas, WIN table)
  hbc/apps/backyardcornhole/  meta.xml, icon.png; boot.dol is a build output
  dolphin/                 GameSettings/BYCEAO.ini, iso-root/ (generated), INSTALL-KINHANK.md, PROVENANCE.md
  tests/                   host tests: C core vs golden vectors
```

## Build

With devkitPPC + libogc installed at `/opt/devkitpro`, plus Python 3 and `wit`:

```sh
make -C wii/packaging
# When the game's ELF is built:
make -C wii/packaging ELF=/absolute/path/game.elf OUT=/workspace/uploads/wii-game
```

The default builds the video/WPAD stub and writes artifacts to
`/workspace/uploads/wii-stub/`, outside Git. It produces HBC files, the
`BackyardCornhole-BYCEAO.iso` USA/NTSC disc, real-Wiimote settings and a release
zip. The ISO's DOL is checked against the HBC DOL. A supplied game must select
NTSC/60 Hz and embed its assets. No game sources are built by this target.

With desktop Dolphin, Xvfb, ffmpeg and ripgrep installed:

```sh
wii/packaging/smoke.sh /workspace/uploads/wii-stub/BackyardCornhole-BYCEAO.iso
```

The ten-second smoke records a screenshot and logs under
`/workspace/docs/cornhole/wii-port/pr/smoke/` using an isolated profile and
software OpenGL. See [packaging/README.md](packaging/README.md) for dependencies,
loader provenance and limits, and [KINHANK_INSTALL.md](packaging/KINHANK_INSTALL.md)
for Retro's manual Android installation steps. The top-level `wii/Makefile`
platform targets remain scaffold placeholders; use `wii/packaging` above.

## Docs

Host input tests: `make -C wii host-test` from the repo root. Without make: `sh wii/tests/run-input-tests.sh` (native C11 compiler and libm only).

[SPEC.md](../docs/wii-port/SPEC.md) · [IMPL_PLAN.md](../docs/wii-port/IMPL_PLAN.md) · [WIIMOTE_ARCADE_MAPPING.md](../docs/wii-port/WIIMOTE_ARCADE_MAPPING.md) · [REPO_LAYOUT.md](../docs/wii-port/REPO_LAYOUT.md)
