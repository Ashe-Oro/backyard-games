/* Original minimal DOL apploader, dedicated to the public domain (CC0-1.0).
 * Disc offsets use Wii four-byte units; sizes use bytes.
 * No Nintendo code, data, certificates or disc dump is used as source input.
 */
typedef unsigned int u32;
typedef void (*report_fn)(const char *, ...);
struct dol_header {
    u32 offset[18], address[18], size[18], bss, bss_size, entry, padding[7];
};
static struct dol_header dol __attribute__((section(".state"))) = {0};
static u32 dol_offset __attribute__((section(".state"))) = 0;
static int phase __attribute__((section(".state"))) = 0;
static int section __attribute__((section(".state"))) = 0;

static void init(report_fn report)
{
    (void)report;
    phase = section = 0;
}

static int load(void **dst, u32 *size, u32 *offset)
{
    if (phase == 0) {
        *dst = &dol_offset; *size = 4; *offset = 0x420 / 4;
        phase = 1;
        return 1;
    }
    if (phase == 1) {
        *dst = &dol; *size = sizeof(dol); *offset = dol_offset;
        phase = 2;
        return 1;
    }
    if (phase == 2) {
        /* Wii disc DOL offsets use four-byte units (unlike GameCube). */
        phase = 3;
        volatile unsigned char *p = (volatile unsigned char *)dol.bss;
        for (u32 i = 0; i < dol.bss_size; ++i) p[i] = 0;
    }
    while (section < 18) {
        int i = section++;
        if (!dol.size[i]) continue;
        *dst = (void *)dol.address[i];
        *size = dol.size[i];
        *offset = dol_offset + dol.offset[i] / 4;
        return 1;
    }
    return 0;
}

static void *final(void) { return (void *)dol.entry; }

__attribute__((section(".text.entry")))
void entry(void (**init_out)(report_fn), int (**main_out)(void **, u32 *, u32 *),
           void *(**final_out)(void))
{
    *init_out = init; *main_out = load; *final_out = final;
}
