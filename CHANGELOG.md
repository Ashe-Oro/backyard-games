# Changelog index

Each platform keeps its own changelog ([Keep a Changelog](https://keepachangelog.com/) + SemVer):

| Platform | Changelog | Tags |
|---|---|---|
| Web (playbackyardgames.vercel.app) | [web/CHANGELOG.md](web/CHANGELOG.md) | `web-vX.Y.Z` (none yet) |
| Arcade (Electron / Batocera cabinet) | [arcade/CHANGELOG.md](arcade/CHANGELOG.md) | `arcade-vX.Y.Z`; `v1.0.0`–`v1.0.5` |
| Wii (HBC + Dolphin) | [wii/CHANGELOG.md](wii/CHANGELOG.md) | `wii-vX.Y.Z` |

Shared cornhole data (`shared/cornhole/`) has no changelog of its own. A shared change that alters
gameplay values is logged in **each** affected platform's changelog and gets an entry in
`shared/cornhole/approvals.json`.

**Tags:** `v1.0.0` through `v1.0.5` are arcade releases. Entries 1.0.0–1.0.3 moved from this file to [arcade/CHANGELOG.md](arcade/CHANGELOG.md) unchanged; 1.0.4 and 1.0.5 are recorded there too
(`git log --follow arcade/CHANGELOG.md`). Those tags are never moved or deleted.
