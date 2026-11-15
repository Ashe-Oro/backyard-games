# Cabinet snapshots

These are not the git 0.2.14 arcade sources. They came from the live Batocera install.

- `0.2.17/` is a verbatim copy of the cabinet's `BackyardCornhole-linux/resources/app` plus `READ-ME.txt` and `run-cornhole.sh`, pulled on 2026-11-12. Source for 0.2.15–0.2.17 was never in this repo.
- `0.2.18/` is that 0.2.17 tree with the power, P1 button, and backswing-spin port applied. Installed on the cabinet 2026-11-12 9:54 PM ET over the existing Electron runtime. Rollback: `/userdata/system/backyard-games/.backup-v0.2.17-20261112-215429`.
- `0.2.20/` corrects the P1 physical play map and halves power sensitivity with a 3300 counts/sec default, including saved factory-speed migrations. It also halves the button nudge step from 0.1 to 0.05 and removes spinner lateral placement. Includes runnable snapshot tests and unchanged-file SHA-256 verification in `CODEX_SUMMARY.md`. The first 0.2.20 build (map + 3300 only) was already installed; this commit completes the remaining 0.2.20 items, Installed on the cabinet 2026-11-14 1:06 PM ET over the first 0.2.20 build. Rollback: `/userdata/system/backyard-games/.backup-v0.2.20-20261114-130604`. First-install rollback of 0.2.19: `/userdata/system/backyard-games/.backup-v0.2.19-20261114-130121`.
- `0.2.19/` is the 0.2.18 tree with speed-based throw power restored: peak forward counts/sec over an 80 ms window, default full-power speed 1650 counts/sec (1.57× the 0.2.17 default of 1050). P1 buttons, backswing spin, and forward aiming are unchanged. Installed on the cabinet 2026-11-13 8:04 AM ET over the existing Electron runtime. Rollback: `/userdata/system/backyard-games/.backup-v0.2.18-20261113-080423`.

- `0.2.21/` adds five randomly selected background beer pong matches, with no immediate variation repeat and a coin-flipped comeback winner. Cornhole gameplay and controls are unchanged. Installed on the cabinet 2026-11-14 1:12 PM ET over 0.2.20. Rollback: `/userdata/system/backyard-games/.backup-v0.2.20-20261114-131236`.

Do not push licensed or packaged Electron binaries; these snapshots contain only the app resources (including the already-bundled vendor three.js and font).

- `0.2.22/` removes the long bottom-center controls bar, adds a compact bottom-right P1 button legend (KEYS tucks or pops it and remembers the choice; top/bottom 4 no longer reset), hides diagnostics, shortens last-shot power from five to three seconds, and raises default full-power speed from 3300 to 4000 with factory-profile migration. Installed on the cabinet 2026-11-14 1:30 PM ET over 0.2.21. Rollback: `/userdata/system/backyard-games/.backup-v0.2.21-20261114-133056`.

- `0.2.23/` raises default full-power speed from 4000 to 5600 (+40%) with factory-profile migration including 4000; adds on-board spin curl (A clockwise/right, C counterclockwise/left) and a left spinner legend; and replaces the sideline 10–2 beer pong script with a steady 10–7 blue win plus rim-ricochet misses. Installed on the cabinet 2026-11-14 2:03 PM ET over 0.2.22. Rollback: `/userdata/system/backyard-games/.backup-v0.2.22-20261114-140357`.

- `1.0.1/` raises default full-power speed from 5600 to 7560 (+35%) with factory-profile migration including 5600; removes the blue cooler prop. Install timestamp: pending. Rollback path: pending.

- `1.0.2/` lowers default full-power speed from 7560 to 6048 (−20%) with factory-profile migration including 7560 and 5600. All other gameplay stays as in 1.0.1, including the removed blue cooler. Install timestamp: pending. Rollback path: pending.

- `1.0.3/` adds a small upper-right running-version label (`v` + installed package version), visible during gameplay. Install timestamp: pending. Rollback path: pending.

- `1.0.4/` adds a vertical power meter with no LAST SHOT label; it persists about two seconds after the throw. Removes the CLEAR BOARD popup; top-right clears bags in Practice, while only bottom-right toggles KEYS. Install timestamp: pending. Rollback path: pending.

- `1.0.5/` fixes straight throws from a left or right stance: the bag flies parallel to the board and stays to that side of the hole, with aim and spin still available. Copied from the verified live 1.0.4 snapshot. Install timestamp: pending. Rollback path: pending.

- `1.0.6/` adds a clear direction arrow from the bag during the forward stroke and after release. Hard left/right leans show clearly; the arrow holds about two seconds, then fades. Copied from 1.0.5 with its straight-path behavior preserved. Install timestamp: pending. Rollback path: pending.

- `1.0.7/` replaces the big vertical power bar and long direction shaft with one compact arrow from the bag: length and color show power on a faint full-power guide. Holds about two seconds, then fades. Copied from 1.0.6 with gameplay preserved. Install timestamp: pending. Rollback path: pending.

- `1.0.8/` fixes next-toss readiness and rejects incomplete early spins; a soft bag glow shows readiness. Install timestamp: pending. Rollback path: pending.
