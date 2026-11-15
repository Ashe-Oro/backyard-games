/* Packaging-only executable. Does not depend on the game under development. */
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    VIDEO_Init();
    WPAD_Init();
    GXRModeObj *mode = VIDEO_HaveComponentCable() ? &TVNtsc480Prog : &TVNtsc480IntDf;
    void *xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(mode));
    VIDEO_Configure(mode);
    VIDEO_ClearFrameBuffer(mode, xfb, COLOR_BLACK);
    console_init(xfb, 0, 0, mode->fbWidth, mode->xfbHeight,
                 mode->fbWidth * VI_DISPLAY_PIX_SZ);
    printf("\x1b[2J\x1b[3;3HBACKYARD CORNHOLE\n\n  Packaging smoke stub - NTSC / 60 Hz\n");
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (mode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    unsigned long frame = 0;
    for (;;) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);
        printf("\x1b[9;3HFrame: %lu", ++frame);
        VIDEO_WaitVSync();
    }
}
