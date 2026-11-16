# Install on the Kinhank X5 Pro (Dolphin Android 5.0-19870) — for Retro

Developers never touch the Kinhank. Retro installs and tests.

Files (from `wii/dist/` or the release bundle): `BackyardCornhole-BYCEAO.iso`, `BYCEAO.ini`, `SHA256SUMS`.

1. Check the files: `sha256sum -c SHA256SUMS`.
2. Copy the ISO:
   `adb push BackyardCornhole-BYCEAO.iso /storage/3050BAA350BA6EEA0/Roms/wii/`
3. Per-game Real Wii Remotes: put `BYCEAO.ini` in the `GameSettings/` folder of **this** Dolphin build's
   user directory (path to be recorded here at the P0 check; do not assume the desktop path), e.g.
   `adb push BYCEAO.ini <dolphin-user-dir>/GameSettings/BYCEAO.ini`.
   Contents: `[Controls] WiimoteSource0 = 2`, `WiimoteSource1 = 2` (Remote 1 and 2 = Real).
   Alternative: set Wii Remote 1/2 to "Real Wii Remote" in the game's per-game settings in Dolphin.
4. DolphinBar in **Mode 4**; sync the Wiimotes to it. The game uses buttons and the accelerometer only
   (no sensor bar / pointer).
5. Dolphin settings already proven on this box: Vulkan, 1× native resolution, NTSC (no PAL60).
6. Virtual SD: enable it and make sure it's writable. Saves go to `sd:/apps/backyardcornhole/data/save.dat`.
7. Launch from the game list. Properties must show game ID **BYCEAO**, region USA.

Fallback: if the per-game ini isn't picked up, set the **global** Wii Remote 1 (and 2) source to Real Wii
Remote in Controller settings (affects every game), or boot the bare `boot.dol` with that global setting.

Record at the P0 check: the real `GameSettings/` path, whether the ini applied, virtual-SD writability,
how to copy files off the SD, and Dolphin's speed % during a full match.
