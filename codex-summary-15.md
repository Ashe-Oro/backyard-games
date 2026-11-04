# Task 15 summary

Fixed the aim-path jump on touch-down in `index.html`.

- Both idle and touching previews now launch the same calm-air reference throw at the calibrated sweet-spot center for the selected height and bag surface. Live flick power still drives the meter and actual release, but no longer changes the preview.
- Pointer-down records the existing aim without changing it. Horizontal movement adjusts aim relative to that initial finger position; vertical movement preserves it exactly. Mouse hover aiming remains available.
- PUSH, NORMAL and KOBE immediately use their respective launch arcs. Existing dot positions, ground cutoff, release origin, camera framing and UI are preserved.

## Verification

`tests/task15_check.py` passes headless Chromium with mobile touch emulation at 456×1024. It uses real CDP touch-start/move/cancel events, 5-second interaction and 10-second navigation timeouts, and a 60-second suite limit. All 12 combinations of three heights, two boxes and two surfaces have exactly equal idle and immediate touch-start world positions and visibility for both dot arrays. It also checks vertical-motion stability, horizontal aiming, cancellation, and immediate distinct height arcs. No browser errors.

Captured positions: `shots/task15-preview-positions.json`.
Screenshots: `shots/task15-{push,normal,kobe}-{idle,touch}.png`.

Regression checks passed: tasks 8, 9, 10, 11, 12, 13 and 14, plus wind-flag checks, including CPU calibration, meter behavior, responsive controls, fixed camera, and release alignment. Career progression, wins/losses, persistence and champion checks also passed. The original batch runner encountered missing bundled Playwright browsers; affected suites were retried using the installed `/usr/bin/google-chrome` through a temporary runner outside the repository.

Older regression limitations: task 3 expects the previous screen-space box spacing, and task 5 expects the pre-Task-14 portrait camera z=2.45; those assertions fail against the existing Task 14 framing. The old general Playwright suite's iPhone touch coordinates overlap the current controls, so its horizontal-aim assertion fails; Task 15's real touch checks exercise the unobstructed play area and pass. Task 6 also expects two menu rows, predating the pitcher’s-box row. These older suites were left unchanged.

`git diff --check` passes. No writes to `.git`.
