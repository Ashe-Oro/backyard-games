# backyard-games — Repo Layout Proposal (web · arcade · wii)

Status: **PROPOSAL ONLY**, 2026-11-15 (rev 2: Codex review reconciled). Nothing has been moved, edited, tagged or retagged.
Repo: GitHub `Ashe-Oro/backyard-games` (local checkout `/workspace/tailgate-games`)
Related: [SPEC.md](SPEC.md) · [IMPL_PLAN.md](IMPL_PLAN.md) · [WIIMOTE_ARCADE_MAPPING.md](WIIMOTE_ARCADE_MAPPING.md)

Goal: one monorepo where each build is clearly separate:

| Platform | What it is | Ships to |
|---|---|---|
| **web** | The buildless static site: home page + 5 Three.js yard games, mobile and desktop browsers | Vercel project `backyard-games` (production `playbackyardgames.vercel.app`) |
| **arcade** | Backyard Cornhole Arcade: Electron, trackball + buttons | The arcade cabinet (existing Batocera Ports install). Unchanged by this proposal |
| **wii** | Native Wii homebrew (libogc/devkitPPC + WPAD) | Real Wii via HBC; Kinhank via Dolphin Android (ISO + GameSettings ini) |
| shared | Platform-agnostic cornhole rules data and golden test vectors | Not shipped by itself; consumed by the other three |

---

## 1. What the repo looks like today (inspected 2026-11-15)

```
/ (repo root = the web site root that Vercel serves)
  index.html  home.css  assets/  poster.png  screenshot.png        ← web home page
  cornhole/  beer-pong/  ladder-golf/  washers/  lawn-darts/       ← web games
  yard-games/                                                      ← shared web engine/UI (engine.js, rules.mjs, backyard.js, power-arrow.js, ui.css…)
  tests/      (19 .py Playwright suites, 23 .cjs, 4 .mjs; see test facts below)
  shots/      (11 MB of review screenshots, tracked; Python suites write here relative to their working directory)
  vercel.json (CSP headers only)   .vercelignore (one rule: `arcade`, plus a comment)
  CHANGELOG.md                     ← contains ONLY arcade releases 1.0.0–1.0.3
  README.md                        ← web docs + Vercel deployment notes
  ~80 tracked agent artifacts: CODEX_TASK_*.md, CLAUDE_*.md, codex-out-*.txt, codex-run-*.log, codex-summary-*.md, chain-*.log, claude-*.log, cornhole-changes.bundle
  arcade/                                                          ← Electron cabinet
    package.json (backyard-cornhole-arcade 1.0.3), build.mjs, build-linux.mjs, main.cjs, preload.cjs, controller.js, trackball.mjs, party.mjs, …
    tests/ (node --test *.test.mjs + .cjs browser/Electron checks)
    media/ (cover/marquee/gameplay art, gamelist entry)
    cabinet-snapshots/0.2.17 … 1.0.3/   ← the actual installed/released app resources per version
```

