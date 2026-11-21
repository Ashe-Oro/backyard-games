# Install Backyard Cornhole on the Kinhank (Dolphin) — for Retro

This is the **kinhank** target, the current test target. Developers never
touch the Kinhank. Retro installs and tests over ADB.

The original Wii hardware target is separate: see [../wii/INSTALL-WII.md](../wii/INSTALL-WII.md)
and [../TARGETS.md](../TARGETS.md). Do not put this bundle's `GameSettings` on
a real Wii; that hardware ignores it and tops out at 480p.

This is the playable 0.1.2 throw-fix build, rendered at 3× (about 1080p) in
Dolphin. Desktop smoke does not validate real Wiimote feel or Android
performance. Owner: Ashe.

## Bundle

From `make -C kinhank` or the `BackyardCornhole-Kinhank-v0.1.2.zip` release:

```text
BackyardCornhole-BYCEAO.iso
GameSettings/BYCEAO.ini
INSTALL.md
SHA256SUMS
```

The ini in this folder, [GameSettings/BYCEAO.ini](GameSettings/BYCEAO.ini), is
the source of truth. It must contain `InternalResolution = 3`.

## Install

1. Check the files: `sha256sum -c SHA256SUMS`.
2. Connect ADB to `192.168.4.59:5555` from a machine on the cabinet's network:

   ```sh
   adb connect 192.168.4.59:5555
   adb -s 192.168.4.59:5555 push BackyardCornhole-BYCEAO.iso /storage/3050BAA350BA6EEA0/Roms/wii/
   ```

3. In Dolphin Android, use **Open User Folder** (or the app's user-data export /
   import facility) to locate the active `GameSettings` directory. Copy
   `GameSettings/BYCEAO.ini` there. The expected file content is:

   ```ini
   [Controls]
   WiimoteSource0 = 2
   WiimoteSource1 = 2

   [Video_Settings]
   InternalResolution = 3
   ```

   The directory varies by Dolphin build and Android storage permissions.
   Common locations are `/sdcard/dolphin-emu/GameSettings/` on older builds or
   `/sdcard/Android/data/org.dolphinemu.dolphinemu/files/GameSettings/` on newer
   official builds ([Dolphin's user-directory guide](https://dolphin-emu.org/docs/guides/controlling-global-user-directory/)). Confirm the active location before copying; forks can use
   a different app ID. If the active folder is accessible via ADB:

   ```sh
   # Substitute the confirmed active user directory; do not copy to both blindly.
   adb -s 192.168.4.59:5555 push GameSettings/BYCEAO.ini "<active-user-dir>/GameSettings/BYCEAO.ini"
   ```

4. Set the MAYFLASH DolphinBar to **mode 4**. Pair two Wii Remotes to the bar; in Dolphin,
   confirm Wii Remote 1 and 2 are **Real Wii Remote**. The ini is keyed to
   disc ID `BYCEAO`, independent of the ISO filename. The game uses buttons and
   the accelerometer only (no sensor bar / pointer).
5. Set graphics backend **Vulkan**, confirm internal resolution is **3×**
   (about 1080p) from `BYCEAO.ini`, and confirm the game boots in NTSC /
   approximately 60 Hz. Do not override the ini back to 1×. The game's frame
   rate may report approximately 30 FPS for an interlaced framebuffer; VI should
   remain approximately 60 Hz. If Vulkan is unavailable, record the device /
   driver issue before changing the test configuration.
6. Virtual SD: enable it and make sure it's writable. Saves go to
   `sd:/apps/backyardcornhole/data/save.dat`.
7. Launch the ISO from the Wii ROM list. Properties must show game ID **BYCEAO**,
   region USA. Check both players' connection, menu input and tosses, including
   reconnects.

Fallback: if the per-game ini isn't picked up, set the **global** Wii Remote 1
(and 2) source to Real Wii Remote in Controller settings (affects every game).
The 3× resolution still comes from this game's ini, so keep installing it.

Record at the next check: the real `GameSettings/` path, whether the ini
applied, virtual-SD writability, how to copy files off the SD, and Dolphin's
speed % during a full match.

Controls: hold B, swing back, swing forward, then release B. D-pad left/right
slides along the foul line; sideways motion and wrist twist add spin. Choose
LEFT/RIGHT hand for each player in SETTINGS. Hold 1+2 for two seconds to
toggle live swing diagnostics.
