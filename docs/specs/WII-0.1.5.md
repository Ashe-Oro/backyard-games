# Build "Backyard Cornhole Wii 0.1.5", a Kinhank (Dolphin on Android) release that cuts default throw power 25% and makes the keg, beer pong cups and beer pong ball round again.

## Reference
- 0.1.4 on `main` at f3c69e4 (PR #64, tags `wii-v0.1.4` / `kinhank-v0.1.4`). Keep its controls: KOBE / NORMAL / SLIDER on the D-pad picker as the only height/arc control, swing speed = power, sideways swing = aim, no IR, no −/+ aim nudge, pointer for menus only.
- Ashe, Nov 22, 2026, 9:57 AM ET: default power is too high ("it's too easy to overthrow the board"); reduce it 25%. The keg, beer pong cups and beer pong ball have sharp, faceted edges and should be circular / round again.
- 0.1.4 host-test (`test_swing_to_landing`) printed: swing speeds 0.45 / 0.65 / 0.9 g → power 0.612 / 0.736 / 0.866 → landing 8.21 / 8.77 / 9.85 m (distance = `-(land_z - BYC_REL_Z)`).
- Board geometry (report these with the new landings): `BYC_BOARD_Z = -8.2296` m (27 ft), `BYC_REL_Z = -0.45` m, so the board front is 7.7796 m from release. `BYC_L = 1.2192` m, `BYC_TH` tilt; the board's z-span is about `L*cos(TH) ≈ 1.200` m, so the deck covers roughly 7.78–8.98 m of throw distance. In 0.1.4 the 8.21 and 8.77 m landings were on the board; 9.85 m went past it.
- Gold look: a close-up of the keg and of the beer pong table should show smooth circular silhouettes and rounded shading, not octagons or boxes. Compare against a 0.1.4 close-up of the same views (ISO at `/workspace/scratch/wii-0.1.4/BackyardCornhole-BYCEAO.iso`). Kinhank runs at 3× internal resolution; judge the shapes at that sharpness. Frame rate stays about 60 fps in Dolphin (e2e fps.txt).
- Existing 0.1.4 e2e practice shot: `/workspace/scratch/wii-0.1.4/dolphin-e2e/aim.png` (wide yard view; keg right, pong table left). Close-ups must be much tighter than this.

## Details
1. Reduce default throw power by 25%. The same physical swing must produce 0.75× the power it does in 0.1.4. Mapping starts at `swing_power(peak, full_speed)` in `wii/source/core/swing.c` (about lines 20, 90, 204, 226) and the default profile's `full_speed` / calibration `target_power` (about line 428 and `swing_default_profile`). Prefer whichever change keeps the mapping linear and keeps max reachable power at 1.0 so a strong swing can still reach the board. A 0.75 multiplier on the already-clamped result would cap max at 0.75 — avoid that unless you prove a strong swing still reaches the board and document why. Update and extend `test_swing_to_landing` (and any other power assertions): the three 0.1.4 speeds must land shorter in proportion, and a normal-strength swing (the middle speed, and a near-full swing) must land on or near the board rather than past it. Print the new powers and landing distances. Leave aim, curl, the picker, weak-fallback behaviour (aside from the 0.75 scale) and IR-free input alone.

2. Make the keg, beer pong cups and beer pong ball smooth and round again. Hypothesis (verify, do not assume): `party_draw.c` draws cylinders with `cyl8()` — two boxes 45° apart, an octagonal silhouette — and the beer pong ball with `gfx_box_unlit` (a cube). That is a coarse tessellation, and at Kinhank 3× IR the facets read as sharp edges. Other possible contributors: a later perf cap, the 16:9 HUD layout, or flat per-face normals. Find the actual cause and fix it. Raise radial segment counts; use a real cylinder / sphere (or equivalent triangle bands) rather than two rotated boxes; use smooth (averaged) normals if the shading is faceted. Also round the cups held by the keg guest if those are the same faceted stand-ins. Do not change party animation, placement, colours or the boxy character art. Do not lower Kinhank `InternalResolution`. Keep ~60 fps in Dolphin.

3. Version 0.1.5 on the STRAP ON! screen and the in-game HUD footer, matching `VERSION` in `wii/Makefile` and `kinhank/Makefile`, `BYC_VERSION`, `wii/hbc/apps/backyardcornhole/meta.xml`, `wii/INSTALL-WII.md`, `wii/README.md`, `kinhank/INSTALL.md`, and new `## [0.1.5]` entries in `wii/CHANGELOG.md` and `kinhank/CHANGELOG.md` (Keep a Changelog). Do not rewrite historical 0.1.4 entries. Keep the KOBE / NORMAL / SLIDER labels (hud.c already uses KOBE). No IR. No −/+ nudge.

4. The arcade and web trees are untouched. Do not force-push, do not print or persist tokens, do not contact Retro, do not push, do not open a PR, and do not install on the Kinhank.

## Tools
- Worktree `/workspace/cornhole-wii-0.1.5`, branch `wii/power-round-0.1.5` from `origin/main` (f3c69e4).
- Toolchain: `DEVKITPRO=/tmp/byc-wii-tools/opt/devkitpro`, `WIT=/tmp/byc-wii-tools/wit-source/project/bin/wit` (system `wit` 3.01a cannot build the disc). Dolphin: `DOLPHIN=/usr/games/dolphin-emu`, `E2E_VIDEO=Vulkan`.
- Build: `make -C wii host-test`, `make -C wii release`, `make -C kinhank`. Pass DEVKITPRO and WIT on those make lines (or export them).
- Take close-up before (0.1.4 ISO) and after (this build) shots of the keg and the beer pong table. Prefer 3× internal resolution, hide the help overlay, crop tight. If the default yard camera is too far, use a temporary look-at for the shots only — do not ship a debug camera.
- Local commits only (author Ashe-Oro <ashe@asheoro.com> as in this repo). Do not `git push`. Do not write secrets, ISO files, or build artifacts into git.

## How to work
Autonomous, no questions. Log assumptions in `WII-0.1.5-LOG.md` at the repo root. Confirm the faceting cause in that log (what you found, not only the hypothesis).

Checklist and measurable targets:
- Same three scripted swings (0.45 / 0.65 / 0.9 g) produce ~0.75× the 0.1.4 powers (0.612 / 0.736 / 0.866) and shorter landings than 8.21 / 8.77 / 9.85 m. Mid and near-full swings land on or near the board (about 7.78–8.98 m throw distance), not past it. A swing that reaches power 1.0 can still reach the board.
- Faster swings still land strictly farther; left/right aim, handedness, −/+ no-op, and picker-only height are unchanged.
- Close-up after shots of the keg, cups and ball are visibly round (no octagon/box silhouette); before shots from 0.1.4 show the faceting. Dolphin e2e reports about 60 fps and PASS.
- `make -C wii host-test` is green. `make -C wii release` and `make -C kinhank` succeed. Strap and HUD show v0.1.5. `wii/tools/check-no-ir.sh` still passes.
- Limit parallel build load; leave CPU for other jobs on this box. Stop any Dolphin / Xvfb processes you start.

## Deliver
- Local commit(s) on `wii/power-round-0.1.5` with the code, tests, changelogs, version bump, this spec, and `WII-0.1.5-LOG.md`.
- Artifacts in `/workspace/scratch/wii-0.1.5/` (create the directory): `BackyardCornhole-BYCEAO.iso` plus a correct `.sha256` of that exact file, the Kinhank zip, host-test log, Dolphin e2e output, and named before/after close-up shots of the keg and the beer pong table (e.g. `keg-before.png`, `keg-after.png`, `pong-before.png`, `pong-after.png`).
- Do not push. Write a short summary at the end of the log and in your final message: root cause of the faceting, how power was changed, old vs new power and landing numbers, board-distance recap, test counts, e2e result and fps, commit SHAs, ISO sha256, and shot paths.
