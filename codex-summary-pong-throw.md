# Beer Pong throw repair

## Root cause and history

The reported **upward mouse flick / touch swipe** does not launch on the original HEAD. Downward pull-and-release already works on desktop and phones; this is not a failure of all throwing.

Breaking commit: `aed719a4818a8ad10ccbaa8621594d816febba4f` (Use pull-and-release aiming for Beer Pong). `git log -p -- beer-pong` shows it removed upward flick handling and replaced it with a pointer state machine that only arms a throw for positive (downward) `dy`. Upward input remains `pending`, so pointerup discards it. It also added an explicit test asserting that upward flicks never fire. None of the four recent suspected commits introduced that behavior.

Verified the history in a headless browser by serving the historical Beer Pong HTML, CSS, and JS through Playwright routes: `PONG_REV=aed719a^` passes the new desktop and native-touch regression; `PONG_REV=aed719a` fails with zero shots. Original HEAD also fails separately on desktop and phone touch.

## Fix

Small change confined to `beer-pong/game.js`: recognize an upward `flick` gesture, reuse distance-based power and release handling, and aim in the swipe direction. Downward pulls retain their existing opposite-direction aim, power, cancellation, and positioning behavior. CPU/human swats, bounce physics and shot styles are untouched. This restores upward gestures with distance-based power rather than reintroducing the old velocity estimator.

Added `tests/beer_pong_throw_check.cjs`: actual mouse events at 1280×800 and native CDP touch at 390×844 must launch exactly once, aim right for a rightward swipe, and move toward the opposing cups over subsequent frames. It fails before the fix and passes after. Updated the old input test's deliberate no-upward-throw expectation; its pull/release and cancellation checks still pass.

Screenshots: `shots/pong-throw-{before,after}-{mouse,touch}.png`.

## Validation

Used `/usr/bin/google-chrome` through a temporary Playwright launch adapter because the bundled Playwright browser is absent. Static server: `python3 -m http.server 8012`. Commands:

- `NODE_OPTIONS=--require=/tmp/pong-chrome.cjs SHOTS=shots node tests/beer_pong_throw_check.cjs`
- Same test with `PONG_REV=aed719a^` (passes), `PONG_REV=aed719a` (expected failure), and unmodified HEAD before editing (expected failure for each input).
- Ran all 18 `tests/*_check.cjs` / `tests/*_check.mjs` suites.
- Ran all 16 Python `*_check.py` suites using the existing `run_all.py --worker` and `/tmp/cornhole-venv/bin/python`. These predate the home page split, so served `cornhole/` on port 8000 and ran workers from `cornhole/` so their `Path('index.html')` instrumentation reads the game. Screenshot side effects moved to `/tmp/pong-legacy-shots`.
- `git diff --check` passes.

JavaScript passes: bounce, main Beer Pong (practice, CPU, full 19-shot two-player win), feedback, input, natural bounce, CPU re-rack, new throw regression, framing, game UI, home, Cornhole swipe feedback, yard games, yard help, swat AI, yard rules.

Initial JavaScript failures: position (viewport/input timing, also fails against unchanged HEAD), human swat (late keyboard swat after resume), re-rack (90-second outer runner limit). Follow-up results below.

Python: `task13_check.py` passes. Existing unrelated Cornhole failures: career, playwright, task10, task11, task12, task14, task15, task16, task17, task3, task5, task6, task8, task9, wind_flag. Examples: first-visit help intercepts old tests' clicks; removed meter element in task10; outdated menu copy assertion in task11; obsolete instrumentation/preview assumptions. Cornhole production files are unchanged. The full legacy suite is therefore **not green**; those failures have not been hidden or repaired outside this task's scope.

Follow-up: human swat passes on rerun (also passes against original HEAD). Position passes after making its `point()` helper wait two animation frames so viewport resizing/camera updates finish before pointerdown. The unchanged HEAD position suite failed too, confirming that race predates this fix.

Final follow-up: re-rack passes with the outer limit increased to 240 seconds. All 18 JavaScript suites have now passed, including the position synchronization change and reruns of swat/re-rack. The 15 legacy Python failures listed above remain; one Python suite passes. No other games were edited.
