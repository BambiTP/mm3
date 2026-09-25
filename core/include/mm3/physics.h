/*
 * physics.h - Data-driven physics styles.
 *
 * Every physics style is just a mm3_physics_profile: plain numbers plus a set
 * of ability flags. The player code reads the profile; it never checks
 * "which game is this". Adding a new style = adding a new table entry.
 *
 * Units: speeds in subpixels/frame, accelerations in subpixels/frame^2,
 * at a fixed 60 ticks per second. 256 subpixels = 1 pixel, 16 px = 1 tile.
 */
#ifndef MM3_PHYSICS_H
#define MM3_PHYSICS_H

#include <stdint.h>
#include "fixed.h"

typedef enum {
    MM3_STYLE_CLASSIC = 0, /* heavy momentum, committed jumps            */
    MM3_STYLE_NEW     = 1, /* snappier, wall jump, ground pound, spin    */
    MM3_STYLE_WORLD   = 2, /* tight control, long jump, dash             */
    MM3_STYLE_WONDER  = 3, /* forgiving, floaty, generous timing windows */
    MM3_STYLE_CUSTOM  = 4, /* fully user-tunable                         */
    MM3_STYLE_COUNT
} mm3_style;

/* Ability flags: which moves a style allows. */
enum {
    MM3_ABIL_WALL_JUMP    = 1u << 0,
    MM3_ABIL_GROUND_POUND = 1u << 1,
    MM3_ABIL_SPIN_JUMP    = 1u << 2,
    MM3_ABIL_LONG_JUMP    = 1u << 3,
    MM3_ABIL_CROUCH_JUMP  = 1u << 4, /* TODO: not implemented yet */
    MM3_ABIL_WALL_CLIMB   = 1u << 5, /* TODO: not implemented yet */
    MM3_ABIL_AIR_TURN     = 1u << 6  /* can reverse direction freely in the air */
};

#define MM3_JUMP_TIERS 3

typedef struct {
    /* Horizontal movement */
    int32_t walk_max;         /* top speed without RUN held             */
    int32_t run_max;          /* top speed with RUN held                */
    int32_t walk_accel;
    int32_t run_accel;
    int32_t air_accel;
    int32_t friction;         /* deceleration with no direction held    */
    int32_t skid_decel;       /* deceleration when pushing against vx   */

    /* Vertical movement */
    int32_t gravity_hold;     /* rising while JUMP held                 */
    int32_t gravity_release;  /* rising after JUMP released             */
    int32_t gravity_fall;     /* falling                                */
    int32_t max_fall;

    /* Jump launch speed depends on |vx| at takeoff (faster run = higher). */
    int32_t jump_vel[MM3_JUMP_TIERS];
    int32_t jump_tier_speed[MM3_JUMP_TIERS - 1]; /* |vx| thresholds */

    /* Forgiveness windows (frames) */
    int32_t coyote_frames;      /* jump allowed this long after leaving ground */
    int32_t jump_buffer_frames; /* early jump press remembered this long        */

    /* Ability parameters (ignored if the ability flag is off) */
    int32_t wall_slide_max;
    int32_t wall_jump_vx;
    int32_t wall_jump_vy;
    int32_t ground_pound_vy;
    int32_t spin_jump_vel;
    int32_t spin_fall_max;
    int32_t long_jump_vx;
    int32_t long_jump_vy;

    uint32_t abilities;
} mm3_physics_profile;

/* Built-in, versioned profile for a style. Custom returns a sane default. */
const mm3_physics_profile *mm3_builtin_profile(mm3_style style);
const char *mm3_style_name(mm3_style style);

#endif
