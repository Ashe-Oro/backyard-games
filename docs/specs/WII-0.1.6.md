# Build "Backyard Cornhole Wii 0.1.6", a Kinhank release that scales throw power back again (the arcade way) so a real Wiimote swing lands on the board, and that stops a phantom practice throw right after boot.

## Reference
- 0.1.5 on `main` at bd5f7be (PR #65, tags `wii-v0.1.5` / `kinhank-v0.1.5`). Keep its controls: KOBE / NORMAL / SLIDER on the D-pad picker as the only height/arc control, swing speed = power, sideways swing = aim, no IR, no −/+ nudge.
- Ashe, relayed by Retro at 10:27 AM ET on Nov 22, 2026, after playing 0.1.5 on the Kinhank: "it's still too easy to overthrow the board, so there's still too much power coming from the controller. just like we did on the arcade, just keep scaling it back, please."
- Arcade approach (mirror this, do not invent a different kind of curve): they raised the full-power speed threshold so the same physical input fills less of the meter. In `arcade/CHANGELOG.md` and `trackball.mjs`, power is `clamp(peak / powerSpeed, 0, 1)`. 0.2.23 / 1.0.0 used 5600 counts/s; 1.0.1 raised that 35% to 7560 ("full-power throws need 35% more spinner speed"); 1.0.2 set 6048 (20% less than 7560). They did not only multiply an already-clamped result.
- 0.1.5 mapping: `power = clamp(0.75 * peak / full_speed, 0, 1)` with `full_speed = swing_speed(SWING_DEFAULT_E_G)` and `SWING_DEFAULT_E_G = 1.2`. Scripted NORMAL landings (distance = `BYC_REL_Z - land_z`): 0.45 g → 0.459 / 7.58 m; 0.65 g → 0.552 / 7.94 m; 0.90 g → 0.650 / 8.38 m; 1.20 g → 0.750 / 8.84 m; unclipped 2.20 g → 1.000 / 10.63 m. Because 0.75 * sqrt(e/1.2) hits 1.0 at about 2.13 g, a typical real Wiimote bowling swing (often 2–4 g) saturates and overthrows.
- Board geometry: `BYC_BOARD_Z = -8.2296` m (27 ft), `BYC_REL_Z = -0.45` m, deck span **7.7796–8.9795 m** from release. A full-strength swing should land near the back of that deck (about 8.7–8.98 m), not at 10.63 m past it.
- Phantom throw: right after boot on the Kinhank, the practice HUD already showed "1 THROWS" although Retro sent no ADB input. Ashe had to press A (or 2) to leave STRAP ON! and start Practice, so a menu confirm or stale B/accel is in play. `practice_throws` only increments in `after_throw` once a bag has settled, so this is a real counted throw, not a label typo. Hypothesis (verify; do not assume): `input_start_match` / `input_next_turn` / `input_set_context(INPUT_PLAY)` all call `swing_reset(..., false)`, so a B held through the strap or title (or a stale `b_held` from `input_connection`) grips as soon as practice starts and the B-up throws. The pause path is protected by `input_resume` waiting for B up; starting practice from the title is not. Boot accel alone should not throw without B.

## Details
1. Cut default throw power again, the arcade way. Raise the swing speed that maps to full power (higher default `full_speed` / `SWING_DEFAULT_E_G`) so the same Wiimote swing produces less power and you have to swing really hard to reach the top. Also lower `SWING_POWER_GAIN` from 0.75 (about 0.55 is a reasonable starting point) and add a max-power cap so a full-strength swing lands near the back of the board, not far past it. Measure the NORMAL landing vs power and pick the cap from that, then publish the chosen gain, full-speed / default e, and cap. Keep the mapping linear below the cap. Recalibration (SET POWER) should still be able to target the hole within the new limits. Update tests, including the 0.1.5 scripted speeds and a table for **0.45, 0.65, 0.9, 1.5, 2, 3 and 4 g**. Weak-fallback lobs stay weak and below the cap. Leave aim, curl, the picker, no-IR, and no −/+ nudge alone.

2. Find and, if real, fix the phantom practice throw. Add a test that boots to the strap, confirms through the title into Practice the way a player does (A or 2), and does not count a throw from: a B held across the menu confirm, a B-up immediately after play starts, stale/unknown B state after connect, or boot-like accelerometer motion without a fresh B press. `practice_throws` must stay 0 and no bag may be in flight until a new B-down and B-up in play. Do not break the intentional mid-match rule that a B already held during the bag flight or turn change can grip when the next turn opens. Do not reintroduce a stillness / settle-time gate on a real throw.

3. Version 0.1.6 on the STRAP ON! screen and the in-game HUD footer, matching `VERSION` in `wii/Makefile` and `kinhank/Makefile`, `BYC_VERSION`, `wii/hbc/apps/backyardcornhole/meta.xml`, install/README docs, and new `## [0.1.6]` entries in `wii/CHANGELOG.md` and `kinhank/CHANGELOG.md`. Do not rewrite historical 0.1.5 entries. Keep the KOBE / NORMAL / SLIDER labels.

4. The arcade and web trees are untouched. Do not force-push, do not print or persist tokens, do not contact Retro, do not push, do not open a PR, and do not install on the Kinhank.

## Tools
- Worktree `/workspace/cornhole-wii-0.1.6`, branch `wii/power-phantom-0.1.6` from `origin/main` (bd5f7be).
- Toolchain: `DEVKITPRO=/tmp/byc-wii-tools/opt/devkitpro`, `WIT=/tmp/byc-wii-tools/wit-source/project/bin/wit` (system `wit` 3.01a cannot build the disc). Dolphin: `DOLPHIN=/usr/games/dolphin-emu`, `E2E_VIDEO=Vulkan`.
- Build: `make -C wii host-test`, `make -C wii release`, `make -C kinhank`. Pass DEVKITPRO and WIT. If the power change makes the e2e DTM miss the hole, update the scripted toss energies in `wii/test/gen_wiimote_dtm.py` the same way 0.1.5 did, still buttons + accelerometer only.
- Local commits only (author Ashe-Oro <ashe@asheoro.com>). Do not `git push`. Do not write secrets, ISO files, or build artifacts into git.

## How to work
Autonomous, no questions. Log assumptions, the arcade comparison, the chosen mapping, the landing table, and the phantom-throw root cause (what you found, not only the hypothesis) in `WII-0.1.6-LOG.md` at the repo root.

Checklist and measurable targets:
- Publish a swing-speed-to-landing table for 0.45, 0.65, 0.9, 1.5, 2, 3 and 4 g (NORMAL, calm, centered) with power and distance, plus the board span 7.7796–8.9795 m. Mid-to-hard real swings (about 1.5–3 g) land on or near the board, not past it. A 4 g / max-power NORMAL throw lands near the back of the board (about 8.7–8.98 m), not around 10.6 m.
- Faster swings still land strictly farther until the cap; left/right aim, handedness, −/+ no-op, and picker-only height are unchanged.
- The new phantom-throw test fails on 0.1.5 behavior and passes after the fix. A deliberate bowling throw in practice still counts.
- `make -C wii host-test` is green. `make -C wii release` and `make -C kinhank` succeed. Strap and HUD show v0.1.6. `wii/tools/check-no-ir.sh` still passes.
- Dolphin e2e (`DOLPHIN=/usr/games/dolphin-emu E2E_VIDEO=Vulkan`) PASS, about 60 fps in emulated time.
- Limit parallel build load. Stop any Dolphin / Xvfb processes you start.

## Deliver
- Local commit(s) on `wii/power-phantom-0.1.6` with the code, tests, changelogs, version bump, this spec, and `WII-0.1.6-LOG.md`.
- Artifacts in `/workspace/scratch/wii-0.1.6/` (create the directory): `BackyardCornhole-BYCEAO.iso` plus a correct `.sha256` of that exact file, the Kinhank zip (and a zip `.sha256`), host-test log, and Dolphin e2e output.
- Do not push. End with: phantom-throw finding, the new mapping (gain, full-speed/e, cap), the landing table, board-distance recap, test counts, e2e result and fps, commit SHAs, ISO sha256, and artifact paths.
