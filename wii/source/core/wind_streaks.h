/* Decorative wind gusts: a port of cornhole/wind-streaks.js (1.0.3). 24 streaks, own LCG seed, two
 * dashes each; never touches bag physics. The renderer draws the vertices as GX_LINES. */
#ifndef BYC_WIND_STREAKS_H
#define BYC_WIND_STREAKS_H

#include <stdbool.h>
#include "physics.h"

#define STREAK_COUNT 24
#define STREAK_VERTS 4            /* two line segments: 0-.65 and .85-1.1 of the streak length */

typedef struct { double x, y, z, phase, size; } Streak;
typedef struct {
    Streak streaks[STREAK_COUNT];
    double seconds;
    bool visible;
    int active;                   /* streaks drawn this frame: round(4 + strength * 20) */
    double opacity;               /* .035 + strength * .095 */
    Vec3 verts[STREAK_COUNT * STREAK_VERTS];
} WindStreaks;

void wind_streaks_init(WindStreaks *w);
/* enabled: false in menus / rival intro / champion screens (as index.html). */
void wind_streaks_update(WindStreaks *w, double dt, const Wind *wind, bool enabled);

#endif
