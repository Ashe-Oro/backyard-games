# Task 16 summary

Implemented the collapsible bag menu in `index.html`.

- An attached pixel chevron handle slides the panel left in 150ms. The collapsed handle displays the current height and bag surface, and expands the same panel when tapped.
- The handle has a minimum 44px touch target on phones/coarse pointers and 32px on desktop. It supports keyboard activation, visible focus, `aria-expanded`, and `aria-controls`. Collapsed options are inert; reduced-motion preferences disable the slide.
- Expanded is the first-visit default. Both states persist under `backyard-cornhole-bag-menu-collapsed`; unavailable storage is handled without interrupting play.
- Only the panel and handle receive pointer events. The vacated game area accepts aiming and flick throws; existing pointer capture keeps gestures that start elsewhere working normally. The drawer is hidden with the HUD in menus and in poster mode.

## Verification

- `tests/task16_check.py`: passed at 456×1024 and 1280×800. Checks first-visit default, target sizes, current picks, off-screen position, inert controls, real CDP touch aiming and throw release in the vacated area, persistence after reload in both states, reopening and changing picks, storage-write failures, and browser errors. Interaction timeout 5 seconds, navigation timeout 10 seconds, suite limit 60 seconds.
- `tests/task15_check.py`: passed all height/surface/box combinations with exact idle/touch-start preview equality, vertical stability, horizontal aiming, cancellation, and immediate height changes.
- `tests/task14_check.py`: passed responsive portrait projections, menu clearance, board sizing, release alignment, static camera, and desktop framing.
- `git diff --check`: passed. No writes to `.git`. Existing regression screenshot artifacts were restored after the regression runs.

Screenshots:

- `shots/task15-menu-expanded-456x1024.png`
- `shots/task15-menu-collapsed-456x1024.png`
- `shots/task15-menu-expanded-1280x800.png`
- `shots/task15-menu-collapsed-1280x800.png`
