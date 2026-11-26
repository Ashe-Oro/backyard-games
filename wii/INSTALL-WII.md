# Play Backyard Cornhole 0.1.10 on Wii

You need a Wii that already has the **Homebrew Channel**, a synced Wii Remote,
and an SD card formatted as **FAT32**. If you need the Homebrew Channel, follow
[Wii Hacks Guide](https://wii.hacks.guide). This guide starts after that setup.
This release uses NTSC video (480i/480p); real-Wii playtesting is still pending.

## Install and play

1. Download `BackyardCornhole-HBC-v0.1.10.zip` from the release.
2. Unzip it to the **root of the SD card**. The files should be:

   ```text
   sd:/apps/backyardcornhole/boot.dol
   sd:/apps/backyardcornhole/meta.xml
   sd:/apps/backyardcornhole/icon.png
   ```

   Avoid an extra `BackyardCornhole-HBC-v0.1.10/` folder around `apps/`.
   `INSTALL-WII.md` at the card root is just this guide.
3. Safely eject the card, insert it into the Wii, and open the **Homebrew Channel**.
4. Pick **Backyard Cornhole**, then **Load**.

Hold **B**, swing back, then **release B** after the forward swing passes the bottom. Swing speed
sets power and swinging left or right aims. Twist your wrist to curl the bag.
**D-pad up/down** picks KOBE, NORMAL or SLIDER (how high the bag flies).
**D-pad left/right** slides along the foul line. **A** flips SLICK/STICKY. Press **1** for help or **HOME**
to pause. Choose handedness and SET POWER in settings. Hold **1+2** for two
seconds to show live motion, swing phase and the last throw or reject reason.
Press **A** on the overlay to open pose calibration. Hold **1+2** again for two seconds to hide it.

## Music and saves

Optional: put your own MP3s in `sd:/apps/backyardcornhole/music/`, then open
**MUSIC** in the game to refresh/select tracks and adjust playback.
Career and player settings save to `sd:/apps/backyardcornhole/data/save.dat`.
Keep the SD card inserted for music and saving; back up that folder before
replacing or formatting the card.

## Developer shortcut: wiiload

With devkitPro's `wiiload` installed, the Wii and computer on the same network,
and the Homebrew Channel open on the Wii, run from the repository's `wii/` folder:

```sh
export WIILOAD=tcp:192.168.1.42  # replace with your Wii's IP address
wiiload build/boot.dol
```

Build first using [BUILD.md](BUILD.md). This sends the DOL for a quick run;
use the SD layout above for the normal installed app, music and saves.

## Optional ISO / USB loader route

The release also contains `BackyardCornhole-BYCEAO.iso` (game ID **BYCEAO**).
Loading this homebrew ISO on a Wii through **USB Loader GX** or **WiiFlow** is
**untested** and may depend on your loader and console setup. If you already
use one, follow its own import instructions. The Homebrew Channel route above
is the intended way to launch this app; you do not need the ISO for it.

## Quick fixes

- **Missing from the Homebrew Channel list:** check the exact `apps/backyardcornhole/`
  folder layout above, confirm `boot.dol` is present, and check that the SD card
  is FAT32 and the Homebrew Channel is viewing the SD card.
- **Wii Remote not responding:** check batteries and sync the Remote to your Wii
  using the console and Remote SYNC buttons, then reopen the Homebrew Channel
  and load the game.