Facts that shape the proposal:
- **No CI.** There's no `.github/`. Vercel's Git integration deploys `main` to production with no build step; tests run by hand.
- **The arcade build depends on the web tree.** `arcade/build.mjs` copies `cornhole/{index.html,controls.mjs,wind-streaks.js}` and `yard-games/{backyard.js,power-arrow.js,ui.css}` from the repo root (`repo=path.dirname(here)`), then patches them into `arcade/dist/site/`. `arcade/tests/isolation.test.mjs` hashes the root-level web folders and reads the root `.vercelignore`.
- **The released arcade 1.0.3 is the snapshot**, `arcade/cabinet-snapshots/1.0.3/`. Its `DEV-NOTES.txt` warns that the top-level `arcade/` build tooling is older than the snapshot. Its tests run with `node --test tests/*.test.mjs` from the snapshot folder.
- **`.vercelignore` contains only `arcade`** (the `node_modules/`, `/dist/`, `.cache/` lines belong to `arcade/.gitignore`). So today every other tracked root file, including README, CHANGELOG and the agent artifacts, is uploaded to Vercel.
- **The arcade build tooling doesn't produce the released game.** `npm run build:linux` → `build-linux.mjs` → always runs `build.mjs --site`, which copies root web files plus the **top-level** `arcade/controller.js` and `trackball.mjs`. That top-level trackball still uses retired distance-based power (`power = forwardY / 400`), and lacks the snapshot's `spin.mjs`, legend, cooler removal and version bridge. Running it today would package the wrong gameplay, even though `package.json` says 1.0.3.
- **Web tests:** `tests/run_all.py` runs only `*_check.py` with cwd = repo root. Several legacy suites (e.g. `task6_check.py`, `task12_check.py`) read `index.html` and expect **the cornhole game at `/`**, but the root is now the five-game home page, so they're stale before any move. The `.cjs` checks need `playwright-core`, which only exists in `arcade/node_modules` (via `NODE_PATH`); some use `CHROME_PATH`. The four `.mjs` checks import `../cornhole/…`, `../yard-games/…`, `../beer-pong/…` relatively. Baseline pass/fail isn't recorded anywhere.
- **Tags:** `v1.0.0`, `v1.0.1`, `v1.0.2` and `v1.0.3` are annotated tags on the four "Release Backyard Cornhole 1.0.x" commits (table in §4.2). These are **arcade/Electron releases**. On 2026-11-15, `git ls-remote --tags origin` showed all four on GitHub. Re-check when the migration PR is prepared.
- **`v1.0.3` wasn't on `origin/main` as of the last fetch.** The cached `origin/main` ref ends at `c621a22` (1.0.2); `a07afaa` (1.0.3) is only on `release/1.0.3`. Branch refs are cached, not a live audit, so re-check before PR 0. The local `main` is stale (`c66b7dc`). Branches: `release/1.0.0` … `release/1.0.3`, many `codex/*` and `origin/codex/*` feature branches.
- Web URLs are root-relative (`/cornhole/`, `/yard-games/ui.css`, `/vendor/…` in the arcade copy), so whatever folder Vercel serves must keep the same internal layout.

---

## 2. Proposed top-level layout

```
backyard-games/
  README.md                 overview + "which folder is which" + per-platform build commands
  CHANGELOG.md              index only → web/, arcade/, wii/ changelogs (+ legacy-tag note)
  vercel.json               stays at root; adds "outputDirectory": "web" (see §6 step 3)
  .vercelignore             keeps `arcade`; adds wii, shared, docs, web/tests, node_modules, dist, .cache (§6)
  .github/workflows/        web.yml · arcade.yml · wii.yml (path-filtered, §5)

  web/                      ← everything Vercel serves today, moved as-is
    index.html  home.css  assets/  poster.png  screenshot.png
    cornhole/  beer-pong/  ladder-golf/  washers/  lawn-darts/  yard-games/
    tests/                  (moved from /tests; inventoried as modern vs legacy, §6; never deployed)
    CHANGELOG.md            new
    README.md               the web part of today's root README

  arcade/                   ← unchanged internal layout
    CHANGELOG.md            today's root CHANGELOG.md (git mv, history kept)
    build.mjs               path fix: read web sources from ../web (still the OLD builder; see §6 PR R)
    tests/isolation.test.mjs  web-root variable for file access, web-relative map keys kept
    cabinet-snapshots/…     untouched
    …

  wii/                      ← new (SPEC §1.3)
    Makefile  docker/  source/{core,wii}/  tools/  data/  hbc/  dolphin/  tests/
    CHANGELOG.md            new, starts at 0.1.0 (Unreleased)
    README.md

  shared/                   ← new, data + vectors only (no runtime code in phase 1)
    cornhole/
      common.json           only values verified identical across platforms
      overrides/{web,arcade-1.0.3,wii}.json   explicit per-platform differences
      golden/arcade-1.0.3/  oracle-harness vectors from the pinned snapshot (WIN, transitions, scoring, AI)
      README.md             who owns what, how to regenerate, drift-check rules

  docs/
    wii-port/               SPEC.md, IMPL_PLAN.md, REPO_LAYOUT.md, WIIMOTE_ARCADE_MAPPING.md (copied in from /workspace/docs)
    history/                (optional PR) tracked agent task files and logs, moved with git mv
    screenshots/            (optional PR) today's /shots
```

Naming: **`web/`**, not `mobile/`. The same static site serves phones and desktops; "mobile" would suggest a native app that doesn't exist.

### 2.1 What moves where

