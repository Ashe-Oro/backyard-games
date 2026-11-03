# Cornhole batch 8

Completed the four items in CODEX_TASK_8.md.

- Changed the scoreboard to three grid columns with equal outer tracks. Solo Practice stays centered when the blue team is hidden. Practice results use two lines: bags in/on board, then accuracy/throw count.
- At heights of 450px or less, current HUD/panel text is 10px, practice stats and bag labels/buttons are at least 9px, and bag-option/action buttons are at least 36px tall. Reduced padding/gaps and widened the compact options panel to fit the labels. Earlier batches removed the old hint/touch-control rows; those controls remain removed.
- Human throw height now synchronizes from the current thrower's saved height when rendering the HUD, including the between-turn wait. The highlighted button, meter, and M.arc agree with each player's selection. CPU shot planning remains intact.
- Replaced the Unicode wind arrow with a 12×12 canvas pixel sprite, displayed at 36×36 and rotated using the existing wind angle. The wind box now uses the button-style thick dark border and offset shadow. Direction/speed accessibility text is retained.

## Validation

All seven Playwright suites passed:

- career_check.py
- playwright_check.py
- task3_check.py
- task5_check.py
- task6_check.py
- task8_check.py (new)
- wind_flag_check.py

The new suite verifies practice centering/two-line stats at 1280×800, 390×844, 480×360, and 844×390; landscape button dimensions/font sizes; canvas sprite pixels; and independent PUSH/KOBE choices across four real alternating throws, including the between-turn wait. Existing suites cover career persistence/champion progression, physics/scoring, mouse/touch/wheel throws, iframe play, practice/reset, camera framing, removed controls, responsive layouts, and eight wind directions.

Inspected shots/task8-480.png: centered stats, readable controls, and pixel wind arrow. Suites refreshed existing screenshot artifacts and added shots/task8-{1280,390,480,844}.png.

Runner: `PLAYWRIGHT_BROWSERS_PATH=/tmp/pw-browsers /tmp/cornhole-venv/bin/python tests/run_all.py`, with a local server on port 8000. Default waits are 8 seconds, navigation timeouts 12 seconds, and each suite has a 150-second limit. Fixed the runner to apply these limits to Browser.new_page as well as BrowserContext.new_page.

The first attempt was interrupted by the local server stopping (connection refused). Restarted with the server owned by the validation process; the complete rerun passed. Final output: /tmp/b8-tests.log.

`git diff --check` passed. No writes to .git and no commits; all changes remain in the working tree.
