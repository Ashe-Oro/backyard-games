# Backyard Cornhole Wii 0.1.2: the throw never releases, and remove the lanterns

## Reference
Ashe tested Wii 0.1.1 (tag wii-v0.1.1, main df417d0) on Nov 21, 2026 with a real Wiimote. It boots into the game, the Wiimote connects, and every button works except the throw. He holds B (the trigger), swings, lets go of B, and the bag is never thrown. The swing must copy Wii Sports Bowling's Wiimote handling literally, including the backswing: hold B, backswing, forward swing, release B. See wii/INPUT.md and the earlier memory rules in the docs there. Aim comes from the accelerometer swing direction, the D-pad moves the bag along the foul line, there is no IR pointer, spin comes from lateral movement and wrist twist, and there is a per-player handedness setting.

## Details
1. Find out why a real Wiimote swing with B held and then released never produces a throw. Don't assume the cause. My non-binding guesses: accelerometer thresholds or units that were tuned only against the emulated or fake Wiimote and that a real swing doesn't cross; the release being detected on the wrong edge or the wrong frame; the throw state machine needing a backswing it never sees; or B being read from the wrong field on real hardware (WPAD held versus down versus up, or Nunchuk/extension data). Fix the root cause so a natural Bowling-style swing throws every time, and a slow, short, or lazy swing still throws a weaker bag rather than nothing.
2. Add a fail-safe: if B is released after a forward swing that the game can't classify, still throw at a low power in the measured direction. Never silently drop the throw. If B is released with no swing at all, cancel cleanly and let the player try again.
3. Add a small debug overlay, off by default and toggled by holding 1+2 for two seconds, that shows the live accelerometer values, the B state and the swing phase, so the next real-hardware report can say exactly where it stops.
4. Remove the lantern lights along the bottom of the screen (wii/source/wii/scene.c). Only the Wii build changes. Check whether arcade or mobile have the same lanterns and only note it in the PR; don't change them.
5. Bump to 0.1.2 everywhere the version must match (wii/Makefile VERSION, hbc meta.xml, the in-game footer and CHANGELOG.md), and keep wii/BUILD.md and wii/INSTALL-WII.md current.

## Tools
devkitPPC/libogc toolchain and the Docker setup in wii/docker, the existing tests in wii/tests (input_test, wpad_test, game_test, run-input-tests.sh), and the headless-Dolphin emulated-Wiimote end-to-end test in wii/dolphin.

## How to work
Write failing tests first. Record realistic real-Wiimote accelerometer traces: a normal swing, a gentle swing, a fast swing, a lefty swing, and B released at the end of the forward swing. Use real-hardware value ranges from libogc and WiiBrew docs, not just whatever the fake emits. Show that 0.1.1 drops at least the realistic cases, then fix the code. Measurable targets: 100% of the realistic swing traces produce exactly one throw. A B tap with no swing produces no throw and no stuck state. The existing input and game tests still pass. The headless-Dolphin e2e throws a bag and it lands. A screenshot shows no lanterns.

## Deliver
Local commits on branch wii/throw-fix in /workspace/cornhole-wii-0.1.2: no push, no tag, no PR. Add a CHANGELOG entry for 0.1.2, and put the commit notes in the commit body. Put the built ISO/dol and the HBC zip in /workspace/scratch/wii-0.1.2/, plus a before-and-after screenshot and a summary of the root cause you found and how you verified it. On this shared box, limit parallel builds and stop any background jobs you started when you're done.
