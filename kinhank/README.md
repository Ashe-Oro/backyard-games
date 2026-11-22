# Backyard Cornhole — Kinhank

Dolphin on the Kinhank X5 Pro. The current build is v0.1.7. See
[TARGETS.md](../TARGETS.md) and [INSTALL.md](INSTALL.md).

The game disc is the Wii ISO (`BYCEAO`). This folder is the source of truth
for Dolphin `GameSettings/BYCEAO.ini` (`InternalResolution = 3`, about 1080p)
and for the Kinhank release bundle.

```sh
make -C wii iso
make -C kinhank ISO=../wii/dist/BackyardCornhole-BYCEAO.iso
```

Retro installs the zip over ADB with [install-adb.sh](install-adb.sh), which
`chmod 666`s the ini and checks `ls -l` is world-readable. A 660 shell:shell
copy is ignored by Dolphin. Leave the global `GFX.ini` at 1×; 3× comes only
from this per-game ini. Developers never touch the Kinhank.
Releases are tagged `kinhank-vX.Y.Z`.