| Today | Proposed | Notes |
|---|---|---|
| `index.html`, `home.css`, `assets/`, `poster.png`, `screenshot.png` | `web/` | OG image URLs (`/assets/backyard.png`) are unchanged because `web/` becomes the served root |
| `cornhole/`, `beer-pong/`, `ladder-golf/`, `washers/`, `lawn-darts/`, `yard-games/` | `web/` | Internal paths unchanged |
| `tests/` | `web/tests/` | The `.mjs` relative imports keep working. Python suites write `shots/…` relative to their **working directory**, so the runner must set cwd and the output path explicitly; this needs a small code change, not just the move. `web/tests/` and its outputs are excluded from deployment (§6). Legacy suites that expect cornhole at `/` are already stale today; they're archived or fixed in a separate bounded patch, not in the move |
| `shots/` (tracked review images) | `docs/screenshots/` (optional, separate PR) | Pure history, not used at runtime |
| `CHANGELOG.md` (arcade 1.0.0–1.0.3) | `arcade/CHANGELOG.md` via `git mv`; a new root `CHANGELOG.md` index | `git log --follow arcade/CHANGELOG.md` keeps the history |
| Root `README.md` | Split: root overview + `web/README.md` (game list, Vercel notes, test commands) | |
| `vercel.json`, `.vercelignore` | Stay at root; contents updated | §6 step 3 |
| `arcade/**` | Stays | `build.mjs` web-root path + isolation-test web-root variable (keys unchanged). Not "two edits only": the Python runner/cwd changes above are also needed |
| Tracked agent artifacts (`CODEX_TASK_*`, `codex-*`, `claude-*`, `chain-*`, `CLAUDE_*`, `cornhole-changes.bundle`) | `docs/history/` (optional, separate PR) | Untracked local leftovers in the working tree are **not** committed |
| `/workspace/docs/cornhole/wii-port/*.md` | `docs/wii-port/` | Copied in by the Wii skeleton PR |


### 2.2 What stays shared, and how

The restructure PR shares **nothing new at runtime**. The web site stays buildless; the arcade's old builder keeps copying from `web/` exactly as it copies from the root today. Sharing comes later as **data + tests**, and only for values that really are equal.

**Web and arcade cornhole really differ today** (Codex review R2, verified in source), so a single "shared constants" file would be wrong on day one:

| Value | Web (`cornhole/index.html`) | Arcade 1.0.3 (snapshot) |
|---|---|---|
| Slick `mu / ms / keep` | `.30 / .5 / .42` | `.43 / .60 / .32` |
| Release height `REL_Y` | `1.3` | `.98` |
| Spin/curl | Older inline spin path; human spin suppressed at throw | `spin.mjs`: 14 rad/s visual spin, landing curl only, human spin enabled |
| Effective landscape FOV | `14 + 8·portrait` → 14° | `18 + 6·portrait` → 18° |
| Power input | Distance-based pull (touch/mouse) | Speed-based spinner |

So:

| Shared item | Contents | Consumers | Guard |
|---|---|---|---|
| `shared/cornhole/common.json` | Only fields verified **identical** across web and arcade 1.0.3 (e.g. board/hole/bag dimensions, `BOARD_Z`, sticky surface, `MU_GROUND`, `BAG_E`, arcs, wind tiers) | Drift check; Wii header generation | Each field lists the source locations it was verified against |
| `shared/cornhole/overrides/{web,arcade-1.0.3,wii}.json` | Per-platform values that differ on purpose (table above). **Wii baseline = arcade 1.0.3** | Drift check; Wii header generation | Versioned per platform release |
| `shared/cornhole/golden/arcade-1.0.3/` | Oracle-harness vectors (WIN, transitions, scoring, AI decisions) from the **pinned** snapshot, with source SHA-256s | `wii/tests` | Regenerated only when the pinned baseline changes |
| Drift approvals | `shared/cornhole/approvals.json`: structured entries `{field, platform, old, new, baseline, PR}` | Drift check | A drift fails unless an exact matching approval exists. Changelog text is documentation, not an exception mechanism |

