# Cornhole Batch 6

Completed all items in CODEX_TASK_6.md.

- Camera gently pans with aim and step and turns toward the aimed line using frame-rate-independent easing. Neutral aim/step restores exact v2 framing: position `(0.5, 2.05, 2.45)`, look-at `(0, 0.5, -8.7)`. FOV and depth remain unchanged. Bounded offsets keep the board visible; flight no longer pulls the camera away from this framing.
- Removed duplicated static team-color text, fixing RED RED in Solo Practice and 2 Player (and BLUE BLUE).
- Consolidated height, bag side, spin, and optional step/aim controls into one labeled left-side panel. MORE expands within normal layout flow. Current selections are highlighted with aria-pressed; step position is displayed. Panel scrolling accommodates short screens. Menu interaction does not aim or throw a bag.
- Removed the bottom-center height bar and bottom hint entirely.
- Kept flick-only throws, physics, wind widget/animated flag, three modes, per-player settings, unlimited practice, and career progression.
- Career regression testing exposed a final-win HUD lookup past the end of the roster. Clamped the display-name lookup so the champion transition completes without a console error.

## Verification

All five existing Playwright suites and the new Batch 6 suite passed:

| Suite | Result |
| --- | --- |
| playwright_check.py | PASS; no console/page errors |
| career_check.py | PASS; win/loss, persistence, later circuits, champion, desktop/card/mobile; console errors empty |
| task3_check.py | PASS; modes, aim, per-player options, practice, reset, sweet spots |
| task5_check.py | PASS; v2 FOV/depth, foreground bag, menu/intro HUD hiding |
| wind_flag_check.py | PASS; widget, eight flag directions, calm/strong wind, responsive placement |
| task6_check.py | PASS; panel overlap, highlighted choices, names, removed hint, neutral camera, aim/step movement, board bounds |

Runs used 8-second default waits, 12-second navigation limits, and 80–150-second process caps. Initial browser launches failed because the default cache path was missing; using the existing `/tmp/pw-browsers` installation resolved this. The original Batch 3 width assertion required over 128 pixels; exact requested v2 framing measures 127.6 pixels. Updated it to a narrow 127–129 pixel range. Updated old hint/side selectors for the requested UI.

Added `tests/run_all.py` for reproducible bounded suite runs (start a local server on port 8000 first):

```sh
PLAYWRIGHT_BROWSERS_PATH=/tmp/pw-browsers /tmp/cornhole-venv/bin/python tests/run_all.py
```

New Batch 6 screenshots: `shots/task6-1280.png`, `shots/task6-480.png`, `shots/task6-390.png`, `shots/task6-844.png`. Existing suites also refreshed their screenshots. Visually inspected card and phone captures. `git diff --check` passed.

`CLAUDE_UI_REVIEW.md` was not edited. No commits or writes to .git; all changes remain in the working tree.
