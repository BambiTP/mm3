/*
 * fixed.h - Fixed-point math for the deterministic simulation.
 *
 * All world positions and velocities are integers measured in SUBPIXELS.
 * 1 pixel = 256 subpixels (Q24.8). Never use float/double in core/.
 *
 * Right-shifting negative numbers and signed overflow are implementation
 * defined / undefined in C99, so every helper here uses division with an
 * explicit rounding rule instead.
 */
#ifndef MM3_FIXED_H
#define MM3_FIXED_H

#include <stdint.h>

typedef int32_t mm3_fx;

#define MM3_FX_SHIFT 8
#define MM3_FX_ONE   256

/* Whole pixels -> subpixels. */
#define MM3_PX(n) ((mm3_fx)((n) * MM3_FX_ONE))

/* Division that always rounds toward negative infinity (b must be > 0). */
static inline int32_t mm3_floor_div(int32_t a, int32_t b)
{
    int32_t q = a / b;
    if ((a % b) != 0 && a < 0) q--;
    return q;
}

/* Subpixels -> whole pixels, rounding toward negative infinity. */
static inline int32_t mm3_fx_to_px(mm3_fx v)
{
    return mm3_floor_div(v, MM3_FX_ONE);
}

static inline int32_t mm3_abs(int32_t v) { return v < 0 ? -v : v; }
static inline int32_t mm3_sign(int32_t v) { return (v > 0) - (v < 0); }
static inline int32_t mm3_min(int32_t a, int32_t b) { return a < b ? a : b; }
static inline int32_t mm3_max(int32_t a, int32_t b) { return a > b ? a : b; }

static inline int32_t mm3_clamp(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Move v toward target by at most step (step >= 0). */
static inline int32_t mm3_approach(int32_t v, int32_t target, int32_t step)
{
    if (v < target) return (v + step > target) ? target : v + step;
    if (v > target) return (v - step < target) ? target : v - step;
    return v;
}

#endif
