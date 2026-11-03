# Backyard Cornhole feature batch

Implemented independent pitcher-box steps and aim, gentler flick power, slider/classic/Kobe arcs with matching meter guides, subtle spin/curve, match-long wind with imperfect CPU compensation, stronger equal-mass bag pushes and knock-ins/off-board slides, airborne blocker contacts and brief ride-up animation. Increased pixel resolution modestly and tightened board framing. Single HTML file, no build step; X Player Card metadata and vercel.json remain intact. Space charging is removed; keyboard input cannot throw.

Controls:
- Mouse: move sideways to aim; press, flick upward, release. M selects no-button spinner/trackball flicks; pointer lock is optional.
- Touchpad: tap/drag flick, two-finger upward wheel swipe, or M spinner mode.
- Phone: swipe upward through the play area; sideways swipe adds drift. Buttons handle step, aim, spin, arc, slick/sticky side, and mode. Portrait/landscape, touch-action none and viewport zoom restrictions are supported.
- Keyboard setup: Left/Right step, A/D aim, Q/E spin (opposite key returns through neutral), W/S arc, F side, M input mode. Space/Enter only start or advance menus.

Validation: headless Chromium via Python Playwright at 1280×800, 480×360, iPhone 13 touch emulation, mobile landscape, and a sandboxed 480×360 iframe. Assertions cover mouse flick, real touch swipe, wheel flick, unlocked spinner flick, position changes, arc flight time and spin landing differences, bag push, complete inning progression with unchanged wind, visible wind HUD, and no keyboard throw. Browser console/page errors: none. Screenshots: shots/desktop.png, shots/card.png, shots/mobile.png. Re-run with a local HTTP server on port 8000 and `python3 tests/playwright_check.py` after installing Playwright Chromium. This environment required HTTPS certificate bypass for external assets during browser tests.

Rough edges: fabric contact uses soft discs and a brief height lift rather than a deformable cloth/stack solver. Power guides show calm-air predictions; wind and stepping require player adjustment. Wheel delta sensitivity varies by hardware; touchpad tests emulate wheel input rather than physical hardware. Mobile testing uses Chromium emulation, not a physical iPhone. Existing remote Three.js/font dependencies remain. No deployment performed.

Git limitation: `/workspace/cornhole/.git` is mounted read-only, so the required in-place commit failed creating `.git/index.lock`. An isolated clone contains the commit; `cornhole-changes.bundle` preserves it for application in a writable checkout (`git fetch /workspace/cornhole/cornhole-changes.bundle HEAD`, then `git cherry-pick FETCH_HEAD`). The original workspace files contain all changes, but its Git history could not be updated under this environment's permissions.
