/*
 * rng.h - Deterministic pseudo-random numbers (xorshift32).
 *
 * The RNG state lives inside the game state, so save-states and replays
 * reproduce every "random" event exactly. Never call rand() in core/.
 */
#ifndef MM3_RNG_H
#define MM3_RNG_H

#include <stdint.h>

typedef struct {
    uint32_t s;
} mm3_rng;

void     mm3_rng_seed(mm3_rng *r, uint32_t seed);
uint32_t mm3_rng_next(mm3_rng *r);
/* Uniform-ish integer in [0, n). n must be > 0. */
uint32_t mm3_rng_range(mm3_rng *r, uint32_t n);

#endif
