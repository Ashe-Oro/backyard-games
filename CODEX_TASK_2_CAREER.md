# Backyard Cornhole — Career mode (Punch-Out!!-style), from Ashe

Run AFTER the feature batch in CODEX_TASK.md is committed. Do NOT deploy.

Goal: a Mike Tyson's Punch-Out!!-style ladder. The player works through a lineup of named CPU opponents, each harder, to become the **Cornhole King** (champion).

- Opponent roster (~8–10 fights) in circuits (e.g. Backyard Circuit → County Fair → Tailgate Pro → King's Court). Each opponent: name, pixel portrait/intro card with a one-line taunt, bag colors, and a distinct play style (e.g. always-slider pusher, Kobe lobber, blocker who leaves short bags, sticky-side sniper, wild thrower). Original characters only — no real people or Nintendo IP.
- Difficulty ramps: accuracy (smaller random error), smarter shot selection (read the board: go for hole, Kobe over blockers, slide to push own bag in / knock opponent's off, leave blockers, play the score), and wind compensation.
- Rematches: some earlier opponents return later as stronger versions ("Tailgate Ted II"), like Punch-Out!!.
- **Wind only in later circuits** (none early, light mid, strong late); constant within a match. Quick Match mode keeps current behavior/options.
- Progress saved in localStorage (current fight, wins), simple circuit map/title screen, pre-fight intro card, post-fight win/lose card, champion ending screen.
- Keep single-file index.html, X Player Card + mobile + touchpad support intact; flick-only throwing (no keyboard throw).
- Test headless (Playwright): career start, beat/lose a fight via scripted throws, progress persists on reload, wind absent in early fights and present late, no console errors at desktop/480x360/mobile. Commit in git; summary to /workspace/cornhole/codex-summary-career.md.

- **(Added by Ashe) Between-fight intro pop-up:** before each career match show a "NEXT UP: <Name>" card with a small pixel-art portrait of that opponent (mix of men and women), their fun made-up name, nickname/hometown, record, and taunt. Fun, punny original names (e.g. "Big Bag Barb", "Slidin' Steve", "Kobe Kowalski", "Sticky Vicky"). Show again between every fight, including rematches (stronger-version portrait variant).

## Also include in this run (leftovers from batch 1)
- **Arc trajectory preview:** a dotted/pixel arc line in the 3D scene showing the planned flight for the current arc setting (slider/classic/Kobe), aim, step and spin (calm-air, no wind), so arc is clearly discoverable. Keep W/S + on-screen ARC buttons; do NOT move two-finger wheel away from throwing.
- **Board framing:** board is still small on screen. Frame the camera so the target board is noticeably larger and the clear focal point (roughly 2x current on-screen size) while the bag flight still reads.
- **Terminology:** never use "Woody" anywhere; bags on the board = "On the Board".
- Note: /workspace/cornhole/.git may be read-only for you; if commit fails just leave changes in the working tree (no bundle needed).