Rules: restructuring **never changes web physics to match arcade** (or the reverse). The Wii's P0–P1 tools read the pinned 1.0.3 snapshot **directly** (SPEC §5.1.1), so the shared-data PR isn't a P0 prerequisite and stays after the restructure. Moving web or arcade JS to import from `shared/` would need a build step for the buildless site; that isn't proposed.

---

## 3. Per-platform CHANGELOGs

All use [Keep a Changelog](https://keepachangelog.com/) + SemVer, as the current root file does.

| File | Starts with | Version source |
|---|---|---|
| `arcade/CHANGELOG.md` | Today's root file verbatim (1.0.0–1.0.3), retitled "Backyard Cornhole Arcade — Changelog", with compare links to the legacy `v1.0.x` tags | The approved arcade release source after PR R (§6); today `arcade/package.json` and the snapshot's `resources-app/package.json` both say 1.0.3 |
| `web/CHANGELOG.md` | `## [Unreleased]`, plus a note: "Before web-v1.0.0, the site deployed continuously from `main`; see merged PRs." | New `web/version.json` (optional), or just the tag |
| `wii/CHANGELOG.md` | `## [Unreleased]` → first release `0.1.0` | `wii/Makefile` `VERSION` → `meta.xml` and the in-game pause footer |
| `CHANGELOG.md` (root) | Index: links to the three files + "Tags `v1.0.0`–`v1.0.3` are arcade releases (legacy names)" | — |

Rules (self-contained; no external policy file is needed):
- A PR updates the changelog of every platform whose shipped behavior changes. A `shared/` change that alters gameplay values is logged in **each** affected platform's changelog **and** gets a structured drift approval (§2.2).
- PR descriptions include the changelog lines.
- Release check: a platform tag `X-vA.B.C` must equal that platform's version source and a `## [A.B.C]` changelog heading (§5).

---

## 4. Tags and release branches

### 4.1 New tag scheme

`<platform>-v<semver>`, annotated, message = that version's changelog section:

| Platform | Examples |
|---|---|
| arcade | `arcade-v1.0.4`, `arcade-v1.1.0` |
| web | `web-v1.0.0` (first tagged web release), `web-v1.1.0` |
| wii | `wii-v0.1.0`, pre-releases `wii-v0.2.0-rc.1` |

Release branches (when needed): `release/arcade-1.0.4`, `release/wii-0.1.0`, `release/web-1.0.0`.

### 4.2 Existing `v1.0.0`–`v1.0.3` tags: mapped, never rewritten

| Existing tag (annotated) | Commit | Platform | Optional additive alias |
|---|---|---|---|
| `v1.0.0` | `d3002ea76884ed5b2ba16f4abc205669f4e36e0b` | arcade | `arcade-v1.0.0` |
| `v1.0.1` | `54be030e2eff3c074be5caf29085e680d15f2ab3` | arcade | `arcade-v1.0.1` |
| `v1.0.2` | `c621a22f368cba1dfd06052fc8c37893c1b10792` | arcade | `arcade-v1.0.2` |
| `v1.0.3` | `a07afaa4ada23f365babc45544cc36547446734a` | arcade | `arcade-v1.0.3` |

- **Kept exactly as they are.** Never deleted, moved, re-signed or re-pointed. They stay the canonical names for arcade 1.0.0–1.0.3, documented as **legacy arcade tags** in the root and arcade changelogs.
- No new plain `vX.Y.Z` tags after the switch.
- Optional, purely additive aliases point at the **same commits** above (never at a restructure commit), e.g. `git tag -a arcade-v1.0.3 v1.0.3^{} -m "Alias of v1.0.3 (arcade)"`. They need Ashe's OK, and whoever holds push rights pushes them. An alias must **not** trigger any packaging job (§5).
- Existing `release/1.0.x` branches are left alone.

---

## 5. CI / build commands per platform (proposed; nothing exists yet)

There is no CI today. Each job below lists its working directory, bootstrap, commands and outputs. Wii `make` targets are **new deliverables** of the Wii plan, not existing scripts. Ashe enables Actions.

**Always-on gate:** a lightweight `ci-gate` job runs on every PR. It checks which paths changed and requires only the matching platform jobs, so a path-filtered required check can't sit "pending" forever on an unrelated PR.

| Workflow | Triggers | Working dir → bootstrap → commands → outputs |
|---|---|---|
| `web.yml` | PR/push touching `web/**`, root `vercel.json`/`.vercelignore`, `shared/**`, or `.github/workflows/web.yml`; tags `web-v*` | **repo root** → `actions/setup-node` (pinned) + `npm ci --prefix arcade` (provides `playwright-core`; or a future `web/package.json` with its own pinned dev deps) + `actions/setup-python` + pinned `playwright` pip package + `python -m playwright install chromium` → start `python -m http.server 8000 --directory web` in the background with a readiness loop (`curl -sf http://127.0.0.1:8000/`) and a cleanup trap → run the **modern test manifest** `web/tests/MANIFEST` (an explicit list of `.mjs`/`.cjs`/`.py` checks known to pass on today's layout) with `BASE_URL=http://127.0.0.1:8000`, `NODE_PATH=$PWD/arcade/node_modules`, Playwright's Chromium (or `CHROME_PATH` where a check requires it) → screenshots go to `$RUNNER_TEMP/shots` (artifact). Deployment stays with Vercel's Git integration |
| `arcade.yml` | PR/push touching `arcade/**`, `web/cornhole/**`, `web/yard-games/**` (the old builder copies them), or the workflow; **no tag publishing until PR R lands** | `arcade/` → `npm ci` → `npm test`; separately, `arcade/cabinet-snapshots/1.0.3/` → `node --test tests/*.test.mjs`. **No `build:linux` and no release upload**: that builder packages the retired distance-power gameplay (§1). After PR R, the job builds from the approved release source and runs the full resource-manifest + speed/spin/version + Electron smoke checks before attaching an artifact to an `arcade-v*` tag |
| `wii.yml` | PR/push touching `wii/**`, `shared/**`, `arcade/cabinet-snapshots/1.0.3/**` (pinned oracle/export input), `arcade/package-lock.json` (pinned `three`), or the workflow; tags `wii-v*` | **repo root** → container `devkitpro/devkitppc:<pinned>` extended with pinned WIT, a **host** gcc and Node (`wii/docker/Dockerfile`) → `npm ci --prefix arcade` → `make -C wii host-test` (host compiler) → `make -C wii` → `make -C wii iso` → `make -C wii release` → checks that generated `data/` matches what's committed (if generated assets are checked in) → outputs in `wii/dist/`: HBC zip, `BackyardCornhole-BYCEAO-vX.Y.Z.iso`, `BYCEAO.ini`, `INSTALL-KINHANK.md`, `SHA256SUMS`. On `wii-v*` tags: the version check (§3), then a GitHub Release with those files |
| `drift.yml` | Any PR once `shared/` exists | repo root → Node → drift check against `common.json`, overrides and approvals (§2.2) |

Local equivalents (each line runs from the **repo root**; no shared shell state assumed):

```bash
# web (two shells)
python3 -m http.server 8000 --directory web
NODE_PATH=$PWD/arcade/node_modules BASE_URL=http://127.0.0.1:8000 node web/tests/run-manifest.mjs   # new runner over web/tests/MANIFEST
# arcade
(cd arcade && npm ci && npm test)
(cd arcade/cabinet-snapshots/1.0.3 && node --test tests/*.test.mjs)
# wii (new targets)
docker run --rm -v "$PWD":/src -w /src backyard-wii-build:<tag> sh -c 'npm ci --prefix arcade && make -C wii host-test all iso release'
```

---

## 6. Safe migration plan (PRs only, no force-push, no history rewrite)

Ground rules: every step is a normal branch + PR. No `push --force`, no rebase of shared branches, no tag deletion or moving. Whoever holds push rights pushes the branch and opens the PR; the current dev box has none, so it hands over branches or bundles. Untracked local leftovers stay untracked. **Re-check remote state** (`git fetch`, `git ls-remote --tags origin`) when preparing each PR; cached refs aren't trusted.

**PR 0 — prerequisite: land arcade 1.0.3 on `main` with history preserved.**
`release/1.0.3` → `main` as a **normal merge commit (not squash, not rebase)**, so the original release commit stays an ancestor. Verify after merging: `git merge-base --is-ancestor <tag> origin/main` succeeds for `v1.0.0`, `v1.0.1`, `v1.0.2` and `v1.0.3`. Then base everything on a fresh `origin/main`. Land open web feature PRs first where possible.

**PR T — test baseline (before any move).** On today's layout: inventory every web test as **modern** (passes now) or **legacy** (e.g. Python suites that expect cornhole at `/`), record baseline results, and write `tests/MANIFEST` + a small runner with explicit server root, base URL, cwd, browser selection and artifact paths. Legacy suites are marked archived, or fixed in a separate bounded patch. This separates migration failures from failures that already exist.

**PR 1 — restructure: `web/` + per-platform changelogs (no behavior change).**
1. `git mv` the web files and folders listed in §2.1 into `web/`; `git mv tests web/tests`.
2. Compatibility edits (stated explicitly; more than one file):
   - `arcade/build.mjs`: read sources from `path.join(repo,'web',folder,file)`.
   - `arcade/tests/isolation.test.mjs`: add a `webRoot` variable for **filesystem access** while keeping **web-relative map keys** (`cornhole/index.html`, `yard-games/backyard.js`, `yard-games/power-arrow.js`…), so the existing `before.get(...)` assertions keep working. Keep hashing root `vercel.json` separately, and keep the root `.vercelignore` checks.
   - Test runner/manifest paths and screenshot output (cwd-independent), per PR T.
3. **One deployment topology:** add `"outputDirectory": "web"` to the **root** `vercel.json` (keeping the CSP headers), and extend `.vercelignore` with `wii`, `shared`, `docs`, `web/tests`, `node_modules`, `dist`, `.cache`. **Verify on the PR's Vercel preview** that `/`, `/cornhole/`, `/beer-pong/`, `/ladder-golf/`, `/washers/`, `/lawn-darts/` (clean and trailing-slash forms), `/assets/backyard.png` and the CSP header all work, and that `web/tests` and screenshots are **not** served. If `outputDirectory` doesn't work for this buildless project, **stop and decide**. Switching the project's Root Directory is a different topology: it means moving `vercel.json`, updating the isolation-test paths, and documenting a **settings rollback** (Vercel → Settings → Root Directory back to the repo root), since reverting a Git merge doesn't revert a project setting.
4. `git mv CHANGELOG.md arcade/CHANGELOG.md`; add the new root index `CHANGELOG.md` and `web/CHANGELOG.md`; split the README.
5. Verify before review:
   - `(cd arcade && npm ci && npm test)` passes, including the updated isolation test.
   - `(cd arcade/cabinet-snapshots/1.0.3 && node --test tests/*.test.mjs)` passes (untouched).
   - `arcade/dist/site/` from the old builder is byte-identical before vs after the move. This **only** proves the move didn't change the old builder's input; it says nothing about the released cabinet app (PR R).
   - The web manifest passes with the same results as the PR T baseline.
   - `git log --follow` works on moved files; `git diff -M --stat` shows renames.
6. Merge (normal merge); watch the production deployment; spot-check `playbackyardgames.vercel.app` and the `backyard-cornhole.vercel.app` alias. **Rollback = revert the merge commit** (plus the settings rollback, only if the Root Directory topology was used).

**PR R — arcade release-source reconciliation (required before any new arcade publication; separately reviewed).** Either promote the 1.0.3 snapshot's real runtime sources (`resources-app/`: controller, trackball, spin, legend, party, backyard edits, main/preload version bridge) into maintained `arcade/` source, or add a packager that consumes the validated snapshot directly and never regenerates it with the old builder. Keep the existing launcher, preload, trusted protocol, Python helpers, private libcups and its license, cabinet support hook and media packaging. Before any `arcade-v*` tag: a full resource-manifest comparison against the installed snapshot, speed/spin/version tests, and an Electron smoke test of the packaged artifact.

**PR 2 (optional) — tidy history files.** `git mv` the tracked agent artifacts into `docs/history/` and `shots/` into `docs/screenshots/`. No code changes.

**PR 3 — Wii skeleton + docs + CI.** Add `wii/` (IMPL_PLAN P0), `wii/CHANGELOG.md`, `docs/wii-port/*.md`, and `.github/workflows/{ci-gate,web,arcade,wii}.yml` (arcade without publishing until PR R).

**PR 4 — shared data.** `shared/cornhole/common.json`, overrides, approvals, `golden/arcade-1.0.3/`, the drift check, and `wii/tools/gen-constants.mjs`. No web or arcade behavior change.

**First tags after the switch:** the next arcade release is `arcade-v1.0.4` (only after PR R); the first Wii release is `wii-v0.1.0`; web gets `web-v1.0.0` whenever Ashe wants to start versioning the site.

---

## 7. Risks of the restructure

| Risk | Mitigation |
|---|---|
| Production site breaks if Vercel serves the wrong folder | One topology, verified on the preview; route and CSP checklist; Git revert plus a documented settings rollback if the Root Directory topology is ever used |
| **Stale arcade gameplay gets published** (old builder = distance power) | No arcade packaging/tag job until PR R; manifest + speed/spin/version + Electron smoke gates |
| Test failures blamed on the move | PR T baseline first; modern manifest; legacy suites archived or fixed separately |
| Isolation test silently weakened | Web-root variable with unchanged map keys; root `vercel.json`/`.vercelignore` still checked |
| Shared-data drift check rejects intentional differences | Common fields + per-platform overrides + structured approvals |
| Open feature branches conflict with the move | Land them first; git's rename detection handles most of the rest |
| Confusion between legacy `v1.0.x` and new `arcade-v*` tags | Exact mapping table; no new plain `v` tags; aliases optional and never trigger packaging |
| Legacy tags not reachable from `main` | PR 0 normal merge + `merge-base --is-ancestor` checks |

## 8. Open questions for Ashe

1. OK with `web/` (not `mobile/`) as the folder name?
2. Vercel topology: `outputDirectory` in the root `vercel.json` (proposed), or the project Root Directory setting?
3. Add the `arcade-v1.0.0`–`arcade-v1.0.3` alias tags, or leave the legacy tags alone?
4. Should the agent task files and logs move to `docs/history/` (PR 2), or stay at the root?
5. Should the web site start semver tags (`web-v1.0.0`) now, or keep continuous deployment without tags?
6. PR R: promote the snapshot into maintained source, or package the snapshot as-is?
7. Legacy web Python suites: archive them, or fund a bounded fix-up patch?

---

## 9. Decisions from review (Codex, 2026-11-15)

Source: [REVIEW_CODEX.md](REVIEW_CODEX.md); the overall accept/reject list is in [SPEC.md §10](SPEC.md).

**Accepted and applied here**
- **R1 (P0):** the arcade tag job using `build:linux` is removed; it would package retired distance-power gameplay. New **PR R** reconciles the release source first, with manifest/speed/spin/version/Electron gates. The byte-identical `dist/site` check is relabeled as proving only that the old builder's input is unchanged.
- **R2:** shared data is now common fields + per-platform overrides + structured approvals; the web-vs-arcade differences are tabulated; the Wii baseline is pinned to arcade 1.0.3; the shared PR stays after P0 because the Wii tools read the snapshot directly.
- **R3:** new PR T (test baseline, modern/legacy inventory, manifest runner); CI commands rewritten with explicit working directories, server lifecycle, pinned drivers/browsers and `NODE_PATH`.
- **R4:** the isolation test keeps web-relative map keys via a `webRoot` variable; root config checks kept; the "two path fixes only" claim removed.
- **R5:** per-job bootstrap and outputs; host compiler and Node in the Wii image; triggers include the pinned snapshot, the lockfile and the workflows; tag triggers explicit; `make release` defined; an always-on `ci-gate` for path-filtered required checks.
- **R6:** `.vercelignore` facts corrected (it only has `arcade` today); one topology with explicit deployment exclusions (tests, screenshots, tooling); the Root Directory alternative needs its own settings rollback.
- **R7:** exact legacy-tag → commit mapping; PR 0 must be a normal merge with ancestry checks; remote state re-checked per PR; aliases never trigger packaging; the external policy reference replaced with self-contained rules; push ability no longer stated as an architectural fact.

**Rejected / not adopted (mentioned for the record)**
- **R7, partly:** "the claim that all four tags are on GitHub was never verified." It was checked with `git ls-remote --tags origin` on 2026-11-15. We kept the re-check-at-PR-time step anyway.
- **R6, partly:** excluding README/CHANGELOG files from deployment isn't adopted as a requirement. Serving them is harmless and matches the site's behavior today. Tests, screenshots and tool output **are** excluded.
