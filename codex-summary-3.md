# Cornhole batch 3

Implemented TASK_3 in `index.html`, with its controls and modes taking precedence over earlier specs. Changes remain in the working tree; nothing committed or deployed.

- Compared the v2 source at `af8250c`. The aiming view now uses a tighter 7.5° desktop field of view, adapts for portrait screens, and keeps its aim framing steady. The board occupies over 20% of desktop viewport width. Flight framing still widens for high bags.
- Direct pointer aim casts the pointer onto a plane at board height and derives the throw direction from that point. Mouse movement and captured finger drags both update aim continuously. Finger movement sideways before an upward flick is covered by the mobile test. Optional step and keyboard aim remain available.
- Large PUSH / NORMAL / KOBE buttons replace ARC +/- and W/S. NORMAL is the default, the current choice is highlighted, H cycles choices, and the dotted trajectory responds immediately. Each human player keeps their own height and bag-side choice.
- STEP / AIM / SPIN buttons are hidden behind MORE. The bottom hint reads exactly “Flick up to throw · Point to aim · Pick PUSH / NORMAL / KOBE”. Mouse/touch press, flick up, release is the primary throw. Wheel and M spinner remain unadvertised fallbacks.
- The title menu has exactly three choices: 2 PLAYER, SOLO PRACTICE, START TOURNAMENT. Local play alternates Red and Blue humans. Practice has no opponent, zero wind, unlimited throws, RESET BAGS, cumulative hole/board outcomes and hit rate. Reset clears the physical bags while retaining the session tally. MENU returns to the three choices between throws.
- START TOURNAMENT retains the saved career ladder, Next Up portraits, CPU styles, rematches, circuit wind, retries, and champion ending.
- Early career and local play retain their original power windows. Starting in County Fair, the white sweet window changes position each match and shrinks by 22%. King's Court changes it each throw. Input power maps through that shifted window into the original physical power range, so the window affects actual throwing difficulty. The board-range marker and live flick preview use the same mapping. The trajectory remains a calm-air reference; wind and collisions can change the result.

## Verification

Python Playwright with headless Chromium, local HTTP server on port 8000:

- `tests/playwright_check.py`: PASS. Updated H cycling, desktop flick, no keyboard throw, step/spin, trajectories, collision pushes, fixed match wind, wheel fallback, spinner, touch sideways aim plus flick, mobile orientations, and sandboxed 480×360 iframe. No console/page errors.
- `tests/career_check.py`: PASS. Career start, scripted physics win/loss, retry, reload persistence, later circuit winds/rematches, champion, desktop/card/mobile. No console/page errors.
- `tests/task3_check.py`: PASS. Exact three menu modes, toggle labels/default/highlight, hidden advanced controls, direct aim sweep, large board framing, separate player choices, nine practice throws without an inning limit, practice reset/tally retention, early/late/rematch power-window differences, King's Court per-throw shifts, responsive HUD bounds.
- `git diff --check`: PASS.

Refreshed existing screenshots and added `shots/task3-*.png`. Inspected desktop practice and 480×360 career framing.

The available Python environment is `/tmp/cornhole-venv`; Chromium was missing initially and was installed under `/tmp/task3-browsers`. Reproduce with `PLAYWRIGHT_BROWSERS_PATH=/tmp/task3-browsers /tmp/cornhole-venv/bin/python tests/task3_check.py` (and the other two scripts) while serving this directory on port 8000.

Unrelated task/log/bundle files were left alone. `.git` was only read.
