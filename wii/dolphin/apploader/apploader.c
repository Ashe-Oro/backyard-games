/* Minimal Wii disc apploader for the Dolphin ISO (SPEC §1.5). Original code written for this
 * project (same license as the repository); no Nintendo apploader is used or bundled.
 *
 * Protocol (as run by Dolphin's BS2 emulation, Core/Boot/Boot_BS2Emu.cpp): the loader is copied
 * to 0x81200000 and apploader_entry() hands back init/main/close. main() is called until it
 * returns 0; each call names one disc read (destination, length, offset >> 2 on Wii).
 * close() returns the DOL entry point. Reads: boot.bin header -> DOL header -> each section.
 *
 * Built -msdata=none / -G 0 (no small-data registers exist in this context) and freestanding.
 * State lives at fixed scratch addresses above the loader so there is no .bss to zero. */
typedef unsigned int u32;
typedef void (*report_fn)(const char *fmt, ...);

typedef struct {
    u32 text_off[7], data_off[11];
    u32 text_addr[7], data_addr[11];
    u32 text_size[7], data_size[11];
    u32 bss_addr, bss_size, entry;
} DolHeader;

#define BOOT_BUF ((volatile u32 *)0x81280000)      /* boot.bin: first 0x440 bytes */
#define DOL_BUF ((volatile DolHeader *)0x81281000) /* DOL header: 0x100 bytes */
#define STATE ((volatile u32 *)0x81282000)         /* [0] stage, [1] next section, [2] dol offset >> 2 */

static void app_init(report_fn report) {
    (void)report;
    STATE[0] = 0; STATE[1] = 0; STATE[2] = 0;
}

static int app_main(void **dst, u32 *len, u32 *off) {
    if (STATE[0] == 0) {
        *dst = (void *)BOOT_BUF; *len = 0x440; *off = 0;
        STATE[0] = 1;
        return 1;
    }
    if (STATE[0] == 1) {
        STATE[2] = BOOT_BUF[0x420 / 4];               /* Wii boot.bin stores the DOL offset >> 2 */
        *dst = (void *)DOL_BUF; *len = 0x100; *off = STATE[2];
        STATE[0] = 2;
        return 1;
    }
    while (STATE[1] < 18) {
        u32 i = STATE[1]++;
        u32 size = i < 7 ? DOL_BUF->text_size[i] : DOL_BUF->data_size[i - 7];
        if (!size) continue;
        u32 addr = i < 7 ? DOL_BUF->text_addr[i] : DOL_BUF->data_addr[i - 7];
        u32 foff = i < 7 ? DOL_BUF->text_off[i] : DOL_BUF->data_off[i - 7];
        *dst = (void *)addr; *len = size; *off = STATE[2] + (foff >> 2);
        return 1;
    }
    STATE[0] = 3;
    return 0;
}

static void *app_close(void) { return (void *)DOL_BUF->entry; }

__attribute__((section(".text.entry")))
void apploader_entry(void (**init)(report_fn), int (**main_fn)(void **, u32 *, u32 *), void *(**close_fn)(void)) {
    *init = app_init;
    *main_fn = app_main;
    *close_fn = app_close;
}
