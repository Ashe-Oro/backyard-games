# Backyard Cornhole Wii + Kinhank 0.1.10

The starting point is `b9ffe4e`, the merged 0.1.9 release. This update changes the Normal controller response following Ashe’s Kinhank playtest. The old full-power effort now gives the previous 75% shot.

## Normal power curve

In 0.1.9, let `r` be the peak swing speed divided by `FULL REF`. Clamp it to 1.30, compute `q = r / (r + 0.47 * (1.30 - r))`, and then compute power as `0.76 * q² * (3 - 2q)`. The old Normal cap was 4.527796 m/s. Multiplying only the Normal scale by 4/3 would put that same swing at ratio 0.975 and deliver 94.99% power. The old curve gives 75% at ratio 0.640156. A scale of 2.886920 would map the old full-power swing to that ratio and push the cap to 9.194840 m/s.

| Normal setting | 0.1.9 | 0.1.10 |
| --- | ---: | ---: |
| Factory `full_speed` (m/s) | 2.450000 | 2.450000 |
| Type scale | 1.4216 | 1.4216 |
| Factory `FULL REF` (m/s) | 3.482920 | 3.482920 |
| 75% ratio | 0.640156 | 1.300000 |
| 100% cap ratio | 1.300000 | 1.720115 |

Below ratio 1.30, Normal uses the old curve’s fraction of cap `p` and subtracts `0.25 * (max(0, p - 0.15) / 0.85)²`. This preserves the original curve through 15% and progressively compresses the rest, mapping the old 100% to 75%. From ratio 1.30 to the new cap, Normal adds the remaining 25% with a smoothstep of the normalized speed across that interval. Slider and Kobe retain the 0.1.9 curve, scales and cap ratios. `FULL REF` and the debug speed readings still use m/s. SET POWER analytically inverts the selected curve, including both Normal segments. Saved custom slot references carry over and continue scaling all three throw types.

The requested 4/3 multiplier would put true 100% at 6.037061 m/s. The existing clipped-shoulder fitter caps its reconstruction at `swing_speed(2 * 3.55 - 1)`, or 5.991022 m/s. Quantized, noisy host reports at both 60 and 100 Hz reconstructed that speed and released at 0.76. The shipped factory cap uses this measured maximum, 32.3% above the previous cap. The maximum is based on the detector and replay fixtures.

## Practice landings and rests

The host drove the real Practice launch path with a single calm, centered SLICK throw for each measurement. Distances are from release. Physics uses 240 steps per second.

| Release | Controller power / LAST SHOT | Peak swing speed (m/s) | First contact (m) | Final rest (m) |
| --- | ---: | ---: | ---: | ---: |
| 0.1.9 | 0.570 / 75% | 2.229613 | 8.017794, front | 8.298957, deck |
| 0.1.9 | 0.760 / 100% | 4.527796 | 8.897737, back | 9.471016, grass |
| 0.1.10 | 0.570 / 75% | 4.527796 | 8.017794, front | 8.298957, deck |
| 0.1.10 | 0.760 / 100% | 5.991022 | 8.897737, back | 9.471016, grass |

The host checks both powers to within `1e-9` controller-power units and both landing measurements within 1 mm of the baseline. It also verifies that LAST SHOT displays the same controller power at both points.

## Low-end resolution

The table compares the same 12 noisy, quantized replays per row at 60 and 100 Hz. Values are mean controller percent ± one standard deviation. The change costs 1.3 percentage points around 1.2 g and 14.6 points around 2.0 g. Each sampled step still clears 1.8 percentage points.

| Mean peak radial load | Normal 0.1.9 | Normal 0.1.10 |
| ---: | ---: | ---: |
| 1.206 g | 34.2 ± 0.9 | 32.9 ± 0.8 |
| 1.290 g | 43.2 ± 0.5 | 40.4 ± 0.4 |
| 1.394 g | 52.0 ± 0.4 | 47.2 ± 0.3 |
| 1.506 g | 59.3 ± 0.5 | 52.5 ± 0.3 |
| 1.601 g | 64.2 ± 0.5 | 55.9 ± 0.3 |
| 1.698 g | 68.9 ± 0.3 | 58.9 ± 0.2 |
| 1.796 g | 72.7 ± 0.2 | 61.2 ± 0.1 |
| 1.912 g | 76.6 ± 0.2 | 63.5 ± 0.1 |
| 2.028 g | 80.0 ± 0.2 | 65.4 ± 0.1 |

## Verification and build

- `make -C wii host-test -j2` passed 21 input tests, 14 throw-trace groups, 13 WPAD tests, 938 physics checks, 2,533 game checks and 209,137 party checks. The generated constants, pinned oracle vectors and no-IR check passed.
- Built with devkitPPC r51 and libogc 3.2.1 from the pinned `devkitpro/devkitppc:20261102` image and WIT 3.05a r8638. Both release checksum lists, both bundled payload lists and all three ZIP integrity checks passed. The packaged `main.dol` matches `boot.dol`.
- Dolphin 2609a replayed the same centered 4.62 m/s clipped swing on both versions. Power went from 0.760 and off the board to 0.572 and on the board. `/workspace/agent-logs/wii-0110/last-shot.png` shows Normal/Slick, LAST SHOT 75% and 1 POINT in v0.1.10.
- `kinhank/dist/BackyardCornhole-BYCEAO.iso` SHA-256 is `4680f1845c0a0bd3d6ed27cb50a938a8a80f8f80cf8ed76bd642a537ea080a4d`. The Kinhank ZIP is `kinhank/dist/BackyardCornhole-Kinhank-v0.1.10.zip`.
