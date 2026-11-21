# Deployment targets

Backyard Games has four explicit deployment targets. Kinhank is the current test target.

| Target | Source | Changelog | Tags |
|---|---|---|---|
| [web](#web--mobile-web-app) | [`web/`](web/) | [web/CHANGELOG.md](web/CHANGELOG.md) | as they are today (`web-vX.Y.Z` when a web release is cut; production otherwise tracks `main`) |
| [arcade](#arcade--linux-cabinet) | [`arcade/`](arcade/) | [arcade/CHANGELOG.md](arcade/CHANGELOG.md) | `arcade-vX.Y.Z` (legacy `v1.0.0`–`v1.0.8` are arcade releases and are never moved) |
| [wii](#wii--original-hardware) | [`wii/`](wii/) | [wii/CHANGELOG.md](wii/CHANGELOG.md) | `wii-vX.Y.Z` |
| [kinhank](#kinhank--dolphin-on-the-kinhank-current-test-target) | [`kinhank/`](kinhank/) plus the Wii ISO | [kinhank/CHANGELOG.md](kinhank/CHANGELOG.md) | `kinhank-vX.Y.Z` |

## web — mobile web app

- **Source:** [`web/`](web/)
- **Build:** none. The site is a buildless static tree.
- **Artifacts:** the `web/` folder, which Vercel serves as `"outputDirectory": "web"`
- **Deploy path:** push or merge to `main` → Vercel project `backyard-games` → https://playbackyardgames.vercel.app/
- **Who deploys it:** automatic, via the GitHub ↔ Vercel integration
- **Changelog:** [web/CHANGELOG.md](web/CHANGELOG.md)
- **Tags:** as they are today. Use `web-vX.Y.Z` when cutting a named web release; otherwise production tracks `main`.

## arcade — Linux cabinet

- **Source:** [`arcade/`](arcade/)
- **Build:** from `arcade/`, `npm test`, then `npm run build:linux` (see [arcade/README.md](arcade/README.md) before publishing a build)
- **Artifacts:** the native Linux package (`BackyardCornhole-linux`)
- **Install path:** runtime `/userdata/system/backyard-games/BackyardCornhole-linux` and Ports launcher `/userdata/roms/ports/BackyardCornhole.sh` on the Batocera cabinet
- **Who deploys it:** installed on the cabinet. This repo does not push to the cabinet automatically.
- **Changelog:** [arcade/CHANGELOG.md](arcade/CHANGELOG.md)
- **Tags:** `arcade-vX.Y.Z`. Legacy tags `v1.0.0`–`v1.0.8` are arcade releases and are never moved.

## wii — original hardware

- **Source:** [`wii/`](wii/)
- **Build:** `make -C wii release`
- **Artifacts:** the Homebrew Channel zip (`BackyardCornhole-HBC-vX.Y.Z.zip`) and `BackyardCornhole-BYCEAO.iso`. NTSC 480p with a component cable and 480p enabled in Wii Settings, otherwise 480i. Wii bundles do **not** include Dolphin `GameSettings`.
- **Install path:** unzip the HBC zip to the root of a FAT32 SD card so the files are `sd:/apps/backyardcornhole/{boot.dol,meta.xml,icon.png}`. The ISO is optional and untested on USB loaders. This repo cannot push to a physical Wii; copy the files onto the SD card by hand. See [wii/INSTALL-WII.md](wii/INSTALL-WII.md).
- **Who deploys it:** Ashe, manually
- **Changelog:** [wii/CHANGELOG.md](wii/CHANGELOG.md)
- **Tags:** `wii-vX.Y.Z`

## kinhank — Dolphin on the Kinhank (current test target)

- **Source:** [`kinhank/`](kinhank/), plus the ISO produced by the Wii build (same `BYCEAO` disc and DOL)
- **Build:** `make -C kinhank` (packages that ISO with this target's `GameSettings`)
- **Artifacts:** `BackyardCornhole-Kinhank-vX.Y.Z.zip` containing `BackyardCornhole-BYCEAO.iso`, `GameSettings/BYCEAO.ini` (`InternalResolution = 3`, about 1080p), [kinhank/INSTALL.md](kinhank/INSTALL.md) and `SHA256SUMS`. The ini in `kinhank/GameSettings/BYCEAO.ini` is the source of truth.
- **Install path:** Retro copies the ISO to `/storage/3050BAA350BA6EEA0/Roms/wii/` and the ini to `/storage/emulated/0/Android/data/org.dolphinemu.dolphinemu/files/GameSettings/BYCEAO.ini` over ADB, then `chmod 666`s the ini and checks `ls -l` is world-readable. Use [kinhank/install-adb.sh](kinhank/install-adb.sh). Do not change Dolphin's global `GFX.ini` (keep it at 1× for other games). See [kinhank/INSTALL.md](kinhank/INSTALL.md).
- **Who deploys it:** Retro. Developers never touch the Kinhank.
- **Changelog:** [kinhank/CHANGELOG.md](kinhank/CHANGELOG.md)
- **Tags:** `kinhank-vX.Y.Z`
