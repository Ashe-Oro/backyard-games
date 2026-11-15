# Backyard Cornhole — Wii

Native Wii homebrew port of Backyard Cornhole Arcade **1.0.3** (C, devkitPPC + libogc, GX, WPAD).

**Status: scaffold only.** Nothing here builds or plays yet. Phase P0 (toolchain + "hello Wiimote" + ISO packaging) is the next step: see [IMPL_PLAN.md](../docs/wii-port/IMPL_PLAN.md).

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
  Makefile                 VERSION source; build targets are P0 deliverables (stubs today)
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

## Docs

[SPEC.md](../docs/wii-port/SPEC.md) · [IMPL_PLAN.md](../docs/wii-port/IMPL_PLAN.md) · [WIIMOTE_ARCADE_MAPPING.md](../docs/wii-port/WIIMOTE_ARCADE_MAPPING.md) · [REPO_LAYOUT.md](../docs/wii-port/REPO_LAYOUT.md)
