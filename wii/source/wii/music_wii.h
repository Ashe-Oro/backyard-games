#ifndef BYC_MUSIC_WII_H
#define BYC_MUSIC_WII_H

#include "../core/music.h"

/* SD-card MP3 backend for the MUSIC screen: libfat directory listing + libogc's libmad MP3 player
 * on ASND. Plays only the player's own files; nothing is bundled. */
MusicBackend music_wii_backend(bool sd_ok);
void music_wii_shutdown(void);

#endif
