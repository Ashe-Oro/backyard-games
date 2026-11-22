# Backyard Cornhole Wii 0.1.4 log

Spec: `docs/specs/WII-0.1.4.md`. Branch `wii/dpad-only-0.1.4` from main `feb6c8c`. Local commits only.

Ashe's rule, as in Wii Sports Bowling: the D-pad sets up the throw (LEFT/RIGHT moves the start position, UP/DOWN picks LOB / NORMAL / SLIDER). Speed and direction come only from the Wiimote's accelerometer. The pointer is for menus only, and in practice it is never enabled at all.

## What changed

- **No button aim.** `Swing.fine_aim`, the `aim` argument of `swing_feed`, `InputSlot.aim`, `SWING_AIM_NUDGE` and the HUD's `AIM: +N` label are gone. A grip starts straight (`locked_aim = 0`). In gameplay, -/+ are no longer read at all: they don't aim, and they no longer reset the swing either (0.1.3 treated them as a setting change). Menus still get D-pad, -/+, A/2 and B.
- **Where each part of the throw comes from** (unchanged maths, now the only inputs):
  - Power: `swing_power(peak, full_speed)` (`wii/source/core/swing.c:20`), applied to the forward stroke's peak speed at `swing.c:226`. The peak is `lobe_peak`, the highest 80 ms window speed in the stroke (`swing.c:280`, `:285`).
  - Aim: lateral acceleration accumulated during the forward stroke (`swing.c:277`), averaged (`swing.c:299`) and locked (`swing.c:302`). The weak-throw path uses the same sideways signal (`swing.c:215`).
  - Arc: the D-pad picker (`wii/source/core/input.c:207-208` → `wii/source/core/game.c:588`, and `game.c:261` each turn), used at launch (`game.c:281`, `:288`). `SwingResult` has no pitch field, so the remote's pitch at release cannot reach the throw.
