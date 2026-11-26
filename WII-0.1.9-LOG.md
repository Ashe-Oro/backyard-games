# Backyard Cornhole Wii + Kinhank 0.1.9

Implemented on `wii/power-bar-0.1.9` from `25d34c6`. Both targets use the BYCEAO disc. The Homebrew Channel metadata, STRAP ON! screen and gameplay footer show 0.1.9.

## Swing power and saves

At the factory reference of **2.45 m/s**, controller power is `clamp(0.55 * peak_speed / (full_speed * type_scale), 0, 0.76)`. For the same power below the cap, SLIDER needs 30% more speed, NORMAL needs 15% more and KOBE needs 10% less than in 0.1.8. At the same unclipped speed, the respective power ratios are `1/1.30`, `1/1.15` and `1/0.90` until the cap.

The speed columns are in m/s. The arrows show 0.1.8 → 0.1.9. Power 0.50 gives the same distance within each type for a fixed stance, aim, curl and wind.

| Throw | Scale | Speed at power 0.50 | Speed at cap | Highest reconstructed speed in matched traces | Power at that speed |
| --- | ---: | ---: | ---: | ---: | ---: |
| SLIDER | 1.30 → 1.69 | 2.895 → 3.764 | 4.401 → 5.721 | 5.854 → 5.991 | 0.76 (100%) → 0.76 (100%) |
| NORMAL | 1.20 → 1.38 | 2.673 → 3.074 | 4.063 → 4.672 | 5.854 → 5.991 | 0.76 (100%) → 0.76 (100%) |
| KOBE | 1/0.90 → 1.00 | 2.475 → 2.227 | 3.762 → 3.385 | 5.854 → 5.991 | 0.76 (100%) → 0.76 (100%) |

The matched host comparison used noisy, quantized shoulder-pivot traces at 60 and 100 Hz, both rolls of both grips, 60–90° backswings, 75–90° follow-throughs, 370–500 ms forward durations and releases at 65–85° past bottom. The grid alone peaked at 5.330 → 5.991 m/s. The comparison also includes a phase-shifted 60 Hz replay of a rolled in-line grip with −88°/+89° travel, 578 ms back and 438 ms forward, +79° release, −5° bottom wrist pitch and noise/jitter seed 798052073. That replay reached 5.854 m/s in both versions. The detector measures speed independently of the picker, so each version's maximum is shared across types. Physical Wii Remote and DolphinBar swing feel still needs hardware validation.

For a clipped throw, the estimator can use the additional falling shoulder through B-up and centers the fit on a clipped point recorded by B-up. Unclipped samples on both sides of the crest are still required. Reports after B-up can confirm the release window. Regression replays at both report rates reached the existing 0.76 cap for all three types. A calm, centered NORMAL cap throw first contacts the board 8.90 m from release.

Version 4 saves keep the factory 2.45 m/s reference and the stored per-player custom SET POWER value. The new scale is applied when computing the throw. Existing version 2 and 3 factory migrations still normalize `swing_speed(1.2)` and `swing_speed(1.9)` to 2.45 m/s.

## HUD and aim

- The upper-right teal **LAST SHOT** card uses a five-section color bar. The fill and label use `round(100 * power / 0.76)` clamped to 0–100% for the actual launched power, including the existing KOBE physics remap. The card stays through flight, scoring and the next aim phase until another bag launches. The combined arrow still previews and briefly displays aim and power.
- Hold **1+2** for the lower-right diagnostics. The overlay reads `SWING SPEED` and `PEAK` in m/s, shows the active slot's `FULL REF` for the selected type, and shows live preview percent and last-throw speed and percent. `FULL REF` is `full_speed * type_scale`. The gain and cap determine the cap speeds above. Accelerometer axes, pitch, phase, rejects, loads, fit, timing and learned axes remain available.
- The detector clears lateral accumulation at the backswing turnaround. Crooked-back, straight-forward traces produce zero aim for both grips at 60 and 100 Hz. Left-hand curl remains mirrored.

## Verification and build

- `make -C wii host-test -j2` passed 20 input tests, 13 throw-trace groups, 13 WPAD tests, 938 physics checks, 2,434 game checks and 209,137 party checks. Generated constants, oracle vectors and the no-IR check passed.
- Built with the devkitPPC r51 and libogc 3.2.1 packages from the `devkitpro/devkitppc:20261102` image and the checksum-verified WIT 3.05a r8638 archive. The Wii release and Kinhank package checksum lists and all three ZIP integrity checks passed. The ISO's extracted `main.dol` matched `boot.dol`.
- Local Dolphin replays of the release ISO captured `last-shot-16x9.png` after scoring, `debug-live-16x9.png` with the next swing's live speed, reference and preview percent, and `debug-4x3.png` with the second throw's speed and percent alongside expanded help. The PNGs are in `/workspace/agent-logs/wii-019/shots/`.
