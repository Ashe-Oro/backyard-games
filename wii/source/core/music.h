/* MUSIC screen logic: a port of arcade/music-player.mjs + music-library.cjs (1.0.3). Platform-free:
 * the platform supplies a MusicBackend (folder listing + a decoder). Plays the player's own files
 * from the SD card; the game never bundles music. */
#ifndef BYC_MUSIC_H
#define BYC_MUSIC_H

#include <stdbool.h>

#define MUSIC_MAX_TRACKS 128
#define MUSIC_MAX_FOLDERS 16
#define MUSIC_NAME_MAX 64
#define MUSIC_PATH_MAX 64
#define MUSIC_DEFAULT_VOLUME 0.25
#define MUSIC_DEFAULT_FOLDER "sd:/apps/backyardcornhole/music"

typedef struct {
    void *user;
    /* Playable file names in `folder` (any order). Returns the count, or -1 if the folder is unavailable. */
    int (*list)(void *user, const char *folder, char names[][MUSIC_NAME_MAX], int cap);
    /* Folders the player can choose from (CHOOSE FOLDER cycles through them). Returns the count. */
    int (*folders)(void *user, char paths[][MUSIC_PATH_MAX], int cap);
    bool (*play)(void *user, const char *folder, const char *name);   /* false: cannot open/decode */
    void (*stop)(void *user);
    void (*pause)(void *user, bool paused);
    void (*volume)(void *user, double volume);                         /* 0..1 */
    bool (*playing)(void *user);                                       /* false once the track ended or failed */
    bool (*produced)(void *user);                                      /* the current/last track decoded audio */
} MusicBackend;

typedef enum { MUSIC_OK, MUSIC_EMPTY, MUSIC_NO_FOLDER, MUSIC_UNPLAYABLE } MusicStatus;

typedef struct {
    MusicBackend backend;
    char tracks[MUSIC_MAX_TRACKS][MUSIC_NAME_MAX];
    int count, index;
    bool enabled;                 /* PLAY MUSIC / PAUSE MUSIC (saved) */
    double volume;                /* saved */
    char folder[MUSIC_PATH_MAX];  /* saved */
    MusicStatus status;
    bool started, paused;         /* a track is open in the backend / held by PAUSE MUSIC */
    int failures;
    bool dirty;                   /* a saved setting changed */
} Music;

void music_init(Music *m, const MusicBackend *backend, bool enabled, double volume, const char *folder);
void music_refresh(Music *m);          /* REFRESH: rescan the folder, restart from the first track */
void music_choose_folder(Music *m);    /* CHOOSE FOLDER: next available folder, then rescan */
void music_toggle(Music *m);           /* PLAY MUSIC / PAUSE MUSIC */
void music_next(Music *m);             /* NEXT TRACK */
void music_volume_step(Music *m, int dir);   /* -1 / +1 = 10% */
void music_update(Music *m);           /* each frame: advance when a track ends, skip unplayable files */
void music_shutdown(Music *m);
const char *music_now(const Music *m); /* track name or the arcade's status line */
bool music_can_toggle(const Music *m);
bool music_can_next(const Music *m);
int music_compare_names(const char *a, const char *b);   /* case-insensitive, numeric-aware */

#endif
