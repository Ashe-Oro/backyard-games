# Batocera artwork

Original art generated from our own game scene, Press Start 2P typography and code-drawn board/bags. Regenerate with `tools/generate-media.cjs` after the scene QA screenshots are refreshed. No scraped third-party game art.

- Cover / box art / thumbnail: 600 × 800.
- Transparent logo / marquee: 1200 × 300.
- Gameplay / fan art: 1920 × 1080.

The cabinet uses Hypermax-Plus-PixN, Gamelist CoinOPS. That view reads `image` at a central 0.4 × 0.5 display area and `fanart` full-screen. Its marquee display is hidden in that view, but the carousel and other views support the marquee field. The prepared XML supplies all relevant fields.

Installation is pending the next maintenance window. Back up both Ports and Windows gamelist files before editing. Stop EmulationStation before updating disk metadata so its cached list cannot overwrite the changes. Merge only the `./BackyardCornhole.sh` entry and retain play history and unrelated entries. Store media in `/userdata/roms/ports/images/`, then restart the frontend and check both Ports and Favorites.

The obsolete Windows package was verified at `/userdata/roms/windows/BackyardCornhole.pc` (358 MB). Preserve it outside the scanned ROM tree before removing its Windows entry. Do not touch Wine bottles or other Windows games. Verify native launch and exit afterward. This directory contains prepared assets; their existence does not mean cabinet installation has happened.
