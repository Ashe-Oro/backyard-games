/* MUSIC backend (arcade music-library.cjs / <audio> on the cabinet). Files come from the SD card:
 * sd:/apps/backyardcornhole/music by default, or any folder CHOOSE FOLDER offers. MP3 only: that
 * is the decoder libogc ships (libmad); the arcade's own hint is "Add MP3s to your music folder." */
#include "music_wii.h"
#include <gccore.h>
#include <asndlib.h>
#include <mp3player.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define MAX_FILE_BYTES (100L * 1024 * 1024)   /* music-library.cjs skips files over 100 MB */

static bool sd_mounted, audio_up, paused;
static FILE *track;
static volatile bool decoded;

static bool is_mp3(const char *name) {
    size_t n = strlen(name);
    return n > 4 && name[0] != '.' && !strcasecmp(name + n - 4, ".mp3");
}
static int list(void *user, const char *folder, char names[][MUSIC_NAME_MAX], int cap) {
    (void)user;
    if (!sd_mounted) return -1;
    DIR *d = opendir(folder);
    if (!d) return -1;
    int n = 0;
    struct dirent *e;
    while (n < cap && (e = readdir(d))) {
        if (!is_mp3(e->d_name) || strlen(e->d_name) >= MUSIC_NAME_MAX) continue;
        char path[MUSIC_PATH_MAX + MUSIC_NAME_MAX + 2];
        snprintf(path, sizeof path, "%s/%s", folder, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size > MAX_FILE_BYTES) continue;
        snprintf(names[n++], MUSIC_NAME_MAX, "%s", e->d_name);
    }
    closedir(d);
    return n;
}
static bool has_dir(const char *path) {
    DIR *d = opendir(path);
    if (d) closedir(d);
    return d != NULL;
}
/* CHOOSE FOLDER: the game's own music folder, the card's common music folders, and their
 * immediate sub-folders (albums). The cabinet opens a folder dialog; a Wii Remote cycles a list. */
static int folders(void *user, char paths[][MUSIC_PATH_MAX], int cap) {
    (void)user;
    static const char *const roots[] = {MUSIC_DEFAULT_FOLDER, "sd:/music", "sd:/mp3"};
    int n = 0;
    if (!sd_mounted) return 0;
    for (unsigned r = 0; r < sizeof roots / sizeof roots[0] && n < cap; r++) {
        if (r > 0 && !has_dir(roots[r])) continue;      /* the default is always offered */
        snprintf(paths[n++], MUSIC_PATH_MAX, "%s", roots[r]);
        DIR *d = opendir(roots[r]);
        if (!d) continue;
        struct dirent *e;
        while (n < cap && (e = readdir(d))) {
            if (e->d_name[0] == '.' || strlen(roots[r]) + strlen(e->d_name) + 2 > MUSIC_PATH_MAX) continue;
            char sub[MUSIC_PATH_MAX * 2];
            snprintf(sub, sizeof sub, "%s/%s", roots[r], e->d_name);
            struct stat st;
            if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode)) snprintf(paths[n++], MUSIC_PATH_MAX, "%.*s", MUSIC_PATH_MAX - 1, sub);
        }
        closedir(d);
    }
    return n;
}
static s32 reader(void *file, void *buffer, s32 len) {
    size_t n = fread(buffer, 1, (size_t)len, file);
    return n ? (s32)n : -1;
}
static void on_frame(struct mad_stream *stream, struct mad_frame *frame) { (void)stream; (void)frame; decoded = true; }
static void stop(void *user) {
    (void)user;
    if (!audio_up) return;
    if (paused) { ASND_Pause(0); paused = false; }   /* the decoder thread must be able to exit */
    MP3Player_Stop();
    if (track) { fclose(track); track = NULL; }
}
static bool play(void *user, const char *folder, const char *name) {
    if (!audio_up || !sd_mounted) return false;
    stop(user);
    char path[MUSIC_PATH_MAX + MUSIC_NAME_MAX + 2];
    snprintf(path, sizeof path, "%s/%s", folder, name);
    track = fopen(path, "rb");
    if (!track) return false;
    decoded = false;
    if (MP3Player_PlayFile(track, reader, on_frame) < 0) { fclose(track); track = NULL; return false; }
    return true;
}
static void pause_music(void *user, bool on) {
    (void)user;
    if (!audio_up || on == paused) return;
    ASND_Pause(on ? 1 : 0);
    paused = on;
}
static void volume(void *user, double v) {
    (void)user;
    if (audio_up) MP3Player_Volume((u32)(v < 0 ? 0 : v > 1 ? 255 : v * 255));
}
static bool playing(void *user) { (void)user; return audio_up && track && MP3Player_IsPlaying(); }
static bool produced(void *user) { (void)user; return decoded; }

MusicBackend music_wii_backend(bool sd_ok) {
    sd_mounted = sd_ok;
    if (!audio_up) { ASND_Init(); MP3Player_Init(); audio_up = true; }
    MusicBackend b = {NULL, list, folders, play, stop, pause_music, volume, playing, produced};
    return b;
}
void music_wii_shutdown(void) {
    if (!audio_up) return;
    stop(NULL);
    ASND_End();
    audio_up = false;
}
