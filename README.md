# Backyard Games

One repo, four deployment targets, plus shared cornhole rules data.
The current test target is **Kinhank** (Dolphin on the Kinhank). See [TARGETS.md](TARGETS.md).

| Folder | What | Ships to | Changelog | Tags |
|---|---|---|---|---|
| [`web/`](web/) | Buildless static site: home page + five Three.js yard games (phones and desktops) | Vercel project `backyard-games` → https://playbackyardgames.vercel.app/ | [web/CHANGELOG.md](web/CHANGELOG.md) | as they are today (`web-vX.Y.Z` when cut) |
| [`arcade/`](arcade/) | Backyard Cornhole Arcade: Electron, trackball + buttons. The released 1.0.3 app is `arcade/cabinet-snapshots/1.0.3/` | The Batocera arcade cabinet (Ports) | [arcade/CHANGELOG.md](arcade/CHANGELOG.md) | `arcade-vX.Y.Z` |
| [`wii/`](wii/) | Native Wii homebrew port of cornhole (libogc/devkitPPC), 0.1.9 playable | Original Wii via Homebrew Channel zip and ISO (480p/480i) | [wii/CHANGELOG.md](wii/CHANGELOG.md) | `wii-vX.Y.Z` |
| [`kinhank/`](kinhank/) | Same Wii ISO plus Dolphin GameSettings at 3× (about 1080p) | Kinhank X5 Pro, installed by Retro over ADB | [kinhank/CHANGELOG.md](kinhank/CHANGELOG.md) | `kinhank-vX.Y.Z` |
| [`shared/`](shared/) | Cornhole rules data (geometry, physics constants, scoring, wind) + drift check. Data only | Not shipped; checked against web and arcade | — | — |
| [`docs/`](docs/) | Wii port spec, plan, repo layout, Wiimote mapping | — | — | — |

Root files: [TARGETS.md](TARGETS.md), `vercel.json` (`"outputDirectory": "web"` + CSP header), `.vercelignore`, and
[`CHANGELOG.md`](CHANGELOG.md) (an index; tags `v1.0.0`–`v1.0.8` are legacy arcade release names).

## Build and test

Every command runs from the repo root.

```bash
# web: serve locally (no build step)
python3 -m http.server 8000 --directory web
# web: the manifest runner serves web/ itself and runs web/tests/MANIFEST (24 modern checks must pass;
# 3 known pre-existing failures are reported separately). Needs playwright-core: npm ci --prefix arcade
node web/tests/run-manifest.mjs
node web/tests/run-manifest.mjs --baseline-ref a07afaa   # same checks on v1.0.3, side by side

# arcade: tooling tests, then the released 1.0.3 snapshot's tests
(cd arcade && npm ci && npm test)
(cd arcade/cabinet-snapshots/1.0.3 && node --test tests/*.test.mjs)

# shared: drift check (no dependencies; needs the a07afaa commit in the clone for the baseline re-derivation)
node --test shared/cornhole/tests/*.test.mjs

# wii (original hardware artifacts)
make -C wii host-test
make -C wii release

# kinhank (current test target): same ISO plus 3x GameSettings
make -C kinhank ISO=../wii/dist/BackyardCornhole-BYCEAO.iso
```

Don't publish arcade builds from `npm run build:linux` yet: the top-level `arcade/` builder still packages
older distance-power code. See `docs/wii-port/REPO_LAYOUT.md` §6 PR R.
