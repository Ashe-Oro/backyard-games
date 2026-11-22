#include "save.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void save_defaults(SaveData *s) {
    memset(s, 0, sizeof(*s));
    s->profile[0] = s->profile[1] = swing_default_profile();
    s->music_volume = MUSIC_DEFAULT_VOLUME;
    snprintf(s->music_folder, sizeof s->music_folder, "%s", MUSIC_DEFAULT_FOLDER);
}

uint32_t save_crc32(const uint8_t *d, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= d[i];
        for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
    }
    return ~c;
}

typedef struct { uint8_t *p; size_t n, cap; bool ok; } W;
static void put8(W *w, uint8_t v) { if (w->n < w->cap) w->p[w->n] = v; else w->ok = false; w->n++; }
static void put16(W *w, uint16_t v) { put8(w, (uint8_t)(v >> 8)); put8(w, (uint8_t)v); }
static void put32(W *w, uint32_t v) { put16(w, (uint16_t)(v >> 16)); put16(w, (uint16_t)v); }
static void putf64(W *w, double d) {
    uint64_t u; memcpy(&u, &d, 8);
    put32(w, (uint32_t)(u >> 32)); put32(w, (uint32_t)u);
}

size_t save_encode(const SaveData *s, uint8_t *out, size_t cap) {
    W w = {out, 0, cap, true};
    put8(&w, 'B'); put8(&w, 'Y'); put8(&w, 'C'); put8(&w, 'S');
    put16(&w, SAVE_VERSION);
    size_t len_at = w.n; put16(&w, 0);
    size_t start = w.n;
    put8(&w, (uint8_t)s->career_fight); put16(&w, (uint16_t)s->career_wins);
    for (int i = 0; i < 2; i++) {
        putf64(&w, s->profile[i].full_speed);
        putf64(&w, s->profile[i].spin_strength);
        put8(&w, s->profile[i].left_handed ? 1 : 0);
    }
    put8(&w, s->help_tucked ? 1 : 0);
    put8(&w, s->wind_on ? 1 : 0);
    put8(&w, s->music_on ? 1 : 0);
    put8(&w, (uint8_t)lround(fmax(0, fmin(1, s->music_volume)) * 100));
    size_t folder = 0;
    while (folder < sizeof s->music_folder - 1 && s->music_folder[folder]) folder++;
    put8(&w, (uint8_t)folder);
    for (size_t i = 0; i < folder; i++) put8(&w, (uint8_t)s->music_folder[i]);
    size_t payload = w.n - start;
    if (w.ok) { out[len_at] = (uint8_t)(payload >> 8); out[len_at + 1] = (uint8_t)payload; }
    uint32_t crc = w.ok ? save_crc32(out, w.n) : 0;
    put32(&w, crc);
    return w.ok ? w.n : 0;
}

typedef struct { const uint8_t *p; size_t n, i; } R;
static uint8_t get8(R *r) { return r->i < r->n ? r->p[r->i++] : 0; }
static uint16_t get16(R *r) { uint16_t a = get8(r); return (uint16_t)(a << 8 | get8(r)); }
static uint32_t get32(R *r) { uint32_t a = get16(r); return a << 16 | get16(r); }
static double getf64(R *r) { uint64_t hi = get32(r), lo = get32(r), u = hi << 32 | lo; double d; memcpy(&d, &u, 8); return d; }

bool save_decode(SaveData *s, const uint8_t *in, size_t len) {
    save_defaults(s);
    if (len < 12 || memcmp(in, "BYCS", 4) != 0) return false;
    R r = {in, len, 4};
    uint16_t version = get16(&r), payload = get16(&r);
    if (version < 1 || version > SAVE_VERSION || (size_t)payload + 12 != len) return false;
    uint32_t crc = (uint32_t)in[len - 4] << 24 | (uint32_t)in[len - 3] << 16 | (uint32_t)in[len - 2] << 8 | in[len - 1];
    if (save_crc32(in, len - 4) != crc) return false;
    r.n = len - 4;
    int fight = get8(&r), wins = get16(&r);
    if (fight >= 0 && fight <= 8 && wins >= fight) { s->career_fight = fight; s->career_wins = wins; }
    for (int i = 0; i < 2; i++) {
        SwingProfile p = {getf64(&r), getf64(&r), false};
        uint8_t hand = get8(&r);
        SwingProfile d = swing_default_profile();
        if (version < 3 && p.full_speed == swing_speed(1.2)) p.full_speed = d.full_speed;
        s->profile[i].full_speed = isfinite(p.full_speed) && p.full_speed > 0.05 && p.full_speed < 20 ? p.full_speed : d.full_speed;
        s->profile[i].spin_strength = isfinite(p.spin_strength) && p.spin_strength >= 0 && p.spin_strength <= 1 ? p.spin_strength : d.spin_strength;
        s->profile[i].left_handed = hand == 1;
    }
    s->help_tucked = get8(&r) == 1;
    s->wind_on = get8(&r) == 1;
    if (version >= 2) {
        s->music_on = get8(&r) == 1;
        uint8_t volume = get8(&r), folder = get8(&r);
        if (volume <= 100) s->music_volume = volume / 100.0;
        if (folder > 0 && folder < sizeof s->music_folder && r.i + folder <= r.n) {
            bool printable = true;
            for (size_t i = 0; i < folder; i++) if (in[r.i + i] < 0x20 || in[r.i + i] > 0x7e) printable = false;
            if (printable) { memcpy(s->music_folder, in + r.i, folder); s->music_folder[folder] = 0; }
        }
        r.i += folder;
    }
    return true;
}
