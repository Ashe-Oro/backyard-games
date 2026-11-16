/* Packaging-only executable. Does not depend on the game under development. */
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdlib.h>

/* Original 5x7 bitmap lettering; doubled pixels keep YUYV pairs aligned. */
static const struct { char letter; u8 row[7]; } font[] = {
    {'A', {14,17,17,31,17,17,17}}, {'B', {30,17,17,30,17,17,30}},
    {'C', {14,17,16,16,16,17,14}}, {'D', {30,17,17,17,17,17,30}},
    {'E', {31,16,16,30,16,16,31}}, {'H', {17,17,17,31,17,17,17}},
    {'K', {17,18,20,24,20,18,17}}, {'L', {16,16,16,16,16,16,31}},
    {'N', {17,25,25,21,19,19,17}}, {'O', {14,17,17,17,17,17,14}},
    {'R', {30,17,17,30,20,18,17}}, {'Y', {17,17,10,4,4,4,4}}
};

static void draw_title(void *xfb, unsigned width)
{
    static const char title[] = "BACKYARD CORNHOLE";
    volatile u32 *pixels = xfb;
    unsigned left = (width - (sizeof(title) - 1) * 12) / 2;
    left &= ~1u;
    for (unsigned i = 0; title[i]; ++i) {
        for (unsigned g = 0; g < sizeof(font) / sizeof(font[0]); ++g) {
            if (font[g].letter != title[i]) continue;
            for (unsigned y = 0; y < 7; ++y) {
                for (unsigned x = 0; x < 5; ++x) {
                    if (!(font[g].row[y] & (16u >> x))) continue;
                    for (unsigned dy = 0; dy < 2; ++dy)
                        pixels[((160 + 2*y + dy)*width + left + i*12 + x*2)/2] = COLOR_WHITE;
                }
            }
        }
    }
}

int main(void)
{
    VIDEO_Init();
    WPAD_Init();
    GXRModeObj *mode = VIDEO_HaveComponentCable() ? &TVNtsc480Prog : &TVNtsc480IntDf;
    void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(mode));
    VIDEO_Configure(mode);
    VIDEO_ClearFrameBuffer(mode, xfb, COLOR_BLACK);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (mode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    for (;;) {
        draw_title(xfb, mode->fbWidth);
        VIDEO_SetNextFramebuffer(xfb);
        VIDEO_Flush();
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
        VIDEO_WaitVSync();
    }
}
