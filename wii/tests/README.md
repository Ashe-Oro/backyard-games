Host input tests: `make -C wii host-test` or `sh wii/tests/run-input-tests.sh` from the repo root. The shell runner also works from another directory and deletes its temporary executables.

`bowling_trace.h` models the hand moving around the shoulder with a changing radius and wrist angle. The raw axes match libogc and the game. Tip up reads `(0,-1,0)` g and flat with buttons up reads `(0,0,+1)` g. The fixture adds seeded noise, report jitter, accelerometer quantization and a ±3.6 g rail. The Wii Remote movie generator in `../test/gen_wiimote_dtm.py` uses the same motion at Dolphin's input cadence. The repository has no recorded Remote traces.

`throw_test.c` replays the motion through the real input layer. It covers the exact static seed vectors, slow tip rotation, both grips and rolls at 60/100 Hz, release on both sides of the bottom, every named rejection, forward-only power, clipped shoulders, 30–60 ms gaps, duplicated samples, a stalled report stream, retry and handedness. A seeded sweep reports results separately for tip-forward and in-line grips. It caps tip-forward early-release throws at 3%. Earlier-than-bottom releases in some randomized in-line swings can still throw and need hardware traces for tuning.

`input_test.c` covers immediate grip, B-up gating, pause and reconnect, player slots, menu and play buttons, calibration and the 1+2 debug chord. The input and game fixtures use the same physical swing. `game_test.c` exercises the full path from sensor report to bag landing and preview arrow. It checks that power rises with forward speed, the same lateral motion aims the same way for either hand, −/+ leave flight unchanged, and the KOBE/NORMAL/SLIDER picker controls trajectory. It also covers LAST SHOT persistence, canceled practice tosses and factory and custom power saves. The no-IR check is part of `make host-test`.

`wpad_test.c` links the actual adapter against a test-owned API double in `fake/`. It verifies queue draining, registration errors, reconnect, report validity, drops and estimated timestamps. Physical traces run with uniform drains and 60 Hz frame batches, including Bluetooth stalls and frame hitches. Run `wii/build/wpad-test dump-realistic` to export the fixtures as CSV. The fake header is only used in host builds.

A C11 host compiler with libm is sufficient for the motion tests. Keep assertions enabled.

Game tests (wii/game): `make -C wii host-test` also runs
- `physics_golden.mjs` + `physics_probe.c`: the C physics/wind/RNG/CPU core against
  `golden/arcade-1.0.3-physics.json`, produced by the read-only oracle `tools/oracle/golden.mjs` from the pinned
  1.0.3 snapshot (WIN table within one 0.0025 step, 66 shots and 8 multi-bag scenes with identical states and
  transition sequences and positions within 1 mm, roundWind sequences, mulberry32, 24 seeded planCareerShot
  decisions including RNG draws).
- `game_test.c`: menus, a 20-round 2 PLAYER soak, 200 practice throws with CLEAR BOARD, career vs the first
  and last rivals, scoring (cancellation, wash keeps order, crossing 21, moving slides don't count), wind policy,
  straight-from-stance throws and the KOBE remap, pause freezing, disconnect/resume, hand/settings/SET POWER,
  saves (round trip, CRC failure, invalid fields); review fixes: CLEAR BOARD while aiming, waiting and in flight,
  pause/resume on round summary, FINAL and tournament result, practice past 256 throws (1,500 throws with 800+
  bags left in the yard, plus a simulated allocation ceiling), the combined arrow (forward-swing preview,
  straight/left/right, both hands, power ordering, 2 s hold + fade, projected launch line), the MUSIC screen
  against a fake backend, wind streaks.
- `party_test.c`: the `party.mjs` port (pong catalog, scripts, miss paths, non-repeating selection, keg/keg-stand
  and cook timelines, geometry) and a 30-minute soak checked against values from the JS module.
- `check-generated`: `cornhole_constants.h` matches `shared/cornhole`, and the golden vectors match the oracle.
