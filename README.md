# tailgate-games

Backyard Cornhole is a static Three.js game served from `index.html`.

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
