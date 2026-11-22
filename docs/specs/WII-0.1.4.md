# Backyard Cornhole Wii 0.1.4: swing = power + direction, picker = height

## 1. Reference
- Wii Sports Bowling. Ashe's rule (Nov 21 2026), passed on by Retro: "like Wii Bowling, the player can use D-pad left and right to move the bag's starting position left or right before throwing. Everything else, meaning speed, direction and height, comes only from the Wiimote's accelerometer. The pointer is for menus only."
- 0.1.3 (main feb6c8c, PR #63) already throws from the accelerometer and never enables IR (WPAD_FMT_BTNS_ACC). Keep it that way.

## 2. Details
Correction from Ashe (supersedes any earlier note): height must NOT come from the Wiimote. Height/arc is chosen only with the throw-type picker: lob / normal / slider, which is D-pad UP/DOWN today (slot->height, INPUT_HEIGHT, g->heights, g->arc; game.c ~260 and ~587). Keep that picker exactly as it is, working without the pointer, and make sure release pitch never affects the arc.

In gameplay (PH_AIM, human thrower):
- Remove the MINUS/PLUS fine-aim nudge (s->fine_aim and the AIM: +N label). Direction comes only from the sideways swing (swing.c lateral_sum, about line 272, and locked_aim, about line 302).
- Power stays the forward swing speed (swing_power(peak, full_speed), swing.c about line 226).
- Keep D-pad LEFT/RIGHT stepping the starting position along the foul line, exactly as now.
- Keep A (sticky/slick surface), 1 (help), 2 (reset), HOME (pause) and the 1+2 debug overlay. Menus keep their D-pad, -/+ and A/2 navigation; this change is gameplay-only.
- Update the in-game help text and the bottom help bar to match. For example: "B HOLD, SWING BACK & FORWARD, LET GO / SWING SPEED = POWER / SWING LEFT/RIGHT TO AIM / TWIST TO CURL / D-PAD UP/DOWN KOBE-NORMAL-SLIDER / D-PAD LEFT/RIGHT MOVE / A SURFACE / 2 RESET / 1 HELP / HOME PAUSE". Keep the lob/normal/slider indicator.
- Add these debug overlay lines: the last throw's power and aim.
- Show the version: draw a small "v0.1.4" on the Wiimote strap screen and in a corner of the in-game HUD (Retro couldn't read it over ADB).
- Update docs (docs/wii-port/WIIMOTE_ARCADE_MAPPING.md, TARGETS.md if it lists controls) to the new mapping.
- The arcade and web builds are untouched.

## 3. Tools
- Worktree /workspace/cornhole-wii-0.1.4, branch wii/dpad-only-0.1.4 from main feb6c8c.
- Build and tests as in WII-0.1.3-LOG.md. The Dolphin e2e needs DOLPHIN=/usr/games/dolphin-emu, and it must keep driving throws with buttons + accelerometer only (no IR bindings).
- Local commits only; don't push. Don't touch arcade/, web/ or anything outside this repo.

## 4. How to work (blind critic loop, measurable targets)
Write failing tests first, then implement.
Targets:
- In gameplay, -/+ change nothing about the throw (input test).
- Scripted swing-to-landing tests through the game physics: a faster swing lands measurably farther (three speeds, strictly increasing distance), and a left swing vs. a right swing lands left vs. right of center (mirrored for left-handed).
- The same swing with different release pitch gives an identical arc, and only the lob/normal/slider picker changes the arc (the three types give distinct trajectories).
- All 0.1.3 throw traces (a–f) still throw.
- D-pad left/right still steps the stance.
- No IR use anywhere in gameplay (grep plus the WPAD format test).
- Help text matches the controls.
- The version shows on the strap screen.
- The full Wii suite is green, the ISO, DOL and Kinhank zip build, and the Dolphin e2e throws and scores.
Then re-read this spec as a blind critic and fix any gaps.

## 5. Deliver
- Local commits on wii/dpad-only-0.1.4. Bump the version to 0.1.4, add CHANGELOG plus kinhank/CHANGELOG entries, and put the same notes in the commit body.
- Artifacts in /workspace/scratch/wii-0.1.4/: the ISO with its sha256, the Kinhank zip, and Dolphin e2e shots showing the new help text and the version tag.
- WII-0.1.4-LOG.md at the repo root.
- Final message: pass/fail for each target, commits and paths.
