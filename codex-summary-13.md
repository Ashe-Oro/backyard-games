# Task 13 summary

Implemented only the two approved polish items in `index.html`:

- Changed the eight-direction `.ol` outline from fixed 2px offsets to .13em, and raised the HUD/panel font clamp floor to 10px. Existing responsive rules, wording and controls remain intact.
- Added three pooled, team-colored bag afterimages, sampled every 60ms and fading over 180ms. The effect copies existing transforms and uses no per-frame mesh, material, geometry or vector allocations.
- Added a separate pool of 14 plain tan pieces through `burst(at, true)` when a throw finishes on the board. The small puff fades over 400ms at the bag position. Existing 70-piece colorful hole celebrations are preserved.

Verification:

- `tests/task13_check.py`: PASS with 5-second interaction / 10-second navigation limits and a 45-second suite ceiling. Checks 390×844 and 456×1024 at deviceScaleFactor 2, opponent-card and recap bounds, the 10px HUD floor, visible pooled afterimages, 14 board pieces, puff expiration, and absence of hole confetti for a board score. No browser errors.
- Visually inspected phone opponent cards, HUD, mid-flight captures and recaps: readable small text and cancellation minus sign, no panel clipping or text overlap. Narrow phone recap text wraps within its panel.
- Batch 9 camera regression: PASS. Batch 10 meter/arc regression: PASS. Batch 11 menu/terminology and 3,200 simulated CPU throws: PASS.
- Batch 12 responsive controls, eight distinct portraits, release alignment, practice-side persistence and 3,200 simulated CPU throws: PASS.
- `git diff --check`: PASS. No writes to `.git`.

Screenshots: `shots/task13-phone-card-{390,456}.png`, `shots/task13-phone-hud-{390,456}.png`, `shots/task13-phone-recap-{390,456}.png`, `shots/task13-mid-flight-{390,456}.png`, and `shots/task13-board-puff-{390,456}.png`.

The board-puff check uses a controlled resting bag with the production throw-completion function; the mid-flight captures use a live throw. Existing unrelated workspace changes were left in place.
