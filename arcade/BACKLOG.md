# Cornhole arcade backlog — November 6–7, 2026

Working candidate: **0.2.12**. Installed cabinet gameplay remains **0.2.11**, with the urgent virtual-hotkey exit repair. The website is separate and unchanged.

## Implemented locally

- Always-on live Board View; redundant menu toggle removed, including legacy saved-off behavior.
- Standard eight-rival tournament connected to arcade menus and button navigation, with the original progression/save rules.
- Reset Bag action, keyboard R and remappable cabinet binding; raw buttons 2/5 are fallback candidates. Resets only the ready bag and aim.
- Live and five-second last-shot power gauge beside the bag; same physical power conversion and color scale as the arrow.
- Slow lateral placement tolerates downward sensor noise; lower placement gain and short easing.
- Longer drive-style stroke candidate: 220 counts instead of 80, fresh full-power speed 4,000 counts/sec. Existing calibration retained. Stroke length adjustable in setup.
- Slick landing retention .32 and friction .43; all six arc/surface combinations retain usable windows. Sticky unchanged.
- Keg and guests relocated right of grill, top pump/hose/tap, corrected face-to-tap inverted pose.
- Chess opening with alternating lifted pieces and reaching players.
- Visible steaks/burgers, a cook, flipping food and periodic burger toss/catch for the dog.
- Playing-field flowers/planters removed; cacti placed at perimeter.
- Original cover, transparent marquee and gameplay art, with prepared native/favorite metadata.
- Cornhole exit waits for chord release; launcher cleanup handles its virtual Alt/F4. Batocera gameStop hook releases stranded virtual hotkeys across games.

## Verification

Passed locally:

- 27 unit checks: stroke separation and effort range, placement noise, settings, animation, exit cleanup, music library and web isolation.
- Packaged Electron at six resolutions (640×480 through 1920×1080): clear PIP/score/menu layout, gameplay/flight screenshots, real three-way dog celebrations, native input, remapping, calibration and playlist play/pause/next/persistence.
- Tournament map/intro/button navigation, loss/retry, pause/resume, all eight victories, crown, restart and native-menu return. Match results use test-only scoring fixtures; these are not human skill/balance playtests.
- Reset preserves thrown bags, scores and throw count; power gauge matches converted launch power, including last-shot display.
- Live Board View ignores the obsolete saved-off setting.
- Per-bag round recount, knock-ins/offs/dead bags; 6–4 resolves once to +2.
- Calm-air flight identical across optional spin; six arc/surface landing sweeps. Normal Slick usable samples increased 50→81 at the tested off-center aim; Slick still slides farther than Sticky, and a firm slider moves a blocker toward the hole.
- Correct face/tap alignment, alternating chess moves, food flipping and dog snack timing; scene freezes on pause.
- Actual web/standalone side-by-side check: normal web pull controls still work, no arcade requests in web, separate saves, accidental import rejected.
- Linux ZIP source hashes, version, launcher, recovery hook and media verified.

These checks use macOS Electron and local Chromium. They do not establish physical cabinet feel, Linux GPU endurance or the final installed picker appearance. Screenshots and physics sweeps: `outputs/arcade-backlog` in the conversation workspace. See `CONTROLS-AUDIT.md` for source references and the physical-test limits.

## Remaining / do not silently deploy a control redesign

- Physical cabinet comparison of slow corrections, full drive effort, chip/normal/hard rolls, and the two blue trackball button signals. New stroke defaults are a candidate until that playtest.
- Install reviewed gameplay build in a maintenance window, preserve profile/calibration, verify trackball/buttons and launch/exit.
- Install prepared media/metadata; mark native entry Favorite; preserve the obsolete Windows package outside scanned ROM folders, then remove only its duplicate entry. Check the actual game picker afterward.
- Recheck exit handling across Cornhole and other games after any future launcher update. Preserve `/userdata/system/scripts/90-release-virtual-hotkeys.sh`.
- Background music accepts a local playlist. No Final Fantasy soundtrack files were downloaded. Source owned/licensed files separately.

Nightly continuation: inspect this file and current Git changes before starting. Do not redo completed items or overwrite the hotkey repair. Report only new progress, blockers, or physical checks needed.
