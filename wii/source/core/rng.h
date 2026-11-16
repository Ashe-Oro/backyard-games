#ifndef BYC_RNG_H
#define BYC_RNG_H

#include <stdint.h>
#include <stdbool.h>
#include "physics.h"

/* mulberry32, identical to index.html rng(seed). The Wii build uses one seeded instance where
 * 1.0.3 calls Math.random (coin toss, CPU gauss/picks, wind, tumble), so tests can fix it. */
typedef struct { uint32_t state; uint32_t draws; } Rng;
void rng_seed(Rng *r, uint32_t seed);
double rng_next(Rng *r);
double rng_gauss(Rng *r);

/* controls.mjs roundWind(). circuit < 0 means null (default tier). */
Wind round_wind(bool practice, int circuit, double previous_x, Rng *r);

#endif
