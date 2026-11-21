# Backyard Cornhole — Kinhank

Dolphin on the Kinhank X5 Pro. This is the current test target. See
[TARGETS.md](../TARGETS.md) and [INSTALL.md](INSTALL.md).

The game disc is the Wii ISO (`BYCEAO`). This folder is the source of truth
for Dolphin `GameSettings/BYCEAO.ini` (`InternalResolution = 3`, about 1080p)
and for the Kinhank release bundle.

```sh
make -C wii iso
make -C kinhank ISO=../wii/dist/BackyardCornhole-BYCEAO.iso
```

Retro installs the zip over ADB. Developers never touch the Kinhank.
Releases are tagged `kinhank-vX.Y.Z`.
