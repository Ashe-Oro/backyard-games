# Backyard Cornhole Wii 0.1.7

Implemented on `wii/power-phase-0.1.7` from `aa32726`. The Wii and Kinhank Makefiles, HBC metadata, and shared STRAP ON! and HUD version sources identify v0.1.7.

## Picker power

Preview and release use `clamp(0.55 * forward_peak / (full_speed * type_scale), 0, 0.76)`. The default shared reference is 3.343588 m/s. KOBE uses the denominator form `1/0.90`, which gives 90% of the previous linear power at the same speed. Every picker still has a 0.76 cap and reaches it at its own speed.

| Picker | Speed reference multiplier | Default cap speed | Default cap excess |
| --- | ---: | ---: | ---: |
| SLIDER | 1.30 | 6.006 m/s | 6.13 g |
| NORMAL | 1.20 | 5.544 m/s | 5.22 g |
| KOBE | 1/0.90 | 5.134 m/s | 4.48 g |

When the adapter flags clipping, release retains the measured forward peak and flags the throw. This avoids granting the cap from an unknown speed. SET POWER folds the NORMAL 1.20 scale into the calibration calculation and writes the shared reference. The other pickers continue to multiply that reference by their own factors. The save format and existing saved speed references carry forward.

## First touch from release

The deck spans **7.7796–8.9795 m** from release. Both versions were measured using the same centered SLICK throws in calm Practice, with the picker set through the D-pad and the launched bag stepped at 240 Hz. The g values are synthetic forward excess `|gforce| − 1`. A constant forward load isolates the mapping from report timing and clipping. All figures are first contact, before any slide or bounce.

| Picker | Excess | 0.1.6 power | 0.1.6 distance | 0.1.7 power | 0.1.7 distance |
| --- | ---: | ---: | ---: | ---: | ---: |
| SLIDER | 0.9 g | 0.378536 | 7.6142 m | 0.291182 | 6.8242 m |
| SLIDER | 1.5 g | 0.488688 | 8.0842 m | 0.375914 | 7.5990 m |
| SLIDER | 2.0 g | 0.564288 | 8.3933 m | 0.434068 | 7.8443 m |
| SLIDER | 3.0 g | 0.691109 | 8.9075 m | 0.531622 | 8.2544 m |
| SLIDER | 4.0 g | 0.760000 | 9.7933 m | 0.613865 | 8.5867 m |
| NORMAL | 0.9 g | 0.378536 | 7.0925 m | 0.315447 | 6.5212 m |
| NORMAL | 1.5 g | 0.488688 | 7.7470 m | 0.407240 | 7.3116 m |
| NORMAL | 2.0 g | 0.564288 | 8.0023 m | 0.470240 | 7.6356 m |
| NORMAL | 3.0 g | 0.691109 | 8.5662 m | 0.575924 | 8.0373 m |
| NORMAL | 4.0 g | 0.760000 | 8.9025 m | 0.665020 | 8.4559 m |
| KOBE | 0.9 g | 0.378536 | 6.5739 m | 0.340682 | 6.2241 m |
| KOBE | 1.5 g | 0.488688 | 7.2004 m | 0.439819 | 6.9323 m |
| KOBE | 2.0 g | 0.564288 | 7.6090 m | 0.507859 | 7.2928 m |
| KOBE | 3.0 g | 0.691109 | 8.2226 m | 0.621998 | 7.8565 m |
| KOBE | 4.0 g | 0.760000 | 8.5755 m | 0.718221 | 8.3657 m |

The host landing suite also checks 6.5 and 7 g, which both cap at 0.76 and land at 9.7933 m for SLIDER, 8.9025 m for NORMAL, and 8.5755 m for KOBE. These two higher values exercise the cap with unclipped synthetic inputs.

## Phase timing and cancellation

The detector uses a grip-relative tangential change to identify BACKSWING and a dip and recovery in the smoothed radial load to identify FORWARD. The model places a shoulder pivot 0.60 m from the remote and aligns the remote’s Y axis tangentially. Each report combines gravity, tangential force and centripetal force. The tests add hand tremor and sensor noise and quantize to the buttons-and-accelerometer report resolution. These are modeled trajectories, with physical performance pending a Wii Remote session. The sources for the signal review were the [WiiBrew accelerometer description](https://www.wiibrew.org/wiki/Wiimote#Accelerometer) and [Groveman’s Wii Bowling accelerometer example, fig. 3.12](https://www.barbara-lerner.com/WiiMotility/thesis_paper.pdf#page=31).

Timing is the first report displaying each phase minus its kinematic timestamp. The five traces use nominal 100 Hz reports and fixed noise seeds. Two also include ±2 ms jitter. The kinematic backswing starts at 600 ms. In these continuous swings, the zero-velocity reversal and forward start share the same timestamp. Fixture five matches the model’s tangential acceleration on either side of the reversal. The old and new classifiers were compiled against the same trace harness.

| Trace | True reversal / forward start | 0.1.6 BACKSWING | 0.1.7 BACKSWING | 0.1.6 FORWARD | 0.1.7 FORWARD |
| --- | ---: | ---: | ---: | ---: | ---: |
| Remote up | 1150 ms | +80 ms | +10 ms | +30 ms | +30 ms |
| Moving into the start pose at grip | 1150 ms | −390 ms | +21 ms | −700 ms | +31 ms |
| Hanging start | 1100 ms | +70 ms | +10 ms | −60 ms | +40 ms |
| Fast backswing, slower return | 1000 ms | +20 ms | +29 ms | +182 ms | +50 ms |
| Hanging start, matched reversal acceleration | 1100 ms | +70 ms | +10 ms | −50 ms | +40 ms |
| Mean absolute error | | 126.0 ms | 16.0 ms | 204.4 ms | 38.2 ms |
| Worst absolute error | | 390 ms | 29 ms | 700 ms | 50 ms |

The bounds are 30 ms after backswing onset and 50 ms after reversal for this timing corpus. The forward averaging window starts with the classified forward reports and requires 16 ms of samples. A forward window of at least **0.22 g excess** (about **1.14 m/s**) sustained for **24 ms** qualifies a release. B-up requires qualifying load within the last **140 ms**. A stationary tap, tiny wiggle, backswing-only release, or expired stroke returns to READY with zero throw events. The next deliberate throw still works. The core’s optional automatic finish and timeout use the same qualification rule. The opening-turn B-up gate and held-B grip on later turns remain covered.

## Verification

- `make -j1 -C wii host-test` passed 36 input tests, 14 throw-trace tests, 12 WPAD adapter tests, 938 physics golden checks, 2,451 game checks and 209,137 party checks. Generated-file and no-IR checks passed.
- Replayed the first retuned movie toss by decoding the generated packet data into the host game. It scored one Practice cornhole at 0.696326 power and a 5.080 m/s forward peak. The generator distributes the higher NORMAL load across Y and Z to stay within the encoded rails. Dolphin playback, release packaging, and Kinhank installation are outside this verification.
