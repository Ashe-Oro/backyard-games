# Cornhole batch 3 (run after career batch)
- **Shifting power sweet spot:** the power meter's sweet-spot box (the white "ideal power" window) should vary by career difficulty. In early circuits it stays at its current place and size. In later circuits and rematches, move it to a different position on the meter each match (or each throw in King's Court), and shrink it slightly, so players can't rely on muscle memory. Show the shifted box clearly. Quick Match keeps the current behavior unless a difficulty option says otherwise.
- Test with Playwright as before and write codex-summary-3.md.

## TOP PRIORITY: reverts requested by Ashe (Nov 3, 2:52 PM)
1. **Camera zoom regression:** in batch 1 the camera pulled back, so the board now looks tiny and far away. Restore the closer v2 framing (commit af8250c), or better, so the board is big and clearly the focal point.
2. **Aim regression:** restore the v2 aiming feel, where moving the mouse or finger points the bag straight at the spot you want on the board (direct pointer aim). Left/Right step is fine to keep as an extra, but the pointer must aim directly and smoothly like v2. On phone, dragging your finger sideways aims before the flick.
Compare to v2 with `git show af8250c:index.html`.

## SIMPLIFY CONTROLS (Ashe, Nov 3, 2:54 PM): "too complicated"
- Trackpad hint is wrong: Ashe clicks with one finger and swipes up. The primary throw on every device = press/touch, flick up, release. Remove the "two-finger swipe" hint text (the wheel throw can stay hidden as a fallback, but don't advertise it).
- Throw height: replace W/S plus the ARC +/- buttons with ONE big, obvious 3-way toggle on screen: PUSH · NORMAL · KOBE (tap to pick, NORMAL by default, the current choice highlighted, the dotted arc preview updating instantly). One key (e.g. Tab or H) cycles it.
- Cut the HUD clutter: hide the STEP/AIM/SPIN button grid by default (put it behind a small "more" toggle or show it only on touch devices), and shorten the bottom hint to one line: "Flick up to throw · Point to aim · Pick PUSH / NORMAL / KOBE".
- Labels must read exactly PUSH / NORMAL / KOBE (Ashe).

## MAIN MENU MODES (Ashe, Nov 3, 3:02 PM)
The title screen offers exactly three modes:
1. **2 PLAYER**: local hot-seat, Red vs Blue, both human, same device (pass and play), with each player's own side/height choices.
2. **SOLO PRACTICE**: no opponent and no wind, unlimited throws, a quick "reset bags" button, and a running tally of bags in the hole or on the board (with a hit rate).
3. **START TOURNAMENT**: the Punch-Out!!-style career ladder from CODEX_TASK_2_CAREER.md (Next Up cards, harder CPU opponents and rematches, wind later on, moving sweet spot). Fold any separate "Quick Match vs CPU" option into this or remove it.
