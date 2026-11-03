# Backyard Cornhole — feature batch (from Ashe)

Game: single-file Three.js 3D pixel-art cornhole at /workspace/cornhole/index.html (live at https://backyard-cornhole.vercel.app; deployment is handled separately — do NOT deploy). It already has: real ACL rules vs CPU / 2P hot-seat, cancellation scoring to 21, flick-to-throw (mouse press-flick-release), spinner mode (M, no click), slick/sticky side (F), on-board bag collisions, X Player Card meta + vercel.json CSP frame-ancestors (keep those intact). Keep it a single self-contained index.html, small payload, no build step.

Implement ALL of the following:

1. **Step left/right (position).** Left/Right arrow keys move the thrower's standing position sideways within the pitcher's box (reasonable range left/right of the board). This is SEPARATE from the existing aim-angle control, so there are two movements: position (arrows) and angle (mouse sideways / A-D). Move existing arrow-key aim to A/D only. Mobile on-screen buttons for both.
2. **Board farther out / power scale.** Light, easy flicks currently overthrow the board by too much. Re-tune so gentle flicks land short/near the board and it takes deliberate effort to reach the hole; board should feel farther out. Keep the hole window at least as wide as now.
3. **Bag-to-bag physics.** Proper pushes and slides: momentum transfer, bags sliding each other along/off the board, knock-ins, bags riding up onto/over other bags if feasible, low throws that push. Make it look and feel right.
4. **Spin.** Player can add slight left or right spin (e.g. Q/E keys + mobile buttons, and/or derived from sideways flick curve). On board contact the bag rotates and drifts just barely in the spin direction. Subtle.
5. **Throw height / arc.** Selectable arc from a high "Kobe" lob (steep drop, can clear blocker bags into the hole, little slide) to a low line drive/slider (flat, slides and pushes bags). E.g. W/S or up-down mouse gesture shape + mobile control; show current arc in HUD.
6. **Tighter pixel art + board framing.** Slightly finer pixels (raise internal render resolution modestly — "a little less blocky, just a little"), same style. Camera framed so the target board is bigger and the clear focal point of the scene (tighter FOV / closer framing), still showing enough scenery.
7. **Input support everywhere.**
   - Laptop touchpad: flick works without holding a button (tap-and-drag and two-finger swipe / wheel-delta flick are fine), plus spinner mode.
   - Phone: touch swipe-up to throw (swipe speed/length = power, sideways = aim drift), on-screen buttons for step, aim, spin, arc, slick/sticky; no page scroll/zoom while playing (touch-action none, viewport meta), works in portrait and landscape.
   - Playable inside an X Player Card iframe (~480x360 and smaller, sandboxed iframe; pointer lock may be unavailable — degrade gracefully).
8. **Wind.** A wind variable (direction + mph) chosen randomly at match start and constant for the whole match (same per game, not per inning). Shown as an on-screen indicator (arrow + mph). It pushes bags in flight proportional to flight time (high Kobe drifts more). Also add slight natural inaccuracy: sideways wobble in the flick adds a little random-ish curve so perfectly straight throws aren't trivial. CPU must also account for wind (imperfectly).

Update the on-screen hint/help text for the new controls (keep it compact; hide extras on narrow screens).

Testing (required): verify with headless Chromium via Playwright (python playwright is available on the box; install if needed). Load the page with no console errors at 1280x800, 480x360, and a mobile viewport (e.g. iPhone 13 with touch emulation). Script: a mouse flick throw, a touch swipe throw, step left/right changing position, spin and arc changes affecting landing, wind indicator visible and constant across innings, bag-on-bag push. Save screenshots to /workspace/cornhole/shots/ (desktop.png, card.png, mobile.png). Commit your work in git in /workspace/cornhole.

At the end write a concise summary of what changed, controls per device, test results, and rough edges to /workspace/cornhole/codex-summary.md.
