# Task 12 summary

Implemented in `index.html`:

- Re-tuned the eight CPU accuracy budgets to the requested ladder. The planner samples real simulated releases for hole, board and miss outcomes; live throws still use the unchanged flight, friction, collision and scoring physics. CPU release now uses its planned aim precisely. Ted samples a wider range of releases to retain high variance; Steve always uses NORMAL and never KOBE.
- Replaced the two repeated rivals with Sierra Spin (“The Cyclone”, Corkscrew Creek, 24–6, orange), who uses existing spin/curl physics, and Duke Dozer (“The Lane Clearer”, Granite Grove, 31–2, lime), who uses low sticky line drives and targets enemy blockers. Each has an original taunt. Existing career save key and fight indices are unchanged.
- Rebuilt all eight pixel portraits with individual hair shapes/colors, skin tones, accessories and clothing. Only Reign Supreme wears a crown. Inspected all eight intro screenshots for distinct silhouettes/colors. Removed rematch/II roster wording; retry button reads TRY AGAIN.
- Added LEFT / RIGHT practice buttons to the bag menu, with localStorage persistence and the existing RIGHT default. The held bag, pointer aiming, preview and actual release share one standing-position calculation. Removed hand bobbing so release has no positional jump.
- Centered the stationary camera and added portrait framing on resize so both boxes fit. Camera never follows aiming or side changes. Aim markers start at the actual hand origin and trace the ballistic preview instead of a disconnected ground line.
- Enlarged mobile bag controls to 44px/12px in portrait (60% width), and 36px/10px at 844×390. The landscape menu stays left of the board and the portrait menu stays clear of the power meter. Desktop menu styles are unchanged.

## CPU-only measurements

400 independent empty-board throws per opponent (3,200 total), seeded PRNG, eight wind bearings at 0/3/7/10 mph for the corresponding circuits. Every throw runs the real planner and launch/step physics at 240 Hz, including Sierra’s spin. No outcome or scoring overrides. Rates fluctuate with sampling, wind and bag interactions in full matches; Big Bag Barb’s measured board rate is 6.3 percentage points below the 35% budget in this sample.

| Opponent | Throws | In hole | On board | Miss | Target in / on |
| --- | ---: | ---: | ---: | ---: | --- |
| Slidin’ Steve | 400 | 6.2% | 28.5% | 65.2% | 7 / 30% |
| Big Bag Barb | 400 | 15.8% | 28.8% | 55.5% | 15 / 35% |
| Kobe Kowalski | 400 | 22.2% | 36.0% | 41.8% | 25 / 35% |
| Sticky Vicky | 400 | 34.5% | 36.0% | 29.5% | 35 / 35% |
| Tailgate Ted | 400 | 45.2% | 29.8% | 25.0% | 45 / 30% |
| Sierra Spin | 400 | 56.5% | 28.5% | 15.0% | 55 / 30% |
| Duke Dozer | 400 | 62.8% | 30.2% | 7.0% | 67 / 25% |
| Reign Supreme | 400 | 80.2% | 14.2% | 5.5% | 80 / 15% |

## Verification and artifacts

- `tests/task12_check.py`: PASS (400 throws each, distinct portrait markup, requested viewports, button dimensions/fonts, preview/held origin equality, static camera). Defaults: 8-second interactions, 12-second navigation, 150-second suite ceiling. Raw counts: `shots/task12-rates.json`.
- Actual live launch equality on both sides and LEFT/RIGHT persistence across reload: PASS; these checks are included in the final Task 12 suite and also passed separately during screenshot refresh.
- `tests/task9_check.py`: PASS, with updated fixed-camera coordinates and responsive framing expectations.
- `tests/task10_check.py`: PASS (meter geometry, power mapping, arc controls).
- Updated Task 11’s historical calibration targets and spinner simulation to the new ladder.
- Captured and visually checked `shots/task12-opp-1.png` through `task12-opp-8.png`, `task12-bag-{1280,844,456}.png`, `task12-practice-{left,right}.png`, and `task12-menu-{456x1024,844x390}.png`.

Browser checks used headless system Chrome through Playwright. No writes to `.git`.
