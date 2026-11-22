# Install Backyard Cornhole on the Kinhank (Dolphin) — for Retro

This is the **kinhank** target, the current test target. Developers never
touch the Kinhank. Retro installs and tests over ADB.

The original Wii hardware target is separate: see [../wii/INSTALL-WII.md](../wii/INSTALL-WII.md)
and [../TARGETS.md](../TARGETS.md). Do not put this bundle's `GameSettings` on
a real Wii; that hardware ignores it and tops out at 480p.

This is the playable 0.1.4 Bowling-throw build, rendered at 3× (about 1080p) in
Dolphin. Desktop smoke does not validate real Wiimote feel or Android
performance. Owner: Ashe.

## Bundle

From `make -C kinhank` or the `BackyardCornhole-Kinhank-v0.1.4.zip` release:

```text
BackyardCornhole-BYCEAO.iso
GameSettings/BYCEAO.ini
INSTALL.md
install-adb.sh
SHA256SUMS
```

The ini in this folder, [GameSettings/BYCEAO.ini](GameSettings/BYCEAO.ini), is
the source of truth. It must contain `InternalResolution = 3`.

## Install

Preferred: from the unzipped bundle (or this folder after `make -C kinhank`),
run [install-adb.sh](install-adb.sh). It pushes the ISO and ini, runs
`chmod 666` on the ini, and checks `ls -l` is world-readable.

To do the same steps by hand:

1. Check the files: `sha256sum -c SHA256SUMS`.
2. Connect ADB to `192.168.4.59:5555` from a machine on the cabinet's network
   and copy the ISO:

   ```sh
   adb connect 192.168.4.59:5555
   adb -s 192.168.4.59:5555 push BackyardCornhole-BYCEAO.iso /storage/3050BAA350BA6EEA0/Roms/wii/
   ```

3. Copy the per-game ini to the path verified on this Kinhank, then
   **`chmod 666` it**. A file copied over ADB is owned by `shell:shell` and
   ends up `-rw-rw----`. Dolphin (the app uid) cannot read that, so it
   silently falls back to global settings and 3× never applies. The ini
   format in this repo is already correct; the missing step is the mode.

   ```sh
   ini=/storage/emulated/0/Android/data/org.dolphinemu.dolphinemu/files/GameSettings/BYCEAO.ini
   adb -s 192.168.4.59:5555 shell mkdir -p "$(dirname "$ini")"
   adb -s 192.168.4.59:5555 push GameSettings/BYCEAO.ini "$ini"
   adb -s 192.168.4.59:5555 shell chmod 666 "$ini"
   adb -s 192.168.4.59:5555 shell ls -l "$ini"
   ```

   The `ls -l` read-back must show a world-readable mode, for example
   `-rw-rw-rw-`. If you still see `-rw-rw----`, Dolphin is ignoring the
   file. Do not change Dolphin's global `GFX.ini` to get 3×; Ashe keeps
   the global setting at 1× for other games. 3× comes only from this
   per-game ini. With a world-readable ini, 3× has been verified at
   59.95 FPS.

   Expected file content (already in [GameSettings/BYCEAO.ini](GameSettings/BYCEAO.ini)):

   ```ini
   [Controls]
   WiimoteSource0 = 2
   WiimoteSource1 = 2

   [Video_Settings]
   InternalResolution = 3
   ```

4. Set the MAYFLASH DolphinBar to **mode 4**. Pair two Wii Remotes to the bar; in Dolphin,
   confirm Wii Remote 1 and 2 are **Real Wii Remote**. The ini is keyed to
   disc ID `BYCEAO`, independent of the ISO filename. The game uses buttons and
   the accelerometer only (no sensor bar / pointer).
5. Set graphics backend **Vulkan** and confirm the game boots in NTSC /
   approximately 60 Hz. Leave the global internal resolution at 1×. Confirm
   this game is running at 3× from `BYCEAO.ini`, not from a global change.
   The game's frame rate may report approximately 30 FPS for an interlaced
   framebuffer; VI should remain approximately 60 Hz. If Vulkan is
   unavailable, record the device / driver issue before changing the test
   configuration.
6. Virtual SD: enable it and make sure it's writable. Saves go to
   `sd:/apps/backyardcornhole/data/save.dat`.
7. Launch the ISO from the Wii ROM list. Properties must show game ID **BYCEAO**,
   region USA. Check both players' connection, menu input and tosses, including
   reconnects.

Fallback: if the per-game ini still isn't picked up after a world-readable
`ls -l`, set the **global** Wii Remote 1 (and 2) source to Real Wii Remote in
Controller settings (affects every game). Do not raise the global internal
resolution. The 3× setting still comes from this game's ini, so keep
installing it with `chmod 666`.

Controls: hold B, swing back, swing forward, then release B. Swing speed is
power, swinging left/right aims, and wrist twist curls. D-pad up/down picks
LOB / NORMAL / SLIDER (the only control for height); D-pad left/right slides
along the foul line. -/+ do nothing in play. The pointer is never used. The
version (v0.1.4) shows on the STRAP ON! screen and in the bottom-right corner. Choose
LEFT/RIGHT hand for each player in SETTINGS. Hold 1+2 for two seconds to
toggle live swing diagnostics.
