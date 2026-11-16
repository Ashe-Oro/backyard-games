# Backyard Cornhole on Kinhank — Retro handoff

These are manual installation instructions for Retro. Packaging and desktop
smoke testing do not connect to the Kinhank or change its files.
This is the playable Wii 0.1.0 release. Desktop smoke does not validate real
Wiimote feel or Android performance. Owner: Ashe.

Verify `sha256sum -c SHA256SUMS` before copying. The release ZIP also includes
`apps/backyardcornhole/{boot.dol,meta.xml,icon.png}` for a softmodded Wii SD card.
Controls: hold B, backswing, forward swing, release B. D-pad left/right slides
along the foul line; sideways motion and wrist twist add spin. Choose LEFT/RIGHT
hand for each player in SETTINGS. No IR or pointing is needed.

1. Connect ADB to `192.168.4.59:5555` from a machine on the cabinet's network:

   ```sh
   adb connect 192.168.4.59:5555
   adb -s 192.168.4.59:5555 push BackyardCornhole-BYCEAO.iso /storage/3050BAA350BA6EEA0/Roms/wii/
   ```

2. In Dolphin Android, use **Open User Folder** (or the app's user-data export /
   import facility) to locate the active `GameSettings` directory. Copy
   `GameSettings/BYCEAO.ini` there. The expected file content is:

   ```ini
   [Controls]
   WiimoteSource0 = 2
   WiimoteSource1 = 2
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

3. Set the MAYFLASH DolphinBar to **mode 4**. Pair two Wii Remotes to the bar; in Dolphin,
   confirm Wii Remote 1 and 2 are **Real Wii Remote**. The ini is keyed to
   disc ID `BYCEAO`, independent of the ISO filename.
4. Set graphics backend **Vulkan**, internal resolution **1× native**, and
   confirm the game boots in NTSC / approximately 60 Hz. The game's frame rate
   may report approximately 30 FPS for an interlaced framebuffer; VI should
   remain approximately 60 Hz. If Vulkan is unavailable, record the device /
   driver issue before changing the test configuration.
5. Launch the ISO from the Wii ROM list. For the stub, confirm
   **BACKYARD CORNHOLE** is visible and HOME exits. For the actual game, check
   both players' connection, menu input and tosses, including reconnects.

Copy `apps/backyardcornhole/` to `sd:/apps/backyardcornhole/` for the Homebrew
Channel on a softmodded Wii. The ISO and HBC package contain the same DOL.
