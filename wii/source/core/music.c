#include "music.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double clamp01(double v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

int music_compare_names(const char *a, const char *b) {
    /* localeCompare(undefined, {numeric: true}) for file names: digit runs compare by value. */
    while (*a && *b) {
        if (isdigit((unsigned char)*a) && isdigit((unsigned char)*b)) {
            while (*a == '0') a++;
            while (*b == '0') b++;
            const char *ea = a, *eb = b;
            while (isdigit((unsigned char)*ea)) ea++;
            while (isdigit((unsigned char)*eb)) eb++;
            if (ea - a != eb - b) return ea - a < eb - b ? -1 : 1;
            int c = strncmp(a, b, (size_t)(ea - a));
            if (c) return c < 0 ? -1 : 1;
            a = ea; b = eb;
            continue;
        }
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb) return ca < cb ? -1 : 1;
        a++; b++;
    }
    return *a ? 1 : *b ? -1 : 0;
}
static int compare(const void *a, const void *b) { return music_compare_names(a, b); }

static void halt(Music *m) {
    if (m->started && m->backend.stop) m->backend.stop(m->backend.user);
    m->started = false; m->paused = false;
}
/* play(): start the current track if music is enabled. A file that cannot be opened counts as a
 * failure and the next one is tried; when every file fails, music turns off (audio.onerror). */
static void play(Music *m) {
    if (!m->count || !m->enabled || !m->backend.play) return;
    if (m->started && m->paused) {
        if (m->backend.pause) m->backend.pause(m->backend.user, false);
        m->paused = false;
        return;
    }
    if (m->started) return;
    while (m->failures < m->count) {
        if (m->backend.volume) m->backend.volume(m->backend.user, m->volume);
        if (m->backend.play(m->backend.user, m->folder, m->tracks[m->index])) {
            m->started = true; m->paused = false; m->status = MUSIC_OK;
            return;
        }
        m->failures++;
        m->index = (m->index + 1) % m->count;
    }
    m->enabled = false; m->status = MUSIC_UNPLAYABLE; m->dirty = true; m->failures = 0;
}
static void scan(Music *m) {
    halt(m);
    m->count = 0; m->index = 0; m->failures = 0;
    int n = m->backend.list ? m->backend.list(m->backend.user, m->folder, m->tracks, MUSIC_MAX_TRACKS) : -1;
    if (n < 0) { m->status = MUSIC_NO_FOLDER; return; }
    if (n > MUSIC_MAX_TRACKS) n = MUSIC_MAX_TRACKS;
    m->count = n;
    qsort(m->tracks, (size_t)n, sizeof m->tracks[0], compare);
    m->status = n ? MUSIC_OK : MUSIC_EMPTY;
    play(m);
}

void music_init(Music *m, const MusicBackend *backend, bool enabled, double volume, const char *folder) {
    memset(m, 0, sizeof *m);
    if (backend) m->backend = *backend;
    m->enabled = enabled;
    m->volume = isfinite(volume) ? clamp01(volume) : MUSIC_DEFAULT_VOLUME;
    snprintf(m->folder, sizeof m->folder, "%s", folder && folder[0] ? folder : MUSIC_DEFAULT_FOLDER);
    if (m->backend.volume) m->backend.volume(m->backend.user, m->volume);
    scan(m);
}
void music_refresh(Music *m) { scan(m); }
void music_choose_folder(Music *m) {
    static char paths[MUSIC_MAX_FOLDERS][MUSIC_PATH_MAX];
    int n = m->backend.folders ? m->backend.folders(m->backend.user, paths, MUSIC_MAX_FOLDERS) : 0;
    if (n > MUSIC_MAX_FOLDERS) n = MUSIC_MAX_FOLDERS;
    if (n > 0) {
        int at = -1;
        for (int i = 0; i < n; i++) if (!strcmp(paths[i], m->folder)) at = i;
        const char *next = paths[(at + 1) % n];
        if (strcmp(next, m->folder)) { snprintf(m->folder, sizeof m->folder, "%s", next); m->dirty = true; }
    }
    scan(m);
}
void music_toggle(Music *m) {
    if (!music_can_toggle(m)) return;
    m->enabled = !m->enabled; m->dirty = true;
    if (m->enabled) { m->failures = 0; play(m); }
    else if (m->started) {
        if (m->backend.pause) { m->backend.pause(m->backend.user, true); m->paused = true; }
        else halt(m);
    }
}
void music_next(Music *m) {
    if (!m->count) return;
    halt(m);
    m->index = (m->index + 1) % m->count;
    play(m);
}
void music_volume_step(Music *m, int dir) {
    double v = clamp01(floor((m->volume + dir * 0.1) * 100 + 0.5) / 100);
    if (v == m->volume) return;
    m->volume = v; m->dirty = true;
    if (m->backend.volume) m->backend.volume(m->backend.user, v);
}
void music_update(Music *m) {
    if (!m->enabled || !m->count || (m->started && m->paused)) return;
    if (m->started) {
        if (m->backend.playing && m->backend.playing(m->backend.user)) return;
        /* onended: next track (a one-track folder loops). A file that produced no audio is an
         * onerror: skip it, and give up once every file in the folder has failed in a row. */
        bool ok = !m->backend.produced || m->backend.produced(m->backend.user);
        halt(m);
        if (ok) m->failures = 0;
        else if (++m->failures >= m->count) {
            m->enabled = false; m->status = MUSIC_UNPLAYABLE; m->dirty = true; m->failures = 0;
            return;
        }
        m->index = (m->index + 1) % m->count;
    }
    play(m);
}
void music_shutdown(Music *m) { halt(m); }
const char *music_now(const Music *m) {
    if (m->status == MUSIC_NO_FOLDER) return "Music folder unavailable.";
    if (m->status == MUSIC_UNPLAYABLE) return "Could not play these files.";
    if (!m->count) return "Add MP3s to your music folder.";
    return m->tracks[m->index];
}
bool music_can_toggle(const Music *m) { return m->count > 0; }
bool music_can_next(const Music *m) { return m->count > 1; }
