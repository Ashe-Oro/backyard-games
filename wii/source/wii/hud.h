#ifndef BYC_HUD_H
#define BYC_HUD_H

#include "../core/game.h"
#include "gfx.h"
#include "scene.h"

void hud_draw(const Game *g, const Input *in);
void hud_board_view_rect(int *x, int *y, int *w, int *h);
bool hud_board_view_visible(const Game *g);

enum { HUD_WIIMOTE_POSES = 5 };
typedef enum { HUD_POSE_IDLE, HUD_POSE_CAPTURING, HUD_POSE_INTERRUPTED, HUD_POSE_MOVED } HudPoseStatus;
typedef struct {
    bool active;
    unsigned pose;
    HudPoseStatus status;
    bool captured[HUD_WIIMOTE_POSES];
    SwingVector poses[HUD_WIIMOTE_POSES];
} HudWiimoteCalibration;

void hud_draw_wiimote_debug(const Input *in, const InputSample *latest,
                           const HudWiimoteCalibration *cal, bool static_pose);

#endif
