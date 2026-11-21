# Moved: Kinhank install guide

The Kinhank (Dolphin) target now lives in [`kinhank/`](../../kinhank/).

- Install: [../../kinhank/INSTALL.md](../../kinhank/INSTALL.md)
- GameSettings (source of truth): [../../kinhank/GameSettings/BYCEAO.ini](../../kinhank/GameSettings/BYCEAO.ini)
- Changelog: [../../kinhank/CHANGELOG.md](../../kinhank/CHANGELOG.md)
- Index: [../../TARGETS.md](../../TARGETS.md)

`wii/dolphin/GameSettings/BYCEAO.ini` is a symlink to that ini so existing
Wii build scripts keep resolving the same path. Wii release bundles no longer
include it.
