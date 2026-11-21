# Backyard Cornhole Wii 0.1.3 log

Spec: `docs/specs/WII-0.1.3.md`. Branch `wii/throw-fix-0.1.3` from main `c4adb4b`. Local commits only.

## Root causes (confirmed by tests)

The new trace tests (`wii/tests/throw_test.c`, `bowling_trace.h`) model a real arm swing with the remote pointing up at the start, plus hand tremor, sensor noise, 10-bit quantization and silent saturation near 3.6 g. They feed `input_feed` from the moment the turn opens. On the 0.1.2 code all 10 tests failed (45 checks), and the WPAD adapter test failed too. Output: `/workspace/scratch/wii-0.1.3/before-0.1.2-tests.txt`.

1. **Settle gate swallowed the press.** After every reset (each turn, button press or report loss), `SWING_SETTLING` needed 450 ms of quiet with B up. A B pressed during that time spent its down edge there, and READY armed only on a down edge. So the swing never armed until B was let go and pressed again. This alone failed every trace.
2. **The stillness gate is the HOLD STILL flash.** With 700 ms of settling granted (`throw-test settled`), 0.1.2 still refused grips. Hand tremor (±1.5°) plus sensor noise breaks "5° / 0.10 g for 150 ms" even when the player holds still, so pressing B gave HOLD STILL and no grip. B pressed while moving (a) never gripped.
3. **A continuous swing was never classified as forward.** Even when it did grip, the remote-up Bowling swing never threw. The 80 ms excess-g window never dropped below 0.35 g at the back apex (its lowest was 0.71 g, against peaks of 2.7 to 2.9 g). So the backswing and forward swing merged into one lobe. The classifier needed either a calm pause behind the body or a near-1 g backswing to call a stroke forward, and got neither. B-up went to PENDING and was cancelled 250 ms later (debug trace in the fix commit message).
4. **Any timing hiccup reset the swing.** A gap over 25 ms, a `dropped` flag, an invalid report or a history overflow reset the detector and required B up again. The WPAD adapter also rejected whole batches, B edges included, when timing looked off. And on a single 25 ms empty poll it called `input_report_loss`. The adapter test with 60 Hz polls, frame hitches, two lost reports and a 45 ms stall never threw.

## What changed

- **Commit `4136df2`: tests first.** Traces (a) to (f), gap, overflow and stall rules, aim and left-hand mirror, and WPAD jitter replay. They fail on 0.1.2 by design.
- **Commit `415b0fc`: the fix.**
  - B grips on the very report it goes down. There is no settle phase and no stillness test, and any pose works. The grip reference is the mean gravity over the last 100 ms. Roll comes from the grip, or, when the remote points straight up, from the first 60 ms of loaded swing.
  - Strokes split where the load dips below half its peak and rises again. The second and later strokes after the grip count as forward. A release mid-forward throws on that same report, with power from that stroke's peak speed.
  - Every B-up while gripped throws. With no forward stroke it is a weak throw at 0.12 to 0.35 power; a tap is the 0.12 lob. There is no hold timeout.
  - Gaps up to 100 ms are interpolated, gaps up to 250 ms are accepted, and longer stalls restart the motion measurement but keep the grip. Dropped flags are ignored, a report with invalid accel still carries B, backward stamps are clamped, and history overflow drops the oldest sample.
  - The adapter never rejects a batch or cancels on a gap.
  - `INPUT_HOLD_STILL`, `hold_still`, `hold_still_ms` and the HOLD STILL text are removed.
  - A turn that opens with B held grips at once. Pause, menus and buttons still need B up first, and `input_resume` arms the next press.
  - The 1+2 overlay shows accel, B, phase, reports/s, max gap, live and peak speed, and the last throw's peak speed and power.
- **Commit `6f304ac`: widescreen.** At 16:9 the 2D layer is 853.3 × 480 square units instead of a stretched 640. Centred and right-anchored HUD pieces follow the real screen. The arrow projects into the same space. The BOARD · 27 FT inset moves to the top-left corner (12, 28, 136 × 100), clear of the pong table and both players by projection (checked in `game_test`), and its camera uses the displayed aspect. 4:3 is unchanged.
- **Release commit:** version 0.1.3 (Makefile, meta.xml, footer, both kinhank and wii changelogs, BUILD/INSTALL/README), INPUT.md rewritten for the new rules, and a power-ordering test.

## Test results

- `make -C wii host-test`: input 36/36, throw-trace 12/12 (0 failed checks), WPAD 12/12, physics golden 938/938, game 2264/2264, party 209137/209137.
- Targets:
  - (a) to (f) all throw. (e) is a 0.12 lob, and the gentle swing (d) is a 0.25 weak throw.
  - Grip happens on the B-down report (0 ms).
  - Gaps of 26, 40, 60 and 100 ms keep the grip, and B-up after a 390 ms stall still throws.
  - The unchanged aim, spin and left-hand tests pass.
- Power rises with forward speed from both a hanging and a remote-up start: 0.28, 0.34, 0.62, 0.77, 0.92.
- DOL, ISO (`main.dol == boot.dol`), `make release` and `make -C kinhank` all build cleanly. The kinhank zip keeps `InternalResolution = 3` and `WiimoteSource0/1 = 2`.
- Dolphin e2e (`wii/test/run-wiimote-e2e.sh`, emulated Wiimote DTM, Vulkan, the 16:9 default) passes on the final ISO: boot shows 0.1.3 16:9, one B-up throw, a scored HANGER, and the 1+2 overlay toggles on and off. Screenshots are in `/workspace/scratch/wii-0.1.3/dolphin-e2e/`. `debug.png` shows the inset clear of the pong players and the new overlay lines.
  - The harness picks `dolphin-emu-nogui` by default. On this box that build doesn't play the DTM inputs, and the 0.1.2 ISO fails the same way. Run it with `DOLPHIN=/usr/games/dolphin-emu`.

## Still risky on real hardware

- **Nothing here was tried with a physical Wii Remote.** All traces are modeled. The next Kinhank test should photograph the 1+2 overlay after a throw: phase, reports/s, max gap, peak speed and power.
- **Full power is easy to reach.** With the default full speed (1.2 g excess), a brisk Bowling swing hits power 1.0. SET POWER calibration exists but is optional; if throws feel too strong, that default is the knob.
- **Power cliff.** Weak throws cap at 0.35 power and a full forward stroke starts near 0.56, so a forward swing just under the 0.35 g stroke threshold jumps in power.
- **A wobble could cut power.** If real hand wobble after the forward swing's lowest point dips the load below half its peak and back up, the new "stroke" would have a lower peak. Releasing then would throw softer. The traces don't show this, but a real hand may.
- **Early release throws too hard.** Releasing during the downswing from the remote-up pose, before the backswing, can count as forward if the grip itself came during motion (a second stroke), and throws a strong bag rather than a lob.
- **Silent saturation.** Above about 3.6 g the ADXL330 clips silently with the provisional 0..1023 rails. Peaks are then capped, not flagged.
- **Smaller inset at 16:9.** The 16:9 inset is smaller (136 × 100) than at 4:3. The help legend (toggled with 1) still covers the bottom-left of the yard in both aspects.
