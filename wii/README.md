# Backyard Cornhole — Wii

Native Wii homebrew port of Backyard Cornhole Arcade **1.0.3** (C, devkitPPC + libogc, raw GX, WPAD).

**Status: playable build (0.1.0, unreleased).** Practice, 2 Player and the 8-rival Tournament run on a GX
renderer; physics, scoring and the tournament CPU match the pinned 1.0.3 game on the host golden tests.
Boots in Dolphin from the ISO (game ID `BYCEAO`) and as a bare DOL. Swing feel and hardware timing are
not validated yet: that needs real Wii Remotes on a Wii, desktop Dolphin and the Kinhank (IMPL_PLAN P1).

| Target | Package |
|---|---|
| Real softmodded Wii | Homebrew Channel: `sd:/apps/backyardcornhole/{boot.dol,meta.xml,icon.png}` |
| Kinhank X5 Pro (Dolphin Android 5.0-19870) | `BackyardCornhole-BYCEAO.iso` + `GameSettings/BYCEAO.ini` (real Wiimotes via DolphinBar Mode 4). See `dolphin/INSTALL-KINHANK.md` |
| Dev loop | Dolphin desktop/Android, same binary |

```bash
make -C wii host-test        # input, physics-golden, game tests (host compiler + Node)
make -C wii                  # build/boot.dol
make -C wii iso release      # dist/: HBC zip, ISO, BYCEAO.ini, SHA256SUMS
```

## Controls (Wii Sports Bowling style; no pointer, no sensor bar)

Hold **B**, swing back, pause, swing through, **let go of B**. Swing speed sets power; swing left/right
aims; wrist twist / lateral motion at release curls the bag (mirrored for LEFT HAND). **D-pad ←/→** steps
along the foul line, **↑/↓** throw height, **A** SLICK/STICKY, **−/+** fine aim, **2** reset bag, **1** help,
**HOME** pause (RESUME · CLEAR BOARD · HAND · SETTINGS · MAIN MENU · QUIT).

## Layout

```
wii/
  Makefile                 VERSION source; all / host-test / iso / release / soak
  TOOLCHAIN.md             versions used and pins
  docker/                  pinned build image
  source/core/             platform-free C (host-testable): physics, rules/game flow, CPU, wind/rng, saves,
                           swing detector + input (wii/input), cornhole_constants.h (generated)
  source/wii/              GX renderer (gfx, scene, hud), WPAD adapter, libfat saves, main loop
  tools/                   gen-constants.mjs (shared/cornhole → C), oracle/golden.mjs, bake_win.c,
                           mkdisc.mjs, make-icon.mjs, zip.mjs
  tests/                   host tests + golden/arcade-1.0.3-physics.json
  hbc/apps/backyardcornhole/  meta.xml, icon.png (boot.dol is a build output)
  dolphin/                 GameSettings/BYCEAO.ini, apploader/, PROVENANCE.md, INSTALL-KINHANK.md
```

Standing rules: port from the 1.0.3 snapshot only; toss power = swing **speed**, never distance; NTSC/60 Hz
only; HBC `boot.dol` is byte-identical to the ISO's `sys/main.dol`; shared rules values come from
`shared/cornhole/` through the generated header.

Docs: [SPEC.md](../docs/wii-port/SPEC.md) · [IMPL_PLAN.md](../docs/wii-port/IMPL_PLAN.md) · [WIIMOTE_ARCADE_MAPPING.md](../docs/wii-port/WIIMOTE_ARCADE_MAPPING.md) · [ARCADE_PARITY_CHECKLIST.md](../docs/wii-port/ARCADE_PARITY_CHECKLIST.md) · [INPUT.md](INPUT.md)
