# Task 10: KOBE power sweet spot

Implemented the KOBE meter change in `index.html`.

- KOBE's input sweet spot is 78% of the base NORMAL window for the same thrower and bag side. For SLICK, NORMAL spans 15% of the meter and KOBE now spans 11.7% (previously 3%).
- Later tournament rounds retain their randomized shift and 0.78 shrink multiplier. This produces a KOBE width of 60.84% of base NORMAL, with an explicit 60% minimum.
- Human KOBE input maps continuously onto its existing calibrated physics range, including outside the target. The displayed solid target uses the exact scoring-input bounds. The dashed on-board range is also mapped to KOBE input coordinates.
- Removed the solid target's artificial 2% visual minimum and used an inset border so its rendered size matches its actual tolerance.
- PUSH/NORMAL power tolerances and tournament behavior remain unchanged. SLICK/STICKY physics, CPU behavior, and the batch-9 static camera remain unchanged.

Added `tests/task10_check.py` covering both throwers, both bag sides, all three heights, shifted/unshifted windows, KOBE width/floor, continuous power mapping at the boundaries, dashed on-board mapping, and rendered target dimensions.

Screenshots: `shots/task10-push.png`, `shots/task10-normal.png`, and `shots/task10-kobe.png`. Inspected the KOBE screenshot.

Validation uses `PLAYWRIGHT_BROWSERS_PATH=/tmp/pw-browsers /tmp/cornhole-venv/bin/python tests/run_all.py`, with waits limited to 8 seconds, navigation to 12 seconds, and each suite to 150 seconds. Full log: `/tmp/task10-tests.log`.

All nine Playwright suites passed: career, gameplay, task 3, task 5, task 6, task 8, task 9, task 10, and wind/flag. No application page errors were reported. `git diff --check` passed. Older suites' regenerated tracked screenshots were restored; the three task-10 screenshots are retained. No writes to `.git` and no commits.
