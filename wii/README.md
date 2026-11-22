# Backyard Cornhole — Wii

Native Wii homebrew port of Backyard Cornhole Arcade **1.0.3** (C, devkitPPC + libogc, raw GX, WPAD).

**Status: 0.1.5.** Practice, 2 Player and the 8-rival Tournament run on a GX
renderer; physics, scoring and the tournament CPU match the pinned 1.0.3 game on the host golden tests.
Boots in Dolphin from the ISO (game ID `BYCEAO`) and as a bare DOL. Swing feel and hardware timing are
not validated yet: that needs a physical Wii Remote retest on a real Wii and on the Kinhank (IMPL_PLAN P1).

| Target | Package |
|---|---|
| Real softmodded Wii (`wii-v*`) | Homebrew Channel zip: `sd:/apps/backyardcornhole/{boot.dol,meta.xml,icon.png}`, plus the ISO. See [INSTALL-WII.md](INSTALL-WII.md) and [../TARGETS.md](../TARGETS.md) |
| Kinhank X5 Pro (current test target, `kinhank-v*`) | Same ISO + `GameSettings/BYCEAO.ini` at 3×. See [../kinhank/](../kinhank/) |
| Dev loop | Dolphin desktop/Android, same binary |

Build from source: [BUILD.md](BUILD.md). Play on a real Wii: [INSTALL-WII.md](INSTALL-WII.md).

```bash
make -C wii host-test        # input, physics-golden, game tests (host compiler + Node)
make -C wii                  # build/boot.dol
make -C wii release OUT=/workspace/uploads/wii-game # complete release, one command
make -C wii smoke OUT=/workspace/uploads/wii-game   # isolated 25-second NoGUI capture
```

## Controls (Wii Sports Bowling style; no pointer, no sensor bar)

Hold **B**, swing back, swing forward, then **let go of B**. Swing speed sets power; swing left/right
aims; wrist twist / lateral motion at release curls the bag (mirrored for LEFT HAND). **D-pad ↑/↓** picks
KOBE / NORMAL / SLIDER (the only control for the arc), **←/→** steps along the foul line, **A** SLICK/STICKY,
**2** reset bag, **1** help (−/+ do nothing in play; they adjust settings in menus),
**HOME** pause (RESUME · CLEAR BOARD · HAND · SETTINGS · MAIN MENU · QUIT). Hold **1+2** for two
seconds to toggle swing diagnostics.

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
  dolphin/                 apploader/, PROVENANCE.md; GameSettings/BYCEAO.ini is a symlink to kinhank/
```

Standing rules: port from the 1.0.3 snapshot only; toss power = swing **speed**, never distance; NTSC/60 Hz
only; HBC `boot.dol` is byte-identical to the ISO's `sys/main.dol`; shared rules values come from
`shared/cornhole/` through the generated header.

Docs: [SPEC.md](../docs/wii-port/SPEC.md) · [IMPL_PLAN.md](../docs/wii-port/IMPL_PLAN.md) · [WIIMOTE_ARCADE_MAPPING.md](../docs/wii-port/WIIMOTE_ARCADE_MAPPING.md) · [ARCADE_PARITY_CHECKLIST.md](../docs/wii-port/ARCADE_PARITY_CHECKLIST.md) · [INPUT.md](INPUT.md)

Release packaging: [packaging/README.md](packaging/README.md). Kinhank handoff: [../kinhank/INSTALL.md](../kinhank/INSTALL.md).
