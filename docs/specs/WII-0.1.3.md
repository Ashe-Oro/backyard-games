# Backyard Cornhole Wii 0.1.3: throw like Wii Sports Bowling

## 1. Reference
- Original Wii Sports Bowling Wiimote handling is the reference. Ashe's words: "you start with the Wiimote pointed upwards, and then you press the B button and then give it a full back and forward swing, and you release during that forward swing."
- What Ashe saw in 0.1.2 on the Kinhank (Dolphin 5.0-19870 on Android, real Wiimote through a DolphinBar, so the Wiimote is passed straight through to the game): every button works, but hold B, swing, release B never throws. A gold "HOLD STILL" message flashes for about a second whether he holds still or not. "It shouldn't need to measure this every time." A photo of that screen is docs/specs/wii-0.1.3-hold-still.jpg.
- Second, smaller issue: at 1080p (Dolphin internal 3x) the layout is still the 640x480 4:3 layout, so the upper-left "BOARD 27 FT" inset overlaps the beer-pong players. On the arcade it doesn't overlap.

## 2. Details
Likely root causes I found in wii/source/core/swing.c (confirm them with tests, don't take them on faith):
- In SWING_READY, B only arms on the down edge, and only if stable() passes (5 deg / 0.10 g over 150 ms). If the hand is moving when B goes down, that edge is spent on hold_still and the swing never arms until B is released and pressed again. A real player is never that still.
- SWING_SETTLING needs 450 ms of quiet with B up before READY, after every reset.
- Any sample gap over SWING_GAP_MS (25 ms), a dropped flag, or a history overflow calls swing_reset(s, true), which also requires B up again. Real Wiimote reports through the DolphinBar can be bursty or jittery, so one late report kills the swing mid-throw.

What to build:
1. Grip on B down with no stillness gate. Pressing B always grips immediately. Use the current or recent gravity vector as the grip reference, whatever it is. Remove the HOLD STILL message, INPUT_HOLD_STILL and hold_still_ms entirely.
2. The intended pose is Wiimote pointing up, as in Bowling. Tolerate any start pose, and never require a calibration or settling step before each throw. Keep any one-time power calibration optional and out of the way.
3. Throwing: hold B, swing back, swing forward, release B during the forward swing, and the bag throws. Power comes from forward swing speed. Aim and spin stay as they are now: lateral motion aims, and wrist twist curls, mirrored for left-handed players.
4. Release B with no detectable forward swing (or almost none) and you get a weak lob, not nothing. Releasing B must never silently do nothing while it's the player's turn.
5. Robust input: a single late or dropped sample must not reset an armed swing. Bridge gaps of up to about 100 ms by interpolating. Only reset after a long gap (over 250 ms). A history overflow must drop the oldest samples, not reset.
6. Keep the 1+2 debug overlay. Add these lines to it: swing phase, B state, samples per second, largest gap in ms, and the last throw's peak speed and power, so Ashe can photograph it if anything is still off.
7. Widescreen: when the console aspect is 16:9 (CONF_GetAspectRatio), lay the HUD out for 16:9 (wider projection, HUD anchored to the real screen edges) so the board inset sits clear of the beer-pong table, as on the arcade. 4:3 stays as it is. This is secondary; finish the throw first.

## 3. Tools
- Repo worktree /workspace/cornhole-wii-0.1.3, branch wii/throw-fix-0.1.3 from main c4adb4b. Wii code is in wii/; the host-side tests and Makefile are there too (see the 0.1.2 repro tests from eb0602f).
- devkitPPC/libogc build as in WII-0.1.2.md. The Kinhank package builds with make -C kinhank.
- Don't touch arcade/, web/, Thistlebrook or anything outside this repo. No force-push, no pushing at all: local commits only.

## 4. How to work (blind critic loop, measurable targets)
First write failing host tests that replay recorded or synthetic Wiimote traces:
- (a) B pressed while the remote is moving.
- (b) Wiimote-up start, full back and forward swing, B released mid-forward.
- (c) The same as (b) with 30–60 ms gaps and jitter injected, and with 100 Hz and 200 Hz report rates.
- (d) A slow, gentle swing.
- (e) B pressed and released with no swing.
- (f) Two throws back to back with no pause between them.
Then fix the code until they pass.
Targets:
- All six throw (e gives a weak lob).
- 0 HOLD STILL code paths remain.
- Time from B down to grip is 0 ms after the press.
- No reset for gaps of 100 ms or less.
- The existing aim, spin and left-hand tests still pass.
- The full Wii test suite is green, and the ISO and DOL build cleanly.
After that, re-read this spec as a blind critic, list anything not met, and fix it.

## 5. Deliver
- Local commits on wii/throw-fix-0.1.3: tests first, then the fix, then widescreen. Bump the Wii version to 0.1.3. Add a CHANGELOG entry (Wii and kinhank/CHANGELOG), and put the same notes in the commit body.
- Built artifacts in /workspace/scratch/wii-0.1.3/: the ISO with its sha256, plus the Kinhank zip from make -C kinhank, which keeps the 3x per-game ini.
- A short WII-0.1.3-LOG.md at the repo root covering root causes confirmed, what changed, test results, and anything still risky on real hardware.
- Final message: pass/fail for each target, commit hashes and artifact paths.
