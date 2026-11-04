# Backyard Games

Backyard Games has a buildless home page at `/` and five static Three.js games:

- Backyard Cornhole: `/cornhole/` (served from `cornhole/index.html`).
- Backyard Beer Pong: `/beer-pong/` — Solo Practice, vs CPU, and same-screen 2 Player.
- Ladder Golf: `/ladder-golf/` — three rungs, three bolas, exact 21; optional bonus.
- Washers: `/washers/` — box-and-cup rules, three washers, exact 21.
- Lawn Darts: `/lawn-darts/` — Handly Cup scoring, two darts, first to 21+.

All five support practice and same-screen two-player games. New games also have CPU matches, first-visit rules, pause and practice clear. Shared new-game rendering and controls are in `yard-games/engine.js`; independently tested scoring is in `yard-games/rules.mjs`. Source rule links appear in each new game’s How to Play dialog.

Both games use distance-based pull-down-and-release throws: grab the bag or ball, drag sideways to position it, pull down for power and diagonally to aim, then release. Return to the starting point to cancel. Post-throw arrows and power feedback help adjust the next attempt. Cornhole retains PUSH / NORMAL / KOBE throw heights and bag surfaces. Both games are linked from their menus.
Beer Pong uses one throw per turn. Arc shots are dead on table contact. In practice and human-vs-CPU, the human can choose a one-table-bounce shot: a made bounce removes the scored cup plus one defending-team choice (or only the last cup). A second bounce is dead. Only CPU opponents swat, only after the legal bounce, and only through actual glove/ball contact. Practice has no defense; same-device two-player remains arc-only for now. Each player gets one optional re-rack before throwing, at 6 cups (triangle), 4 (diamond/square), 3 (triangle/stoplight), or 2 (front-to-back line). The button disappears after confirming. Practice refills restore the original rack and a fresh allowance.

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

## Adding a game

Keep each game in its own directory, such as `cornhole/` or `beer-pong/`, with a menu link back to `/`. Add an `article.game-card` to the home page’s `.game-grid` in `index.html`, a 720 × 400 scene image in `assets/`, and a direct Play link. The responsive grid wraps automatically as games are added. `home.css` styles the shared home page independently of the games.

The primary Vercel address is https://playbackyardgames.vercel.app/. The previous backyard-cornhole.vercel.app address remains attached for existing links.

## Beer Pong opponent profiles

`beer-pong/opponents.mjs` couples shooting accuracy and swat ability in overall opponent profiles (`rookie`, `regular`, `ace`, `boss`). `startMatch('cpu', profileId)` is ready for tournament opponents; the current vs-CPU match uses `regular`. There is no separate swat settings screen. Tune attention, reaction delay/jitter, hand speed, tracking interval, prediction, aim error and reach in those profiles. Attention gates noticing a bounce; a miss or hit is then determined by the fixed-step hand/ball geometry. `beer-pong/glove.js` renders the floating white glove at that same simulated position. CPU shots stay arcs until human swat controls are designed.

Bounce reference: [BPONG sections 4.2–4.3](https://bpong.com/wsobp/official-rules-of-the-world-series-of-beer-pong/). This game retains its existing one-shot turns and one optional re-rack house rules.
