# Cornhole batch 4

Implemented `CODEX_TASK_4.md` in `index.html`. Changes remain in the working tree; `.git` was only read. Batches 1–3 controls, camera framing, physics, modes, career progression, and power windows are preserved.

- Moved wind out of the top-center conditions line into a bordered, high-contrast widget on the right. Its continuous arrow points where the wind blows and its speed reads in MPH. Calm air hides the entire widget, including in practice and the first career circuit. Added an accessible direction/speed label.
- Replaced the static striped flag beside the hay bales with a subdivided, double-sided pennant. Its world-space animation follows the same wind direction used by bag physics, independent of the hay bales' rotation. Higher MPH extends the cloth farther, increases flutter frequency, and reduces ripple amplitude for a stiffer flag. Calm cloth droops with small static folds.
- Added deterministic wind/flag hooks alongside the existing test hooks and `tests/wind_flag_check.py`. Updated the career suite's old always-visible wind assertion to expect a hidden widget during its calm first-circuit checks.

## Verification

Served the working tree on port 8000 using Python's HTTP server. Ran Python Playwright with headless Chromium, `/tmp/cornhole-venv`, and `PLAYWRIGHT_BROWSERS_PATH=/tmp/task3-browsers`.

- `tests/playwright_check.py`: PASS; existing desktop/touch controls, trajectories, collisions, scoring, wind persistence, mobile orientations, and sandboxed iframe checks. No browser errors.
- `tests/task3_check.py`: PASS; modes, direct aim, framing, per-player choices, unlimited practice/reset tally, moving sweet spots, and responsive HUD.
- `tests/wind_flag_check.py`: PASS; all eight wind directions, MPH and arrow rotation, flag geometry alignment, animated flutter, stronger-wind extension/frequency/stiffness, calm droop and no flutter, practice hiding, and widget bounds at 1280×800, 390×844, and 480×360. No browser errors.
- `tests/career_check.py`: PASS; career win/loss, retry, save persistence, later circuit winds, rematches, champion ending, desktop/card/mobile. No browser errors.
- `git diff --check`: PASS.

Refreshed suite screenshots and added `shots/wind-1280.png`, `shots/wind-390.png`, and `shots/wind-480.png`. Visually inspected the compact 480×360 wind widget placement. Unrelated task and log files were left alone.
