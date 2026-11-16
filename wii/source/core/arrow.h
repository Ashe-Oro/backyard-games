/* Combined direction + power arrow (arcade 1.0.7 parity, COMBINED_ARROW_NOTES.md). Platform-free so
 * the exact HUD geometry is host-tested: one compact arrow from the bag/stance, pointing along the
 * launch bearing projected through the main camera, whose length and fill show power on a faint
 * full-power track. There is no separate power meter. */
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

/* 1.0.7 uses a 110 px full-power shaft with a 24 px floor on the cabinet's 1024-wide canvas; the
 * same proportions on the Wii's 640x480 virtual screen. */
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

/* Projects a world point with the main camera onto the 640x480 virtual screen. */
bool view_project(Vec3 world, double aspect, double *sx, double *sy);
double arrow_length(double power);
/* 1 while live or within the hold, then a linear fade to 0 over ARROW_FADE_MS. */
double arrow_alpha(double age_ms, bool live);
/* release_x: stance x at release height; yaw: launch yaw (atan2(v.x, -v.z)); power: 0..1. */
bool arrow_shape(double release_x, double yaw, double power, double aspect, ArrowShape *out);

#endif
