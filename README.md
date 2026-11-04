# tailgate-games

Tailgate Games contains two static Three.js games:

- Backyard Cornhole: `/` (served from `index.html`).
- Backyard Beer Pong: `/beer-pong/` — Solo Practice, vs CPU, and same-screen 2 Player.

Cornhole uses upward mouse/touch flicks. Beer Pong uses distance-based pull-down-and-release throws: drag sideways to position the ball, pull down for power and diagonally to aim, then release. Return to the starting point to cancel. Both games are linked from their menus.
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
