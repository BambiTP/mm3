/*
 * platform.h - What a frontend ("port") is responsible for.
 *
 * The core (core/) is a pure function: state + buttons -> next state.
 * Everything device-specific lives in a frontend. A port for a new device
 * (console SDK, phone, handheld, browser...) must provide:
 *
 *   1. Input   : map the device's controls to an mm3_buttons bitfield once
 *                per tick (keyboard, gamepad, touch overlay...).
 *   2. Timing  : call mm3_tick() exactly 60 times per real second using a
 *                fixed-timestep accumulator (mm3_pacer below). Rendering may
 *                run at any rate; it only ever READS the game state.
 *   3. Video   : draw the tilemap + entities from mm3_game_state.
 *   4. Audio   : play sounds for events (future: core emits an event list).
 *   5. Storage : load/save level and replay files (byte buffers; the core
 *                does the parsing so the format is identical everywhere).
 *
 * Nothing a frontend does may feed back into the simulation except the
 * buttons passed to mm3_tick(). That is what keeps replays and netplay
 * identical on every device.
 */
#ifndef MM3_PLATFORM_H
#define MM3_PLATFORM_H

#include <stdint.h>

/* Fixed-timestep accumulator in integer microseconds (no floats needed). */
typedef struct {
    int64_t accum_us;
} mm3_pacer;

#define MM3_TICK_US (1000000 / 60) /* 16666 us; the 40 us/s drift is irrelevant for pacing */
#define MM3_MAX_TICKS_PER_FRAME 5  /* avoid a "spiral of death" after a stall */

/* Feed elapsed real time; returns how many mm3_tick() calls to make now. */
static inline int mm3_pacer_advance(mm3_pacer *p, int64_t elapsed_us)
{
    int n = 0;
    if (elapsed_us > 250000) elapsed_us = 250000; /* tab was hidden, debugger, etc. */
    p->accum_us += elapsed_us;
    while (p->accum_us >= MM3_TICK_US && n < MM3_MAX_TICKS_PER_FRAME) {
        p->accum_us -= MM3_TICK_US;
        n++;
    }
    if (n == MM3_MAX_TICKS_PER_FRAME) p->accum_us = 0;
    return n;
}

#endif
