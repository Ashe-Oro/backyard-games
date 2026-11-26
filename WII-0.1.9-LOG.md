# Backyard Cornhole Wii + Kinhank 0.1.9

The follow-up builds on `e24a749` on `wii/power-bar-0.1.9`. Both targets use the BYCEAO disc, and the Homebrew Channel, strap screen and gameplay footer show 0.1.9.

## Distance calibration

Ashe’s 27 ft board on Kinhank with a DolphinBar played like 18 ft on Slider, 21 ft on Normal and 25 ft on Kobe. The corresponding distance ratios are 1.5000, 1.2857 and 1.0800.

The host sweep used calm, centered SLICK Practice at 240 physics steps per second. It recorded launch speed, first deck or ground contact, and final rest at 0.001 controller-power steps. Log–log fits on the middle half of the deck used 144 Slider, 137 Normal and 116 Kobe samples. A second run fit first contact against peak swing speed from noisy, quantized bowling traces, excluding capped points.

For distance `d ∝ v^n`, a distance ratio `R` corresponds to a speed ratio `R^(1/n)`. Multiply the 0.1.8 reference scale by that factor to give the old normalized effort at the adjusted speed. A quadratic fit would use `sqrt(R)`. A linear fit would use `R`.

| Throw | R | 0.1.8 scale | Earlier guess in e24a749 | Quadratic factor → scale | Linear scale | Measured controller power exponent → extrapolated scale | Measured launch speed exponent → shipped scale |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Slider | 1.5000 | 1.3000 | +30% → 1.69 | 1.2247 → 1.5922 | 1.9500 | 0.2784 → 5.578 | 1.3291 → **1.7637** |
| Normal | 1.2857 | 1.2000 | +15% → 1.38 | 1.1339 → 1.3607 | 1.5429 | 0.3493 → 2.464 | 1.4833 → **1.4216** |
| Kobe | 1.0800 | 1.1111 | −10% → 1.00 | 1.0392 → 1.1547 | 1.2000 | 0.4501 → 1.318 | 1.7257 → **1.1618** |

The launch speed fit had R² of 0.9973, 0.9985 and 0.9995, respectively. The 0.1.8 peak swing speed exponents were 0.2790, 0.3497 and 0.4456. The launch formula has a base of 5.6 m/s, a piecewise controller slope and a Kobe remap. In local terms, the controller exponent measures `d ln(distance) / d ln(power)`. Converting to launch speed divides by `d ln(launch_speed) / d ln(power)`, about 0.21, 0.24 and 0.26 near mid-deck. Using the raw controller fits as global divisors would extrapolate past the range of the fit and put the Slider cap out of reach. The shipped factors of 1.3567, 1.1846 and 1.0456 use the measured launch-speed exponents. Slider’s centered mid-board final rests mostly converged at the hole, so its first-contact distance supplies the fit. Final rests remain part of the playability measurements below.

The curve uses `r = clamp(peak_speed / (full_speed * type_scale), 0, 1.30)`, `q = r / (r + 0.47 * (1.30 - r))` and `power = 0.76 * q² * (3 - 2q)`. It eases in from zero and flattens approaching the cap. The old formula had no explicit power floor, but its 0.30 g qualification made the first valid power jump to about 30–36% of cap at 0.1.8 factory settings. The linear response, low 2.45 m/s reference and 0.55 gain let strong normalized inputs get near the long end quickly. The clipped-shoulder fit requires near-rail reports, so it did not inflate the unclipped soft fixtures. These checks cannot identify what drove Ashe’s actual swings long. The synthetic 0.1.8 soft throws mostly fell short. The revised curve brings those fixtures forward even though the feel-based factors reduce their power relative to the same new curve with the 0.1.8 scales.

The factory reference stays at 2.45 m/s. Save version 4 preserves exact custom SET POWER references and the earlier factory migrations from `swing_speed(1.2)` and `swing_speed(1.9)`. SET POWER now uses the inverse of the Normal curve to find the reference that sends the median valid toss to the SLICK Normal hole window.

## Low-end resolution

The first back-pass onset, duration and axis-learning gates and the two-report turnaround requirement carry over. The forward crest can now open at 1.12 g, qualify at 1.17 g and use a 0.09 g minimum drop on the lightest swings. The smoothed speed threshold falls from 0.30 to 0.14 g of excess. The 40 ms window already kept replay variance low. The fitted peak remains disabled for these unrailed swings.

The chart shows mean controller percent ± one standard deviation over 12 noisy, quantized replays per point at 60 and 100 Hz, including jitter and occasional repeated reports. The soft sweep checks that each step increases, each step spans at least 1.8 percentage points, and relative standard deviation stays below 6%. At 1.50 g, changing speed by ±10% changes power by approximately ±10%.

| Mean peak radial load | Slider | Normal | Kobe |
| ---: | ---: | ---: | ---: |
| 1.206 g | 25.5 ± 0.8 | 34.2 ± 0.9 | 44.0 ± 1.1 |
| 1.290 g | 32.9 ± 0.4 | 43.2 ± 0.5 | 54.1 ± 0.5 |
| 1.394 g | 40.6 ± 0.4 | 52.0 ± 0.4 | 63.6 ± 0.4 |
| 1.506 g | 47.2 ± 0.4 | 59.3 ± 0.5 | 71.2 ± 0.5 |
| 1.601 g | 51.8 ± 0.5 | 64.2 ± 0.5 | 76.0 ± 0.5 |
| 1.698 g | 56.3 ± 0.3 | 68.9 ± 0.3 | 80.4 ± 0.3 |
| 1.796 g | 60.0 ± 0.2 | 72.7 ± 0.2 | 83.8 ± 0.2 |
| 1.912 g | 64.1 ± 0.2 | 76.6 ± 0.2 | 87.2 ± 0.2 |
| 2.028 g | 67.6 ± 0.3 | 80.0 ± 0.2 | 90.0 ± 0.2 |

