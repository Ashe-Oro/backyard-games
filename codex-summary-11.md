# Task 11: clearer modes, round terminology and CPU difficulty

Implemented in `index.html`:

- Three equal-width menu buttons with the exact requested subtitles in the existing pixel font, smaller and dimmer than their labels. Descriptions remain inside the buttons. Phone landscape subtitles are 9px; desktop subtitles are 11px.
- Replaced every player-facing INNING with ROUND, including the initial HUD, live HUD, summary, wash message and NEXT ROUND button. Internal names remain intact.
- Calibrated all eight opponents according to the final ladder in the task, which supersedes the earlier restriction to fights 1–2. Each opponent has power and angular scatter, a range bias, and (where appropriate) occasional wide releases. Steve uses only NORMAL throws with substantial aim/power error. Later slider, blocker, lobber, sniper and tactical push/knock/block behavior remain; Ted retains large power variance and wild releases.
- CPU prediction now uses the same 240 Hz physics step as gameplay, refines its coarse search and averages successful aiming solutions before applying scatter. Steve II retains low PUSH throws, using sticky fabric to permit controlled board stops.
- Human throwing physics, scoring, saves, circuit wind rules, batch-9 static camera and batch-10 meter/input mapping are unchanged.

## CPU-only measurement

`tests/task11_check.py` runs 400 independent throws per opponent (3,200 total), calling the production planner for every bag and the production launch/physics functions. Seeded runs cover eight wind bearings at 0/0/3/3/7/7/10/10 mph respectively. Each throw starts on an empty board with the CPU trailing 0–1, measuring standard aiming rather than intentional blocker placement or bag collisions. On-board rates exclude bags in the hole. Tactical match situations and random wind strengths can change these rates; this does not measure human match win percentages.

| Opponent | Throws | In hole | On board | Miss |
|---|---:|---:|---:|---:|
| Slidin’ Steve | 400 | 5.50% | 33.25% | 61.25% |
| Big Bag Barb | 400 | 14.50% | 35.50% | 50.00% |
| Kobe Kowalski | 400 | 24.00% | 35.50% | 40.50% |
| Sticky Vicky | 400 | 32.00% | 35.00% | 33.00% |
| Tailgate Ted | 400 | 37.25% | 32.25% | 30.50% |
| Slidin’ Steve II | 400 | 44.75% | 32.00% | 23.25% |
| Big Bag Barb II | 400 | 51.00% | 28.25% | 20.75% |
| Reign Supreme | 400 | 61.00% | 30.25% | 8.75% |

Steve: 22 holes, 133 board bags and 245 misses; zero KOBE throws. This gives a gentle first opponent, with most bags missing.

## Screenshots and validation

Saved and visually inspected all requested menu screenshots:

- `shots/task11-1280.png` (1280×800)
- `shots/task11-844.png` (844×390)
- `shots/task11-390.png` (390×844)

Automated layout checks verify equal button widths, at least 9px subtitles, no title/button overlap, and no subtitle or viewport overflow at all three sizes. Added round HUD and summary assertions, CPU rate checks and Steve's no-KOBE check. Tests use 8-second waits, 12-second navigation and a 150-second suite limit.

Validation used `PLAYWRIGHT_BROWSERS_PATH=/tmp/pw-browsers /tmp/cornhole-venv/bin/python tests/run_all.py --worker tests/<suite>_check.py` for the existing suites, each bounded to 150 seconds by the driver, and `.../python tests/task11_check.py` for the 150-second CPU/layout suite. The complete collection is also available through `tests/run_all.py`. Logs: `/tmp/task11-final.log` and `/tmp/task11-regression.log`.

All ten suites passed: career, gameplay, task 3, task 5, task 6, task 8, task 9, task 10, task 11, and wind/flag. No application page errors were reported. The old task-3 menu assertion now checks the main button text independently of subtitles; task 11 checks the exact descriptions. Additional focused UI validation covered the final round-summary and subtitle assertions (`/tmp/task11-ui.log`). `git diff --check` passed.

No writes to `.git`, no commits. Existing unrelated task/log changes are preserved. Older suites' regenerated screenshots are restored after validation; task-11 screenshots are retained.
