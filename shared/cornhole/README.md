# shared/cornhole — cornhole rules data

Platform-agnostic cornhole **data**: board and bag geometry, physics constants, scoring rules and wind tiers.
No game imports this yet. The web site stays buildless, and the arcade and web games keep their own inline
constants. This folder records what they share, what differs on purpose, and checks that the record stays true.

| File | What | Owner |
|---|---|---|
| `common.json` | Values verified **identical** in web and arcade 1.0.3. Each entry has the exact source snippet it was verified against | Every platform |
| `overrides/web.json` | Web values that differ on purpose (slick surface, release height, camera, spin, pull-distance power) | Web |
| `overrides/arcade-1.0.3.json` | Arcade 1.0.3 values that differ on purpose (read from the pinned cabinet snapshot, not the older top-level `arcade/` sources). **This is the Wii baseline** | Arcade |
| `overrides/wii.json` | Wii-only differences from that baseline (none yet) | Wii |
| `approvals.json` | Structured drift approvals `{field, platform, old, new, baseline, pr}` | Whoever changes a value |
| `tests/drift.test.mjs` | Drift check | — |

Check it (from the repo root, no dependencies):

```bash
node --test shared/cornhole/tests/*.test.mjs
```

The check fails when a snippet listed here no longer appears in its source file, or when a `value` doesn't
match its `expr`. To change a value on purpose: edit the game source, update the JSON entry, add an
approval to `approvals.json`, and log it in each affected platform's CHANGELOG.

Rules:
- Restructuring never changes web physics to match arcade, or the reverse.
- Only add a field to `common.json` after confirming it is identical in every listed platform.
- Golden test vectors (`golden/arcade-1.0.3/`) and the Wii constants generator come later (REPO_LAYOUT §6 PR 4, IMPL_PLAN Track C).
