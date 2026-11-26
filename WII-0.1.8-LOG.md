# Backyard Cornhole Wii 0.1.8

Implemented on `wii/swing-detect-rewrite` from `9907c14`. The Wii and Kinhank Makefiles, Homebrew Channel metadata, STRAP ON! screen and in-game footer identify v0.1.8. The two targets use the same BYCEAO disc.

## Axis finding and release timing

Ashe's DolphinBar readings match raw libogc `gforce`. Tip up is **(0, −1, 0) g** and flat at the TV, buttons up, is **(0, 0, +1) g**. Positive X points to the remote's left. In a tip-forward grip, Z carries most of the radial load at the bottom and −Y is forward tangent. With the remote in line with the arm, Y carries most of the radial load and Z carries the tangent, with its sign set by the roll. The 0.1.7 classifier used the opposite upright Y sign, so the push at the turnaround could open its release window before the remote reached the bottom.

The detector advances through **READY → GRIP → BACKSWING → TURN → FORWARD → PAST BOTTOM → DONE**. It learns the radial axis from the back-pass lobe and the forward sign from address or the turnaround drive. A forward radial crest estimates when the remote crossed the bottom. If B goes up before confirmation, **RESOLVE** waits up to **160 ms**. The estimate allows **10 ms** of tolerance, reduced to zero when the grip rotates over 120° and top-of-lobe directions span over 15°. The window closes after **600 ms** or a follow-through turnaround. A deferred throw uses the original B-up timestamp and stance.

Every rejected release keeps the bag and returns to READY. The last reason is **NO SWING**, **LET GO ON BACKSWING**, **LET GO BEFORE BOTTOM**, **FORWARD TOO WEAK**, **LET GO TOO LATE** or **REPORT GAP**. The same raw sideways motion aims in the same direction for either hand. LEFT HAND retains mirrored curl.

## Forward power

Power uses the strongest 40 ms mean of forward radial excess above 1 g, from the turnaround through B-up. The shoulder model uses a **0.60 m** radius. The forward peak needs **0.30 g** excess to qualify. Near the **3.55 g** rail, a quadratic fit can recover the peak from unclipped shoulders recorded through B-up. With too few shoulders, the detector uses the measured speed.

The factory speed reference drops from **3.344 to 2.45 m/s**. Preview and release still use `clamp(0.55 * peak_speed / (full_speed * type_scale), 0, 0.76)`. The NORMAL cap moves from 5.54 to **4.06 m/s**, within reach of a strong modeled throw with clipping reconstruction. The host game lands a centered NORMAL throw at the cap **8.9025 m** from release, inside the **7.7796–8.9795 m** deck span.

| Picker | Speed reference multiplier | 0.1.7 cap speed | 0.1.8 cap speed |
| --- | ---: | ---: | ---: |
| SLIDER | 1.30 | 6.01 m/s | 4.40 m/s |
| NORMAL | 1.20 | 5.54 m/s | 4.06 m/s |
| KOBE | 1/0.90 | 5.13 m/s | 3.76 m/s |

Save version 4 keeps the field layout. It migrates the exact old factory references and preserves custom SET POWER values. Varying only the backswing from 0.40 to 1.00 s changes modeled power by under 5%.

## Before and after

These outcomes use physical synthetic traces in the raw axes. The named examples below use the tip-forward grip. The 0.1.8 host matrix also tests both rolls of both grips at 60 and 100 Hz with quantized and noisy reports.

| Scenario | 0.1.7 | 0.1.8 |
| --- | --- | --- |
| Proper swing, B-up at +5°, +15° or +40° | Throws | Throws |
| Slow forward stroke, 0.65 s | Cancels | Throws |
| B-up at the leg while going back | Cancels | LET GO ON BACKSWING |
| B-up on the return at −20°, or −8° with a short backswing | Throws | LET GO BEFORE BOTTOM |
| Hold still, tip up or flat | Cancels | NO SWING |
| Violent ±8° wiggle | Throws | Cancels |

The seeded host sweep mixes both rolls and rates. Each entry is throws per 500 trials, using the same fixtures on both versions.

| Grip | Scenario | 0.1.7 | 0.1.8 |
| --- | --- | ---: | ---: |
| Tip-forward | Proper | 424 | 492 |
| Tip-forward | Early return | 405 | 15 |
| In-line | Proper | 0 | 499 |
| In-line | Early return | 0 | 66 |
| Both | Going back or still, per condition and grip | 0 | 0 |

## Diagnostics

Hold **1+2** for two seconds to open the overlay. It keeps the original `ACCEL G` row and adds signed raw Y/Z, magnitude and clipping, static or moving pitch, face and roll, phase/B/pass count, reject reason, tangent/radial/lateral load and excess, estimated arm angle and speed, forward peak and fit, bottom timing, last throw, reject counts and report timing, and the learned axis and sign source.

With B up, press **A** to open pose calibration. Select upright, flat at the TV, roll right 90°, tip down, or a 30° lean toward the TV. **A** captures 300 ms of readings near 1 g starting after the press, and **2** goes back to the live view. The lean's Z sign identifies which way the buttons face. The overlay buffers CSV reports in RAM and flushes in READY or DONE to `sd:/apps/backyardcornhole/data/swing-trace.csv` or OSReport. Each session starts with a header, and the SD trace rotates at 1 MiB with one backup. The learned axis clears on cancel and reject.

## Verification

- `bowling_trace.h`, `throw_test.c`, `input_test.c`, `wpad_test.c` and the game swing fixtures use the physical shoulder-pivot model and its negative-Y tip. The old expectation that −5° releases throw was removed. The emulated Wiimote DTM generator uses the same kinematics and report convention. The repository has no recorded physical Wiimote traces.
- The 0.1.7 baseline passed 36 input tests, 14 throw-test groups, 12 WPAD tests, 938 physics checks, 2,451 game checks and 209,137 party checks. Generated-file and no-IR checks passed.
- `make -C wii -j2 host-test` on 0.1.8 passed 20 input tests, 11 throw-test groups, 13 WPAD tests, 938 physics checks, 2,426 game checks and 209,137 party checks. The suites ran 160 named swing replays, 4,000 randomized trials and 14 WPAD pose and rotation replays. Generated-file and no-IR checks passed.
- Built the DOL, ISO, HBC ZIP, Wii release ZIP and Kinhank ZIP with devkitPPC r51 and WIT 3.05a r8638. The ISO's `main.dol` matches `boot.dol` byte for byte. Both release checksum lists passed.
- The 3× Vulkan Dolphin replay booted to STRAP ON!, entered Practice, toggled the overlay, and scored a cornhole on one B-up throw at 0.700 power. The ASND check passed. All six generated movie tosses threw in host packet replay.

## Open hardware tuning

- Capture the five pose readings and the raw rails on the DolphinBar/Kinhank. Replay valid, backswing-leg and return-leg releases from real CSV captures.
- Check the in-line grip when wrist rotation continues past the bottom. It still accepts **13.2%** of early returns in the host sweep. Review release latency near the bottom, extreme clipping with too few shoulder samples, and lateral/twist separation on hardware.
