# Backyard Cornhole Wii + Kinhank 0.1.11

This update starts at `ca1e4ed`, the merged 0.1.10 release. It gives each throw type a power table based on peak forward acceleration.

## Wiimote trace

Two 0.1.9 trace files cover 45.507 seconds on Wiimote 2. They contain one throw attempt and zero rejects. The capture omits the selected throw type, actual power and outcome, so one attempt cannot establish soft, comfortable and hard medians.

The total acceleration reached **4.808 g** in the 100 ms ending at B-up, **5 ms before release**. Z reached +3.58 g near its rail with Y at +3.15 g, and the report carried no clipping flag. The reconstructed 0.1.9 speed from the 40 ms radial-excess average was **4.570 m/s**. The release timing supplied no evidence for a late-release power penalty.

| Type | 0.1.9 reconstructed power / LAST SHOT | 0.1.11 at 4.808 g / LAST SHOT |
| --- | ---: | ---: |
| Slider | 0.740 / 97% | 0.709 / 93% |
| Normal | 0.760 / 100% | 0.760 / 100% |
| Kobe | 0.760 / 100% | 0.760 / 100% |

## Factory power tables

The shipped anchors use the FINDINGS starting estimates. All three types share a 1.25 g floor. The higher anchors shift up 0.30 g for Slider and down 0.20 g for Kobe from the Normal reference. The observed 4.808 g peak falls above the hard anchor for each type. Powers are controller values before Kobe's launch remap.

| Type | Floor g / power | Soft g / power | Medium g / power | Hard g / power |
| --- | ---: | ---: | ---: | ---: |
| Slider | 1.25 / 0.381 | 2.00 / 0.461 | 2.70 / 0.605 | 3.60 / 0.683 |
| Normal | 1.25 / 0.476 | 1.70 / 0.556 | 2.40 / 0.699 | 3.30 / 0.758 |
| Kobe | 1.25 / 0.546 | 1.50 / 0.626 | 2.20 / 0.732 | 3.10 / 0.760 |

The detector measures peak total g over the last 100 ms of the forward stroke through B-up. Peaks below 1.20 g reject and leave the bag held. Qualifying peaks up to the 1.25 g floor get minimum power. Power increases linearly between anchors, then at one-quarter of the medium-to-hard slope until the 0.76 cap. The clip fitter can contribute up to 1.2 times a type's hard anchor, or 4.32 g Slider, 3.96 g Normal and 3.72 g Kobe with factory settings. A larger measured peak still counts. Swing speed remains visible in diagnostics.

The factory Slider curve reaches its mathematical cap at 7.154 g, above the 6.15 g total implied by three nominal 3.55 g axis rails. Its soft, medium and hard placements all fall below those rails.

SET POWER uses the median of three comfortable throws to calibrate the Normal medium anchor, which defaults to 2.40 g. The resulting effort multiplier scales each type's anchor offsets from the shared floor. Save version 5 keeps the previous layout and stores this multiplier in the former speed-reference field. Earlier factory values become 1.0, and custom references become their prior m/s value divided by 2.45 m/s, bounded to the new multiplier range.

## Practice landings and rests

The host launched single calm, centered SLICK throws in Practice at the factory anchor powers, with physics at 240 steps per second. Distances are from release. The board begins at 7.78 m, the hole center is at 8.75 m, and the rear edge is at 8.98 m. First contact is measured at the deck plane. Medium Slider and Normal touch down before the hole and slide into it. Medium Kobe reaches the hole directly.

| Type | Throw | Controller power | First contact (m) | Final rest (m) |
| --- | --- | ---: | ---: | ---: |
| Slider | Soft | 0.461 | 7.960 | 8.326, deck |
| Slider | Medium | 0.605 | 8.556 | 8.770, hole |
| Slider | Hard | 0.683 | 8.880 | 8.912, deck |
| Normal | Soft | 0.556 | 7.972 | 8.249, deck |
| Normal | Medium | 0.699 | 8.616 | 8.770, hole |
| Normal | Hard | 0.758 | 8.891 | 8.922, deck |
| Kobe | Soft | 0.626 | 7.956 | 7.969, deck |
| Kobe | Medium | 0.732 | 8.760 | 8.771, hole |
| Kobe | Hard | 0.760 | 8.876 | 8.891, deck |

Hard Slider and Normal previously slid off the rear edge. This release caps human Slider and Normal tangential speed after landing retention at `sqrt(2 * (mu * g_normal + g_uphill) * budget)`. The rollout budget tapers from the soft power to 0.033 m at hard power and above. The Slider soft budget is 0.40 m on SLICK and 0.58 m on STICKY. The Normal soft budget is 0.30 m on SLICK and 0.21 m on STICKY. The cap applies above soft power. Hard shots stayed on the back deck for both sides and both tested aim offsets of ±0.012 rad.

## Verification and build

- `make -C wii host-test -j2` passed 21 input tests, 14 throw-trace groups, 13 WPAD tests, 938 physics checks, 2,523 game checks and 209,137 party checks. The generated constants, pinned oracle vectors and no-IR check passed.
- Built with devkitPPC r51 and libogc 3.2.1 from the pinned `devkitpro/devkitppc:20261102` image and WIT 3.05a r8638. Both release and payload checksum lists passed, including the manifests inside the ZIPs. All three ZIP integrity checks passed. The packaged `main.dol` matches `boot.dol`.
- Dolphin 2609a replayed two synthetic Normal/SLICK throws under Xvfb. `/workspace/agent-logs/wii-0111/last-shot.png` shows LAST SHOT 91%, HANGER! and 1 POINT at 0.689 power. `/workspace/agent-logs/wii-0111/debug-overlay.png` shows LAST PEAK 3.43 G, 3.74 M/S, LAST SHOT 100%, and 1 POINT at 0.760 power.
- `kinhank/dist/BackyardCornhole-BYCEAO.iso` SHA-256 is `e5ad8cf13fea8372537601218e0f6ebd47feb10ff500b95ed021846df31b27c3`. The Kinhank ZIP is `kinhank/dist/BackyardCornhole-Kinhank-v0.1.11.zip`.
