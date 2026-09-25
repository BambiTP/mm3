#include "mm3/rng.h"

void mm3_rng_seed(mm3_rng *r, uint32_t seed)
{
    /* xorshift must never be seeded with zero. */
    r->s = seed ? seed : 0x9E3779B9u;
}

uint32_t mm3_rng_next(mm3_rng *r)
{
    uint32_t x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->s = x;
    return x;
}

uint32_t mm3_rng_range(mm3_rng *r, uint32_t n)
{
    return mm3_rng_next(r) % n;
}
