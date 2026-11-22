# Backyard Cornhole Wii 0.1.6

Built on `wii/power-phantom-0.1.6` from `bd5f7bee27a186cddcaa5184f9215ee6433ffe91`.

## Throw power

Arcade 0.2.23 and 1.0.0 used `clamp(peakSpeed / powerSpeed, 0, 1)` with a denominator of 5600 counts/s. Arcade 1.0.1 raised it 35% to 7560 and 1.0.2 lowered it 20% to 6048. The Wii change also raises the speed denominator before clamping, so the same input gives less power along the linear part of the curve. The gain drops from 0.75 to 0.55.

```
v = sqrt(0.6 * 9.80665 * mean_excess_g)
power = clamp(0.55 * retained_forward_peak_v / slot.full_speed, 0, 0.76)
default full_speed = 3.343588 m/s (1.90 g excess)
default unclipped speed at the cap = 4.620231 m/s (3.6279 g excess)
```

In 0.1.5 the reference speed was 2.657 m/s at 1.20 g, with saturation at about 2.13 g. The 0.1.6 reference speed is 25.8% higher. Confirmed clipped forward swings use the same 0.760 cap for preview and release. Weak throws range from 0.066 to 0.1925.

The unchanged 240 Hz physics put NORMAL landings at 8.8403 m for 0.750 power, 8.9025 m for 0.760, and 8.9337 m for 0.770. At 0.780 the bag clears the deck and first touches down at 9.3423 m. The 0.760 cap keeps the landing near the back with about 8 cm of margin. The cap applies to remote power for all three picker positions with their existing launch physics. In the same sweep, a capped KOBE touches down at 8.58 m and a capped SLIDER at 9.79 m. The deck-distance tuning is for NORMAL.

SET POWER still uses the median of three valid unclipped tosses and the 0.699 NORMAL/SLICK hole target. The provisional floor scales with the gain so a classified 0.50 g reference toss can reach that target. A host game test calibrates three 0.65 g tosses and then scores a cornhole in Practice at that speed. Loading a version 1 or 2 save replaces a previous factory speed with the new factory speed and keeps other saved speeds, handedness and curl. Newly written saves carry format version 3 so a custom setting equal to the old factory speed round-trips.

## First landing from release

The host test drives centered, calm NORMAL/SLICK throws through the input and game layers and steps the launched bag at 240 Hz until first contact. The accelerometer column is forward excess above gravity. The speed is an estimate from the retained accelerometer window and the 0.60 m pendulum model. These are synthetic traces and leave real Wii Remote and Kinhank playtesting pending.

The board front is `BYC_BOARD_Z = −8.2296 m` (27 ft from the foul line) and `BYC_REL_Z = −0.45 m`. From release the horizontal deck span is **7.7796–8.9795 m**. At 1.50 g the bag touches down about 3 cm before the deck. At 2.00 and 3.00 g it lands on the deck. At 4.00 g and above the default profile gives 0.760 and lands at 8.90 m in this setup.

| Forward excess | Peak speed | 0.1.6 power | 0.1.6 landing | 0.1.5 landing where measured |
| ---: | ---: | ---: | ---: | ---: |
| 0.45 g | 1.627 m/s | 0.268 | 6.10 m | 7.58 m |
| 0.65 g | 1.956 m/s | 0.322 | 6.55 m | 7.94 m |
| 0.90 g | 2.301 m/s | 0.379 | 7.09 m | 8.38 m |
| 1.20 g | 2.657 m/s | 0.437 | 7.48 m | 8.84 m |
| 1.50 g | 2.971 m/s | 0.489 | 7.75 m | |
| 2.00 g | 3.430 m/s | 0.564 | 8.00 m | |
| 2.20 g | 3.598 m/s | 0.592 | 8.11 m | 10.63 m |
| 3.00 g | 4.201 m/s | 0.691 | 8.57 m | |
| 4.00 g | 4.851 m/s | 0.760 | 8.90 m | |
| 5.00 g | 5.424 m/s | 0.760 | 8.90 m | |

## Phantom Practice throw

The host regression drove the strap and title screens with A or 2 against the 0.1.5 code. It reproduced one counted weak throw when the first play report still held B from the menu and another when the active remote's first report arrived as B held after connecting. A B-up as the first active report and accelerometer motion with B up were clean. The original log is `phantom-regression-0.1.5.log` in the artifact directory.

The title's confirm event starts a match, and the input is then disabled during the first turn delay. Both `input_next_turn` and the transition to `INPUT_PLAY` reset the detector without requiring B-up. The first valid play report with B held therefore gripped even though there was no fresh press. Releasing that grip launched the normal weak fallback, which `after_throw` counted when the bag settled. A/2 confirms the menu, and the code path that launches the bag requires B.

The opening turn now carries the B-up requirement across those resets when the assigned remote's B is held or unknown. All four startup cases stay at zero bags and zero counts until a fresh B-down and B-up in play. The first moving B-down grips on that report. The back-to-back trace suite also verifies that a B press held over from a later bag's flight can still grip the next bag.

## Verification and artifacts

- `make -j1 -C wii host-test` passed 36 input tests, 12 throw-trace tests, 12 WPAD tests, 938 physics golden checks, 2385 game checks and 209137 party checks. Generated-file checks and `wii/tools/check-no-ir.sh` passed.
- `make -j1 -C wii release` and `make -j1 -C kinhank` passed with the specified devkitPro and WIT. The ISO's extracted DOL matched the built DOL. The HBC ZIP passed its CRC check and carries version 0.1.6 in `meta.xml`. The Kinhank ZIP passed its CRC check and contains the exact delivered ISO plus the 3× profile with real Wii Remotes enabled.
- Dolphin 2503 passed the buttons-and-accelerometer movie at 3× with Vulkan. Practice opened at zero throws, then one B-up at 0.696 power scored a cornhole. The help panel and 1+2 overlay each toggled twice, and the DSP warning scan passed. `dolphin-e2e/strap.png` and `dolphin-e2e/aim.png` show v0.1.6. The last reported rate was 59.9 fps in emulated time. The median wall-clock rate in Practice frames 720–1440 was 28.96 fps on this host. The replay was rerun after the first launcher received SIGTERM before the scripted toss. Both launched Dolphin and Xvfb pairs exited.
- The ISO, Kinhank ZIP, both checksum files, build logs and Dolphin output are in `/workspace/scratch/wii-0.1.6/`. Both published checksums passed `sha256sum -c`.
  - `BackyardCornhole-BYCEAO.iso` SHA-256 `7997e7bbd3d60f25b2b0be9f621e767de6a86aac95bfa9e9d0216a2d610edbc0`
  - `BackyardCornhole-Kinhank-v0.1.6.zip` SHA-256 `f148f6bcee5a4665c8e11cb63da9c8fa62f10d48928374317af917cfaa94af8f`
