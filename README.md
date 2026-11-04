# tailgate-games

Tailgate Games has a buildless home page at `/` and five static Three.js games:

- Backyard Cornhole: `/cornhole/` (served from `cornhole/index.html`).
- Backyard Beer Pong: `/beer-pong/` — Solo Practice, vs CPU, and same-screen 2 Player.
- Ladder Golf: `/ladder-golf/` — three rungs, three bolas, exact 21; optional bonus.
- Washers: `/washers/` — box-and-cup rules, three washers, exact 21.
- Lawn Darts: `/lawn-darts/` — Handly Cup scoring, two darts, first to 21+.

All five support practice and same-screen two-player games. New games also have CPU matches, first-visit rules, pause and practice clear. Shared new-game rendering and controls are in `yard-games/engine.js`; independently tested scoring is in `yard-games/rules.mjs`. Source rule links appear in each new game’s How to Play dialog.

Both games use distance-based pull-down-and-release throws: grab the bag or ball, drag sideways to position it, pull down for power and diagonally to aim, then release. Return to the starting point to cancel. Post-throw arrows and power feedback help adjust the next attempt. Cornhole retains PUSH / NORMAL / KOBE throw heights and bag surfaces. Both games are linked from their menus.
Beer Pong uses one throw per turn; touching the table ends the throw as a dead ball.

## Deployment

The GitHub repository is connected to the existing Vercel project `cornhole`
in the `ashe-oros-projects` team.

- Production: https://backyard-cornhole.vercel.app/
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
