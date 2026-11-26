/* save.dat encoding (SPEC §2.8): explicit big-endian fields, no struct dumps.
 * magic "BYCS", u16 format version, u16 payload length, payload, u32 CRC-32 (IEEE) of everything
 * before it. Any field failing validation falls back to its default. File I/O lives in the
 * platform layer (crash-safe tmp -> bak -> dat rename sequence). */
#ifndef BYC_SAVE_H
#define BYC_SAVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "swing.h"
#include "music.h"

#define SAVE_VERSION 4          /* 0.1.8 factory power reference; same layout as 2 and 3 */
#define SAVE_MAX_BYTES 256

typedef struct {
    int career_fight, career_wins;   /* valid: 0 <= fight <= 8, wins >= fight */
    SwingProfile profile[2];         /* per player slot (RED, BLUE), never per remote */
    bool help_tucked;
    bool wind_on;                    /* WIND setting CALM/ON */
    bool music_on;                   /* MUSIC: PLAY/PAUSE */
    double music_volume;             /* 0..1 in 1% steps */
    char music_folder[MUSIC_PATH_MAX];
} SaveData;

void save_defaults(SaveData *s);
size_t save_encode(const SaveData *s, uint8_t *out, size_t cap);
/* Returns false if the container is unusable (bad magic/CRC/length); *s then holds defaults.
 * Individually invalid fields are replaced by defaults and the load still succeeds. */
bool save_decode(SaveData *s, const uint8_t *in, size_t len);
uint32_t save_crc32(const uint8_t *data, size_t len);

#endif
