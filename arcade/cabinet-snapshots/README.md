# Cabinet snapshots

These are not the git 0.2.14 arcade sources. They came from the live Batocera install.

- `0.2.17/` is a verbatim copy of the cabinet's `BackyardCornhole-linux/resources/app` plus `READ-ME.txt` and `run-cornhole.sh`, pulled on 2026-11-12. Source for 0.2.15–0.2.17 was never in this repo.
- `0.2.18/` is that 0.2.17 tree with the power, P1 button, and backswing-spin port applied. Installed on the cabinet 2026-11-12 9:54 PM ET over the existing Electron runtime. Rollback: `/userdata/system/backyard-games/.backup-v0.2.17-20261112-215429`.
- `0.2.20/` corrects the P1 physical play map and halves power sensitivity with a 3300 counts/sec default, including saved factory-speed migrations. It also halves the button nudge step from 0.1 to 0.05 and removes spinner lateral placement. Includes runnable snapshot tests and unchanged-file SHA-256 verification in `CODEX_SUMMARY.md`. The first 0.2.20 build (map + 3300 only) was already installed; this commit completes the remaining 0.2.20 items, Installed on the cabinet 2026-11-14 1:06 PM ET over the first 0.2.20 build. Rollback: `/userdata/system/backyard-games/.backup-v0.2.20-20261114-130604`. First-install rollback of 0.2.19: `/userdata/system/backyard-games/.backup-v0.2.19-20261114-130121`.
- `0.2.19/` is the 0.2.18 tree with speed-based throw power restored: peak forward counts/sec over an 80 ms window, default full-power speed 1650 counts/sec (1.57× the 0.2.17 default of 1050). P1 buttons, backswing spin, and forward aiming are unchanged. Installed on the cabinet 2026-11-13 8:04 AM ET over the existing Electron runtime. Rollback: `/userdata/system/backyard-games/.backup-v0.2.18-20261113-080423`.

Do not push licensed or packaged Electron binaries; these snapshots contain only the app resources (including the already-bundled vendor three.js and font).
