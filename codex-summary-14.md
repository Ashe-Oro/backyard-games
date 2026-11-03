# Task 14 summary

Implemented portrait camera framing in `index.html`. Narrow portraits use a static camera at (0, 5, 14) with a 25° vertical FOV, looking at the existing board target. Framing blends into the original view between aspect ratios 0.60 and 0.85. Aim and box selection never change the camera. The held bag, real throw release and first preview point continue to share exactly the same world position.

At 1280×800 the camera position, orientation and 19° FOV match the previous framing exactly. Landscape framing and gameplay logic remain unchanged.

## Screen-space verification

Coordinates are CSS pixels, measured by projecting the real held mesh position through the active camera:

| Viewport | LEFT bag | RIGHT bag | Board width |
| --- | --- | --- | --- |
| 456×1024 | (131.86, 639.08) | (324.14, 639.08) | 61.86 px |
| 390×844 | (115.76, 526.74) | (274.24, 526.74) | 50.99 px |

Both sides are inside the center 50% horizontally and the lower 45% vertically, with at least 12 pixels of clearance above the menu. The board is fully visible, about 13% of viewport width. Visually inspected the 390×844 RIGHT screenshot to confirm bag visibility and framing.

Saved the requested screenshots:

- `shots/task14-456x1024-left.png`
- `shots/task14-456x1024-right.png`
- `shots/task14-390x844-left.png`
- `shots/task14-390x844-right.png`

Detailed projections: `shots/task14-projections.json`.

## Validation

- New `tests/task14_check.py`: PASS. Uses 5-second interaction and 10-second navigation timeouts with a 45-second suite ceiling. Checks both portrait sizes and boxes, menu clearance, board bounds and minimum size, exact release/preview alignment, static camera across aim extremes, exact desktop framing, and browser errors.
- Batch 9: PASS, including static camera and moving aim indicators. Updated the old phone camera position expectation for the newly required framing.
- Batch 10: PASS, meter/arc behavior and bounds.
- Batch 11: PASS, menus, terminology and 3,200 simulated CPU throws.
- Batch 12: PASS, responsive controls, portraits, saved practice box, release alignment and 3,200 simulated CPU throws. Updated its phone camera position expectation.
- Batch 13: PASS, phone text, panels and pooled throw effects.
- `git diff --check`: PASS.

Regression screenshot captures were retained under `/tmp/task14-regression-shots`; existing tracked screenshots were restored after verification. No writes to `.git`.
