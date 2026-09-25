/*
 * fixed.h - Fixed-point math for the deterministic simulation.
 *
 * All world positions and velocities are integers measured in 1/4096 px
 * (Q20.12). That precision was chosen because it represents the source
 * games' own units exactly:
 *   - 8-bit style: pixel + 1/16 sub + 1/256 of that  = 1/4096 px
 *   - 16-bit style: "4.12" speeds                     = 1/4096 px
 *   - modern styles: floats in px/frame, rounded to 1/4096
 * Never use float/double in core/.
 *
 * Right-shifting negative numbers and signed overflow are implementation
 * defined / undefined in C99, so every helper here uses division with an
 * explicit rounding rule instead.
 */
#ifndef MM3_FIXED_H
#define MM3_FIXED_H

#include <stdint.h>

typedef int32_t mm3_fx;

#define MM3_FX_SHIFT 12
#define MM3_FX_ONE   4096

/* Whole pixels -> fixed. */
#define MM3_PX(n) ((mm3_fx)((n) * MM3_FX_ONE))
/* 1/16 px units (16-bit style speeds, "speed units") -> fixed. */
#define MM3_SUB16(n) ((mm3_fx)((n) * 256))
/* Thousandths of a pixel -> fixed, rounded to nearest (for float sources). */
#define MM3_MILLI(n) ((mm3_fx)(((n) * MM3_FX_ONE + ((n) >= 0 ? 500 : -500)) / 1000))

/* Division that always rounds toward negative infinity (b must be > 0). */
static inline int32_t mm3_floor_div(int32_t a, int32_t b)
{
    int32_t q = a / b;
    if ((a % b) != 0 && a < 0) q--;
    return q;
}

/* Non-negative remainder matching mm3_floor_div (b must be > 0). */
static inline int32_t mm3_floor_mod(int32_t a, int32_t b)
{
    return a - mm3_floor_div(a, b) * b;
}

/* Fixed -> whole pixels, rounding toward negative infinity. */
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

/* Wrap to a signed 8-bit value (two's complement), portably. */
static inline int32_t mm3_s8(int32_t v)
{
    v &= 0xFF;
    return v >= 128 ? v - 256 : v;
}

/* Wrap to a signed 16-bit value (two's complement), portably. */
static inline int32_t mm3_s16(int32_t v)
{
    v &= 0xFFFF;
    return v >= 32768 ? v - 65536 : v;
}

#endif
