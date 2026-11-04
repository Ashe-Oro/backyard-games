# Task 17 — owner feedback

## Investigation

Compared `251b9b9` (batch 12) with the starting live-v7 source using `git diff 251b9b9 HEAD -- index.html`.

- Batch 14 changed portrait framing from `(0, 2.05, 3.2)` with aspect-compensated FOV to `(0, 5, 14)` and 25° FOV. Despite narrowing FOV, moving the camera back made the distant board smaller. Desktop framing remained the batch-12/static-camera framing; batch 9 was already present at batch 12.
- Gravity, friction, release coordinates, launch equations, power windows, flick speed mapping, and CPU planning/rates did not change after batch 12. There was no physics difficulty regression to revert. The smaller target reduced visual precision. Batch 15 intentionally removed the touch-down aim jump and made the sweet-spot preview independent of flick speed; those improvements remain.

## Changes

- Removed the near-board creation and its only references. Target board, foul lines, lanes, pitcher's boxes, scoring and collision logic remain.
- Enlarged the shared bag render geometry by 1.3×, covering held, flying, landed bags and afterimages. Physics dimensions are unchanged. Soft bags visually tuck inward while entering the unchanged six-inch hole; the folded footprint fits inside its radius.
- Reframed portrait with 21.3° vertical FOV and desktop with 8° minimum FOV / a lower centered camera. This telephoto framing keeps both actual release positions in the lower foreground while making the target approximately 19% wider on portrait and 25% wider on desktop versus live v7. Camera placement/FOV depend only on viewport dimensions.
- Kept held-bag position, trajectory origin and actual launch position exactly equal. No camera movement during aim, touch-down preview changes, or changes to flick-only controls.
- Preserved collapsible bag options, wording, CPU ladder/characters, X Player Card tags and `vercel.json`.
- Updated older camera tests' obsolete exact framing expectations, retaining their static-camera, release, bounds and menu-clearance checks. Updated the old panel test to count visible option rows, and collapsed the mobile drawer before a legacy test flick (the iPhone profile has only 664px usable height). Added task-17 checks and made the suite runner use installed system Chrome when bundled Chromium is absent.

## Verification

- Served locally in the foreground with `timeout 1800 python3 -m http.server 8000`.
- Captured `shots/task17-{390x844,456x1024,844x390,1280x800}-{left,right}.png`, with touch enabled and DPR 2. Projection measurements are in `shots/task17-projections.json`.
- Task 17 checks release/preview equality, foreground placement, board size, folded bag/hole fit, static camera, vertical gesture aim stability and browser errors.
- Task 14's existing portrait menu clearance and desktop framing checks pass.
- All 16 `*_check.py` suites pass: the full runner passed 12 suites immediately; the four legacy suites passed after correcting obsolete camera/layout expectations and the drawer-covered touch coordinate. Reruns used the same runner worker and 150-second suite bounds. CPU calibration, career, scoring, flick controls, preview stability, drawer persistence, wind and responsive layout remain covered.
- `git diff --check` passes.

All commands use bounded foreground execution. Initial suite attempts exposed missing bundled Chromium; the runner fallback fixes that environment issue without changing application behavior.
