# Changelog index

Each platform keeps its own changelog ([Keep a Changelog](https://keepachangelog.com/) + SemVer):

See [TARGETS.md](TARGETS.md) for source folders, build commands, artifacts, install paths and who deploys each target. Kinhank is the current test target.

| Platform | Changelog | Tags |
|---|---|---|
| Web (playbackyardgames.vercel.app) | [web/CHANGELOG.md](web/CHANGELOG.md) | as they are today (`web-vX.Y.Z` when cut; none yet) |
| Arcade (Electron / Batocera cabinet) | [arcade/CHANGELOG.md](arcade/CHANGELOG.md) | `arcade-vX.Y.Z`; legacy `v1.0.0`–`v1.0.8` |
| Wii (original hardware, HBC zip + ISO) | [wii/CHANGELOG.md](wii/CHANGELOG.md) | `wii-vX.Y.Z` |
| Kinhank (Dolphin on the Kinhank, current test target) | [kinhank/CHANGELOG.md](kinhank/CHANGELOG.md) | `kinhank-vX.Y.Z` |

Shared cornhole data (`shared/cornhole/`) has no changelog of its own. A shared change that alters
gameplay values is logged in **each** affected platform's changelog and gets an entry in
`shared/cornhole/approvals.json`.

**Tags:** `v1.0.0` through `v1.0.5` are arcade releases. Entries 1.0.0–1.0.3 moved from this file to [arcade/CHANGELOG.md](arcade/CHANGELOG.md) unchanged; 1.0.4 and 1.0.5 are recorded there too
(`git log --follow arcade/CHANGELOG.md`). Those tags are never moved or deleted.

## Unreleased

### Changed
- Removed committed agent run logs, Codex/Claude task and review markdown, Codex output files, a leftover git bundle, and agent QA screenshots (`shots/task*`, `shots/review2/`) from the tree, and gitignored that class of files. Going forward, agents should write this material to `/agent-logs/` (ignored) or outside the repo. Release notes (`WII-0.1.x-LOG.md`), product docs, tests, and non-task screenshots stay. History is not rewritten, so the files remain in earlier commits.

## Wii and Kinhank 0.1.12 - 2026-11-26

### Fixed

- Fixed stretched bag rendering in the main view and guarded the board inset against close flybys. See the [Wii changelog](wii/CHANGELOG.md#0112---2026-11-26) and [Kinhank changelog](kinhank/CHANGELOG.md#0112---2026-11-26).
