# Cornhole batch 6
## Camera follows aim (Ashe, Nov 3, 3:29 PM)
- When the player moves the mouse/finger to change aim (and step), the camera should smoothly pan and turn with it: shift the camera position sideways with the bag/stand position and rotate the look-at toward the aimed line, so the view re-centers on where you're aiming (keep it gentle, with no motion sickness, and the board always in frame). Keep the v2 base framing (CAM_POS 0.5,2.05,2.45; look-at 0,0.5,-8.7) when aim is centered.
- Fix: in Solo Practice / 2 Player the top-left name tag reads "RED RED".
- Run all Playwright suites, write codex-summary-6.md. .git is read-only; leave changes in the working tree. Don't hang on a failing test: cap waits and move on, then report.

## Left-side bag menu (Ashe, Nov 3, 3:32 PM)
- Bug: clicking MORE makes the STEP/AIM/SPIN buttons render on top of and behind SLICK SIDE DOWN / MORE (overlapping). Fix the layout so nothing overlaps.
- Move ALL bag options into ONE tidy left-side panel/menu: throw height (PUSH / NORMAL / KOBE), bag side (SLICK / STICKY), spin (L / 0 / R) and step (if kept). Remove the bottom-center PUSH/NORMAL/KOBE bar. Clear, labeled rows, current choice highlighted, no overlap at desktop, 480x360 card or phone sizes.
- Remove the bottom hint line ("Flick up to throw · Point to aim · Pick PUSH / NORMAL / KOBE") entirely.
