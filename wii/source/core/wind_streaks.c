#include "wind_streaks.h"
#include <math.h>
#include <stdint.h>

void wind_streaks_init(WindStreaks *w) {
    uint32_t seed = 57413;
#define RANDOM() ((seed = seed * 1664525u + 1013904223u) / 4294967296.0)
    for (int i = 0; i < STREAK_COUNT; i++) {
        Streak *s = &w->streaks[i];
        s->x = (RANDOM() - .5) * 5.6; s->y = .25 + RANDOM() * 2.2; s->z = -2 - RANDOM() * 10;
        s->phase = RANDOM(); s->size = .75 + RANDOM() * .5;
    }
#undef RANDOM
    w->seconds = 0; w->visible = false; w->active = 0; w->opacity = 0;
}

void wind_streaks_update(WindStreaks *w, double dt, const Wind *wind, bool enabled) {
    static const double along[STREAK_VERTS] = {0, .65, .85, 1.1};
    double magnitude = hypot(wind->x, wind->z);
    w->visible = enabled && wind->mph > 0 && magnitude > 0;
    if (!w->visible) { w->active = 0; return; }
    w->seconds += dt;
    double strength = fmax(0, fmin(1, wind->mph / 15));
    int active = (int)floor(4 + strength * 20 + .5);
    double lifetime = 3.6 - strength * 1.6;
    double dx = wind->x / magnitude, dz = wind->z / magnitude, travel = (.35 + wind->mph * .12) * lifetime;
    w->opacity = .035 + strength * .095; w->active = active;
    for (int i = 0; i < active; i++) {
        const Streak *s = &w->streaks[i];
        double life = fmod(w->seconds / lifetime + s->phase, 1), envelope = sin(life * BYC_PI);
        double length = (.22 + strength * .38) * s->size * envelope;
        double x = s->x + dx * (life - .5) * travel, z = s->z + dz * (life - .5) * travel;
        double y = s->y + sin(life * BYC_PI * 2 + s->phase * 6) * .035;
        for (int v = 0; v < STREAK_VERTS; v++)
            w->verts[i * STREAK_VERTS + v] = (Vec3){x - dx * length * along[v], y, z - dz * length * along[v]};
    }
}
