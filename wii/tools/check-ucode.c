/* Verify a DOL embeds an ASND ucode CRC that Dolphin 5.0-19870 HLE knows.
 * HashEctor is Dolphin's DSP IRAM hash (Common::HashEctor). */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static uint32_t hash_ector(const unsigned char *data, size_t len) {
    uint32_t crc = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        crc = (crc << 3) | (crc >> 29);
    }
    return crc;
}

int main(int argc, char **argv) {
    static const struct { uint32_t crc; unsigned size; const char *name; int recognized_19870; } known[] = {
        {0x5dbf8bf1u, 0x0606, "ASND_2024", 0},
        {0x373a950eu, 0x0620, "ASND_2024_PAD", 0},
        {0x8d69a19bu, 0x05a0, "ASND_2008", 1},
        {0xcc2fd441u, 0x05c0, "ASND_2009", 1},
        {0xa81582e2u, 0x0620, "ASND_2011", 1},
        {0xdbbeeb61u, 0x0606, "ASND_2020", 1},
        {0xbad876efu, 0x0620, "ASND_2020_PAD", 1},
        {0xfa9c576fu, 0x0620, "ASND_DESERT_BUS_2011", 1},
        {0x614dd145u, 0x0620, "ASND_DESERT_BUS_2012", 1},
        {0, 0, NULL, 0}
    };
    FILE *f;
    long n, off;
    unsigned char *buf;
    int k, found_ok = 0, found_bad = 0;

    if (argc != 2) { fprintf(stderr, "usage: %s boot.dol\n", argv[0]); return 2; }
    f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    if (fseek(f, 0, SEEK_END) != 0) { perror("seek"); return 1; }
    n = ftell(f);
    if (n < 0) { perror("tell"); return 1; }
    rewind(f);
    buf = malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { perror("read"); return 1; }
    fclose(f);

    for (k = 0; known[k].name; k++) {
        unsigned size = known[k].size;
        if ((long)size > n) continue;
        for (off = 0; off + (long)size <= n; off += 4) {
            if (hash_ector(buf + off, size) == known[k].crc) {
                printf("%s crc=0x%08x size=0x%x off=0x%lx %s\n",
                       known[k].recognized_19870 ? "OK" : "BAD",
                       known[k].crc, size, off, known[k].name);
                if (known[k].recognized_19870) found_ok = 1;
                else found_bad = 1;
            }
        }
    }
    free(buf);
    if (found_bad || !found_ok) {
        fprintf(stderr, "check-ucode: %s: need a Dolphin 5.0-19870-recognized ASND ucode (e.g. 0xdbbeeb61), not 0x5dbf8bf1\n", argv[1]);
        return 1;
    }
    return 0;
}
