# Backyard Games

Backyard Games has a buildless home page at `/` and five static Three.js games:

- Backyard Cornhole: `/cornhole/` (served from `cornhole/index.html`).
- Backyard Beer Pong: `/beer-pong/` — Solo Practice, vs CPU, same-screen 2 Player, and the eight-rival Cup Circuit tournament.
- Ladder Golf: `/ladder-golf/` — three rungs, three bolas, exact 21; optional bonus.
- Washers: `/washers/` — box-and-cup rules, three washers, exact 21.
- Lawn Darts: `/lawn-darts/` — Handly Cup scoring, two darts, first to 21+.

All five support practice and same-screen two-player games. New games also have CPU matches, first-visit rules, pause and practice clear. Shared new-game rendering and controls are in `yard-games/engine.js`; independently tested scoring is in `yard-games/rules.mjs`. Source rule links appear in each new game’s How to Play dialog.

Both games use distance-based pull-down-and-release throws: grab the bag or ball, drag sideways to position it, pull down for power and diagonally to aim, then release. Return to the starting point to cancel. All five games share a power-colored arrow and faint full-power reference (`yard-games/power-arrow.js`): blue through green/yellow/orange to red. Arrow length scales with power, and release retains the same color and length. Cornhole reflects its calibrated physical throw power. Cornhole also gives power feedback. Cornhole retains PUSH / NORMAL / KOBE throw heights and bag surfaces. Both games are linked from their menus.
Beer Pong uses one throw per turn. Every mode allows one table bounce; a second bounce ends the shot. Every sink removes exactly one cup. CPU opponents mix arc and bounce shots. Swatting is disabled for both players via `beer-pong/features.mjs` (`FEATURES.swatting=false`), including hands, targets, cues and input. Each player gets one optional re-rack before throwing at 6, 4, 3 or 2 target cups. Practice refills restore the rack and re-rack allowance.

## Deployment

The GitHub repository is connected to the existing Vercel project `backyard-games`
in the `ashe-oros-projects` team.

- Production: https://playbackyardgames.vercel.app/
- Every push or merge to `main` automatically creates a production deployment.
- Vercel automatically assigns the production domains after a successful deployment.
- Other branches receive preview deployments through the Vercel Git integration.
- No separate build or dependency installation is needed for the static game.

Manage the connection in Vercel's **Settings → Git** and production branch tracking
in **Settings → Environments → Production**. Deployment status appears on GitHub
commits and in the Vercel dashboard.

This integration deploys the game; browser regression tests currently run locally
and are not an automated release gate.

## Shared scenery

All five games use `yard-games/backyard.js` for a deterministic voxel backyard: oak/birch trees, fences, plants, lanterns and party props. Static blocks are instanced by material. Scenery stays separate from camera, collision and scoring logic. Cornhole retains its board and bag art, animated dog and wind pennant. Shared grain textures and equipment details tie the lawn games together; target dimensions and scoring geometry are unchanged.

## Adding a game

Keep each game in its own directory, such as `cornhole/` or `beer-pong/`, with a menu link back to `/`. Add an `article.game-card` to the home page’s `.game-grid` in `index.html`, a 720 × 400 scene image in `assets/`, and a direct Play link. The responsive grid wraps automatically as games are added. `home.css` styles the shared home page independently of the games.

The primary Vercel address is https://playbackyardgames.vercel.app/. The previous backyard-cornhole.vercel.app address remains attached for existing links.

## Beer Pong opponent profiles and feature switches

Swatting code is parked behind the disabled release switch in `beer-pong/features.mjs`; changing that switch requires a deployment. `beer-pong/opponents.mjs` couples shooting accuracy and swat ability in overall opponent profiles (`rookie`, `regular`, `ace`, `boss`). `startMatch('cpu', profileId)` is ready for tournament opponents; the current vs-CPU match uses `regular`. There is no separate swat settings screen. Tune attention, reaction delay/jitter, hand speed, tracking interval, prediction, aim error and reach in those profiles. Attention gates noticing a bounce; a miss or hit is then determined by the fixed-step hand/ball geometry. `beer-pong/glove.js` renders the floating white glove at that same simulated position. CPU opponents mix arcs and physically planned bounce shots; bounce frequency belongs to the same overall opponent profile. There are no separate bounce/swat difficulty settings.

Original bounce reference (this game awards one cup per sink): [BPONG sections 4.2–4.3](https://bpong.com/wsobp/official-rules-of-the-world-series-of-beer-pong/). This game retains its existing one-shot turns and one optional re-rack house rules.

### Beer Pong tournament

The Cup Circuit has eight named rivals across four circuits, with increasingly accurate CPU throws and distinct pixel portraits. Wins unlock the next rival and save locally; losses allow rematches. Reloading restarts the pending match. A completed circuit awards the champion trophy, and New Tournament resets progress. If browser storage is blocked, the tournament remains playable for the current session.

Tournament rules match casual CPU play: ten cups, one bounce allowed, one cup per sink, no swatting, and one re-rack per player. Solo practice, casual CPU and pass-and-play remain separate modes.

Tournament checks: `node tests/beer_pong_tournament_check.mjs` and `BASE_URL=http://127.0.0.1:8013 NODE_PATH=/opt/homebrew/lib/node_modules/agent-browser/node_modules node tests/beer_pong_tournament_ui_check.cjs` against a local server.

Tournament tuning lives in `beer-pong/tournament.mjs`: each rival has shooting accuracy/spread and a `rimBounce` restitution value for their target cups. Pete opens with broad aim errors and very soft rims; rim bounce rises from 0.05 to the standard 0.30 by the final rival. Player-owned cups and non-tournament cup physics keep their standard response. Cup artwork/opening size, one-bounce/one-cup scoring and no-swat rules are unchanged.

### Cornhole challenge

Human pull power uses a continuous, steeper curve in `cornhole/controls.mjs`; the inverse keeps internal arc/surface calibration aligned. This roughly halves the normal-throw scoring tolerance without adding throw noise. Lob calibration remains usable on touchscreens. CPU releases keep their physical power values.

Matches share one crosswind for both players throughout a round. The next round switches its direction and chooses a fresh strength. Casual matches use 6–10 mph; tournament circuits progress through 4–6, 6–9, 9–12 and 12–15 mph. The existing wind arrow and pennant show the conditions. Practice remains calm. No aim sway, new controls, board-size changes or scoring changes.

Cornhole wind also drives faint world-space gust streaks (`cornhole/wind-streaks.js`). Density, length, speed and opacity scale with strength; calm weather and reduced-motion preferences hide them, and pausing freezes them. A separate seed and one draw call keep the effect independent of gameplay.
