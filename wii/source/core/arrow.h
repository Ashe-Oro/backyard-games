/* Combined direction + power arrow (arcade 1.0.7 parity, COMBINED_ARROW_NOTES.md). Platform-free so
 * the exact HUD geometry is host-tested: one compact arrow from the bag/stance, pointing along the
 * launch bearing projected through the main camera, whose length and fill show power on a faint
 * full-power track. */
#ifndef BYC_ARROW_H
#define BYC_ARROW_H

#include <stdbool.h>
#include "physics.h"

/* Main camera (index.html: PerspectiveCamera(18) at (0, 3.6, 8) looking at (0, .5, -8.7)). */
#define VIEW_FOV_DEG 18.0
#define VIEW_EYE_X 0.0
#define VIEW_EYE_Y 3.6
#define VIEW_EYE_Z 8.0
#define VIEW_AT_X 0.0
#define VIEW_AT_Y 0.5
#define VIEW_AT_Z (-8.7)
#define VIEW_W 640.0
#define VIEW_H 480.0
#define VIEW_HELD_BAG_PAD 26.0f   /* UI pixels around the projected held bag center */

/* 1.0.7 uses a 110 px full-power shaft with a 24 px floor on the cabinet's 1024-wide canvas; the
 * same proportions on the Wii's 480-high virtual screen (640 wide at 4:3, 853.3 at 16:9). */
#define ARROW_MAX_PX 70.0
#define ARROW_MIN_PX 15.0
#define ARROW_HOLD_MS 2000.0   /* ARROW_LINGER_MS */
#define ARROW_FADE_MS 400.0

typedef struct {
    double ox, oy;        /* bag/stance on screen */
    double dx, dy;        /* unit screen direction of the launch bearing */
    double length;        /* coloured shaft: max(ARROW_MIN_PX, power * ARROW_MAX_PX) */
    double tx, ty;        /* shaft tip */
    double ex, ey;        /* end of the faint full-power track */
} ArrowShape;

/* Width of the square-pixel 2D screen for a display aspect: 640 at 4:3, 853.3 at 16:9.
 * Height is always VIEW_H. The HUD draws in this space, so 16:9 keeps its proportions. */
double view_ui_width(double aspect);
/* Projects a world point with the main camera onto the view_ui_width(aspect) x 480 screen. */
bool view_project(Vec3 world, double aspect, double *sx, double *sy);
/* World-space center of the held bag, including the armed stance dip (0..1). */
Vec3 view_held_bag_center(double release_x, double dip);
/* BOARD · 27 FT inset viewport in 2D screen units (view_ui_width wide). 4:3 keeps the 0.1.2
 * rect; 16:9 uses the free top-left corner so the beer-pong table and players stay clear.
 * The drawn frame adds 3 px each side and a 19 px title bar above (hud.c board_view_frame). */
void view_board_inset(double aspect, int *x, int *y, int *w, int *h);
double arrow_length(double power);
/* 1 while live or within the hold, then a linear fade to 0 over ARROW_FADE_MS. */
double arrow_alpha(double age_ms, bool live);
/* release_x: stance x at release height; yaw: launch yaw (atan2(v.x, -v.z)); power: 0..1. */
bool arrow_shape(double release_x, double yaw, double power, double aspect, ArrowShape *out);

#endif
