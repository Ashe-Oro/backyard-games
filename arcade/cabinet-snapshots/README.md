# Cabinet snapshots

These are not the git 0.2.14 arcade sources. They came from the live Batocera install.

- `0.2.17/` is a verbatim copy of the cabinet's `BackyardCornhole-linux/resources/app` plus `READ-ME.txt` and `run-cornhole.sh`, pulled on 2026-11-12. Source for 0.2.15–0.2.17 was never in this repo.
- `0.2.18/` is that 0.2.17 tree with the power, P1 button, and backswing-spin port applied. Installed on the cabinet 2026-11-12 9:54 PM ET over the existing Electron runtime. Rollback: `/userdata/system/backyard-games/.backup-v0.2.17-20261112-215429`.

Do not push licensed or packaged Electron binaries; these snapshots contain only the app resources (including the already-bundled vendor three.js and font).
