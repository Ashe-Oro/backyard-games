# Changelog index

Each platform keeps its own changelog ([Keep a Changelog](https://keepachangelog.com/) + SemVer):

| Platform | Changelog | Tags |
|---|---|---|
| Web (playbackyardgames.vercel.app) | [web/CHANGELOG.md](web/CHANGELOG.md) | `web-vX.Y.Z` (none yet) |
| Arcade (Electron / Batocera cabinet) | [arcade/CHANGELOG.md](arcade/CHANGELOG.md) | `arcade-vX.Y.Z`; legacy `v1.0.0`–`v1.0.4` |
| Wii (HBC + Dolphin) | [wii/CHANGELOG.md](wii/CHANGELOG.md) | `wii-vX.Y.Z` (none yet) |

Shared cornhole data (`shared/cornhole/`) has no changelog of its own. A shared change that alters
gameplay values is logged in **each** affected platform's changelog and gets an entry in
`shared/cornhole/approvals.json`.

**Legacy tags:** `v1.0.0`, `v1.0.1`, `v1.0.2`, `v1.0.3` and `v1.0.4` are arcade releases 1.0.0–1.0.4. Entries 1.0.0–1.0.3 moved from this file to [arcade/CHANGELOG.md](arcade/CHANGELOG.md) unchanged; the 1.0.4 entry is recorded there too
(`git log --follow arcade/CHANGELOG.md`). Those tags are never moved or deleted, and no new plain `vX.Y.Z` tags are made.