## Practice landings and rests

Each cell shows controller percent, first contact and final rest in metres from release. The deck spans 7.78–8.98 m and the hole is at 8.75 m. The peak column shows the modeled radial load before clipping.

| Throw | Peak | 0.1.8 | e24a749 | Follow-up |
| --- | ---: | --- | --- | --- |
| Slider | 1.80 g | 48%, 7.52 short / 7.70 short | 37%, 6.75 short / 6.91 short | 60%, 7.94 front / 8.30 deck |
| Slider | 2.50 g | 67%, 8.14 front / 8.77 in hole | 51%, 7.71 short / 7.75 short | 78%, 8.52 middle / 8.77 in hole |
| Slider | 3.63 g | 88%, 8.84 back / 10.00 long | 68%, 8.20 middle / 8.77 in hole | 92%, 8.97 back / 9.67 long |
| Normal | 1.80 g | 53%, 7.29 short / 7.41 short | 46%, 6.79 short / 6.90 short | 72%, 7.93 front / 8.21 deck |
| Normal | 2.50 g | 72%, 7.93 front / 8.20 deck | 63%, 7.68 short / 7.76 short | 89%, 8.52 middle / 8.77 in hole |
| Normal | 3.63 g | 96%, 8.74 hole / 8.77 in hole | 83%, 8.29 middle / 8.59 deck | 99%, 8.83 back / 9.39 long |
| Kobe | 1.80 g | 57%, 6.89 short / 6.94 short | 63%, 7.15 short / 7.21 short | 84%, 8.04 front / 8.05 deck |
| Kobe | 2.50 g | 78%, 7.75 short / 7.76 short | 87%, 8.04 front / 8.06 deck | 97%, 8.77 hole / 8.77 in hole |
| Kobe | 3.63 g | 100%, 8.57 middle / 8.59 deck | 100%, 8.57 middle / 8.59 deck | 100%, 8.88 back / 8.89 deck |

SLICK Slider’s landing multiplier goes from 1.35 to 0.90 for human throws. At controller power 0.595 and 10 cm to the side of the hole, the first contact stays at 8.52 m. The rest goes from 9.69 m off the deck to 8.93 m on the deck. Sticky retains its physics, as do Normal and Kobe bag contacts. Kobe stretches the upper end of its existing launch-power remap so the controller cap reaches the back. The LAST SHOT card now reads the controller percentage, matching debug, so Kobe’s physical remap does not fill the bar early. The compact arrow still shows physical power and forward-only aim.

All three types reached 0.76 (100%) in clipped traces at 60 and 100 Hz. The factory cap speeds are 5.62, 4.53 and 3.70 m/s. At 100% a centered Slider first contacts grass at 9.79 m and can reach the back at about 92%. Normal contacts the back at 8.90 m at 100% and can slide off. Kobe contacts the back at 8.88 m at 100% and stays on the deck.

Raw sweeps, baseline snapshots and the measurement runner are in `/workspace/agent-logs/wii-019/`. Run `measure_throw.sh /workspace/backyard-games-clean` there for the current tree, or pass `.012` as a second argument to reproduce the offset.

## Normal-play rendering and trace logging

The photo points to the dark-blue foreground bag mesh. Its fabric stripes and the bottom edge line up with the held bag. A desktop replay of the baseline did not reproduce the artifact, so the Kinhank-specific trigger remains unconfirmed. The renderer now clips the held bag draw to a 52 × 52 virtual-pixel box at its projected center and restores the full scissor before drawing the rest of the scene. Host coverage checks the held mesh’s projected bounds and the compact arrow.

Gameplay and SET POWER write buffered active-remote CSV reports to `sd:/apps/backyardcornhole/data/swing-trace.csv` with diagnostics closed. Writes drain only in READY or DONE, and the 1 MiB rotation keeps `swing-trace.1.csv`. Dolphin’s mounted SD showed a complete throw through DONE before the first 1+2 toggle. A seeded-card replay verified rotation and header handling.

## Verification and build

- `make -C wii host-test -j2` passed 20 input tests, 14 throw-trace groups, 13 WPAD tests, 938 physics checks, 2,523 game checks and 209,137 party checks. Generated constants, oracle vectors and the no-IR check passed.
- Built with devkitPPC r51 and libogc 3.2.1 from the pinned `devkitpro/devkitppc:20261102` image and WIT 3.05a r8638. Both release checksum lists, both bundled payload lists and all three ZIP integrity checks passed. The ISO’s extracted `main.dol` matches `boot.dol`.
- Dolphin 2609a scored every soft toss on the deck and every medium toss in the hole. The 3.87 m/s clipped toss slid off on Slider (92%) and Normal (98%) and stayed on the deck on Kobe (100%). Practice used Vulkan 1× for the Normal HUD captures and the Null backend for Slider and Kobe verification. The host distances above use isolated single-bag throws.
- `/workspace/agent-logs/wii-019/shots/normal-play-no-wedge.png` and `normal-play-no-wedge-16x9.png` show Blue’s Throw with Kobe/Slick in the main camera, diagnostics off, at Vulkan 3×. `last-shot-16x9.png` shows the scored soft toss and `debug-live-16x9.png` shows live speed, peak, `FULL REF`, live percent and the previous throw.
- `kinhank/dist/BackyardCornhole-BYCEAO.iso` SHA-256 is `14b90fca39a18a9e36409a86643402e3ae1f96c9e33b4389fd75154efc7f5238`.