- **HUD.** The help panel lists one control per line: B HOLD, SWING BACK & FORWARD, LET GO / SWING SPEED = POWER / SWING LEFT/RIGHT TO AIM / TWIST TO CURL / D-PAD UP/DOWN LOB-NORMAL-SLIDER / D-PAD LEFT/RIGHT MOVE / A SURFACE / 2 RESET / 1 HELP / HOME PAUSE. The bottom bar reads `HOLD B|SWING  ↑↓ NORMAL  A SLICK  ←→ MOVE  1:HELP`. The picker now says **LOB** where it said KOBE, to match the help text (internally it is still `ARC_KOBE`; it's a one-word change in `hud.c` if Ashe prefers KOBE). `v0.1.4` shows on the STRAP ON! card and in the bottom-right corner during play. The 1+2 overlay is now seven lines; the new ones are `LAST POWER` and `LAST AIM` (radians and degrees).
- **Docs:** `docs/wii-port/WIIMOTE_ARCADE_MAPPING.md` (new "Current Wii mapping (0.1.4)" table at the top, plus the button, aim, legality, state-machine and T16 rows), `wii/INPUT.md`, `wii/README.md`, `wii/INSTALL-WII.md`, `kinhank/INSTALL.md`, `wii/tests/README.md`, meta.xml. `TARGETS.md` doesn't list controls, so it is unchanged. `arcade/`, `web/` and `shared/` are untouched.

## Tests (written first, commit `62c6693`)

- `input_test` `plus_minus_change_nothing_in_play`: the same swing with -, + or both pressed every other report (before the grip, and while B is held) gives identical power, aim, spin and release. No ADJUST events are sent, and presses before the grip leave it ready.
- `input_test` help check: every control above is in `input_help_text`, and `-/+`, `FINE AIM` and `POINT` are not.
- `game_test` `test_swing_to_landing` drives scripted swings through the real input layer and the game, then steps the launched bag at the game's fixed 240 Hz until it touches down, on an empty, calm practice board:
  - Swing excess 0.45 / 0.65 / 0.9 g → power 0.612 / 0.736 / 0.866 → lands 8.21 / 8.77 / 9.85 m down-range.
  - A 12° right/left lean lands at x = +0.442 / -0.442 m (mirror images); LEFT HAND flips both.
  - -/+ held throughout give a bit-identical flight.
  - Release pitch -30 / 0 / 25 / 80° gives a bit-identical flight. With a sideways swing the height profile is identical (see risks for aim).
  - SLIDER / NORMAL / LOB from D-pad presses peak at 1.83 / 2.32 / 4.10 m, and the picker carries over to the next bag.
  - D-pad RIGHT/LEFT step the stance by 0.05.
- `test_version_tag`: `BYC_VERSION` (now passed from the Makefile to the host build) is 0.1.4, and the tag is `v0.1.4`.
- `tools/check-no-ir.sh`, run by `make host-test`, fails on any IR data format, IR read or `WPAD_SetVRes` in `wii/source`, or any data format other than `WPAD_FMT_BTNS_ACC`. The existing WPAD test still asserts `WPAD_FMT_BTNS_ACC`.
- **Against 0.1.3** (`/workspace/scratch/wii-0.1.4/before-0.1.3-tests.txt`): the -/+ flight check and the two version checks fail (3 of 2292). The input tests don't compile, because the fine-aim argument is gone. Speed → distance, left/right, release pitch and the picker already behaved in 0.1.3, so those tests now guard 0.1.4 rather than reproduce a bug.

Two test bugs were found and fixed before the tests were committed. Earlier practice bags left on the board caught later landings, so the board is now cleared before each throw. The 240 Hz accumulator phase made 60 fps samples differ by a substep, so the flight is now stepped from the launched bag.

## Results

- `make -C wii host-test`: input 36/36, throw-trace 12/12 (0 failed checks; 0.1.3 traces a–f all throw), WPAD 12/12, physics golden 938/938, game 2292/2292, party 209137/209137, no-IR check ok.
- `make release` (devkitPPC r51 from `/tmp/byc-wii-tools/opt/devkitpro`, WIT 3.05a from `/tmp/byc-wii-tools/wit-source/project/bin/wit`; this box has no `/opt/devkitpro`, and the system `wit` is 3.01a, which can't compose the disc): DOL with no warnings, ASND CRC 0xdbbeeb61, ISO `main.dol == boot.dol`, HBC zip and release zip. `make -C kinhank` zip keeps `InternalResolution = 3` and `WiimoteSource0/1 = 2`.
- Dolphin e2e (`DOLPHIN=/usr/games/dolphin-emu E2E_VIDEO=Vulkan`, emulated Wiimote via DTM, buttons + accelerometer packets only, profile checked for no IR/Pointer bindings): PASS. Boot, STRAP ON! with `v0.1.4`, help toggled twice, 1+2 overlay on and off, one B-up throw (power 0.645, aim +0.000), a scored HANGER, about 60 fps. A fresh save shows the help panel, so the first 1 tucks it and the second brings it back for `help.png`. The X screen grab lags the game by a few seconds, so the watcher waits before grabbing.

## Artifacts (`/workspace/scratch/wii-0.1.4/`)

- `BackyardCornhole-BYCEAO.iso` (+ `.sha256`), `BackyardCornhole-Kinhank-v0.1.4.zip` (+ `.sha256`), `release/` (HBC zip, release zip, SHA256SUMS)
- `dolphin-e2e/`: `strap.png` (version tag), `help.png` (new help text, bottom bar, corner tag), `debug.png` (7-line overlay before the throw), `aim.png`, `score.png`, logs, DTM
- `before-0.1.3-tests.txt`

## Still risky on real hardware

- **Release pitch nudges aim slightly.** The sideways signal is `g.x / |g|` and includes the B-up report itself, so the remote's pitch on that one report scales its lateral reading by cos(pitch). With a 12° lean, aim is 0.0499 rad at 0° release pitch and 0.0470 at 80°, about 6%. It never touches power or the arc. Excluding the B-up report from the aim would remove this, but it would change aim for every 0.1.3 trace, so I left it alone.
- The `input action 5` value in the OSReport log is the input layer's own toggle, not the panel. A fresh save starts with the panel shown, so the log's 1/0 reads inverted against the screen. This is cosmetic and predates 0.1.4.
- The 0.1.3 risks still apply: everything is modelled (no physical Wii Remote yet), full power is easy at the 1.2 g default, and there is a power cliff between the weak throw (≤ 0.35) and the weakest forward stroke (about 0.56).
- With -/+ gone, aim depends entirely on the sideways swing. If the real-remote lateral signal turns out noisy, there is no longer a button to trim it.
