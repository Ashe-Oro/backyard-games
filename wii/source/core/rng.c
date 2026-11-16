#include "rng.h"
#include <math.h>

void rng_seed(Rng *r, uint32_t seed) { r->state = seed; r->draws = 0; }

double rng_next(Rng *r) {
    /* seed = seed + 0x6D2B79F5 | 0; t = imul(seed ^ seed >>> 15, 1 | seed); ... */
    r->draws++;
    uint32_t s = r->state += 0x6D2B79F5u;
    uint32_t t = (s ^ (s >> 15)) * (1u | s);
    t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
    return (double)(t ^ (t >> 14)) / 4294967296.0;
}

double rng_gauss(Rng *r) {
    double u = 0, v = 0;
    while (!u) u = rng_next(r);
    while (!v) v = rng_next(r);
    return sqrt(-2 * log(u)) * cos(2 * BYC_PI * v);
}

Wind round_wind(bool practice, int circuit, double previous_x, Rng *r) {
    static const int tiers[BYC_WIND_TIER_COUNT][2] = BYC_WIND_TIERS;
    if (practice) return (Wind){0, 0, 0, 0};
    int minimum = BYC_WIND_DEFAULT_MIN, steps = BYC_WIND_DEFAULT_STEPS;
    if (circuit >= 0 && circuit < BYC_WIND_TIER_COUNT) { minimum = tiers[circuit][0]; steps = tiers[circuit][1]; }
    double side = previous_x != 0 ? -(previous_x > 0 ? 1 : -1) : (rng_next(r) < .5 ? -1 : 1);
    double angle = side * BYC_PI / 2 + (rng_next(r) - .5) * BYC_PI / 3;
    double mph = minimum + floor(rng_next(r) * steps);
    Wind w = {sin(angle) * mph * BYC_WIND_MPH_TO_ACCEL, -cos(angle) * mph * BYC_WIND_MPH_TO_ACCEL, mph, angle};
    return w;
}
