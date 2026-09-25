/*
 * physics_profiles.c - Built-in physics styles.
 *
 * These are ORIGINAL placeholder numbers tuned to evoke a "feel", not values
 * copied from any existing game. Expect them to be retuned during playtesting.
 * Once a version ships, never edit a shipped profile in place: levels store
 * their profile by value, but add a new profile version instead so replays
 * and leaderboards stay valid.
 *
 * Reference: 256 = 1 px/frame. 16 px = 1 tile. 60 frames = 1 second.
 */
#include "mm3/physics.h"

static const mm3_physics_profile k_profiles[MM3_STYLE_COUNT] = {
    /* CLASSIC: heavy momentum, air direction locked to takeoff speed. */
    {
        /* walk_max, run_max, walk_accel, run_accel, air_accel */
        384, 640, 14, 22, 14,
        /* friction, skid_decel */
        13, 26,
        /* gravity_hold, gravity_release, gravity_fall, max_fall */
        32, 96, 96, 1152,
        /* jump_vel[3], jump_tier_speed[2] */
        {1024, 1056, 1120}, {256, 512},
        /* coyote_frames, jump_buffer_frames */
        0, 0,
        /* wall_slide_max, wall_jump_vx, wall_jump_vy, ground_pound_vy */
        0, 0, 0, 0,
        /* spin_jump_vel, spin_fall_max, long_jump_vx, long_jump_vy */
        0, 0, 0, 0,
        /* abilities */
        0
    },
    /* NEW: snappier, wall jump, ground pound, spin jump. */
    {
        400, 704, 16, 24, 16,
        16, 32,
        28, 80, 64, 1024,
        {1000, 1040, 1100}, {256, 560},
        4, 4,
        384, 512, 960, 1280,
        900, 512, 0, 0,
        MM3_ABIL_WALL_JUMP | MM3_ABIL_GROUND_POUND | MM3_ABIL_SPIN_JUMP |
            MM3_ABIL_AIR_TURN
    },
    /* WORLD: tight control, long jump, fast dash. */
    {
        440, 736, 24, 30, 24,
        24, 40,
        30, 90, 72, 1100,
        {1000, 1030, 1080}, {256, 600},
        5, 6,
        320, 480, 960, 1280,
        0, 0, 900, 760,
        MM3_ABIL_WALL_JUMP | MM3_ABIL_GROUND_POUND | MM3_ABIL_LONG_JUMP |
            MM3_ABIL_CROUCH_JUMP | MM3_ABIL_WALL_CLIMB | MM3_ABIL_AIR_TURN
    },
    /* WONDER: lower momentum, floaty, very forgiving timing. */
    {
        380, 600, 20, 24, 22,
        30, 40,
        26, 80, 60, 900,
        {980, 1000, 1040}, {256, 480},
        8, 8,
        320, 480, 920, 1280,
        0, 0, 0, 0,
        MM3_ABIL_WALL_JUMP | MM3_ABIL_GROUND_POUND | MM3_ABIL_AIR_TURN
    },
    /* CUSTOM: default starting point for user tuning (same as NEW). */
    {
        400, 704, 16, 24, 16,
        16, 32,
        28, 80, 64, 1024,
        {1000, 1040, 1100}, {256, 560},
        4, 4,
        384, 512, 960, 1280,
        900, 512, 900, 760,
        MM3_ABIL_WALL_JUMP | MM3_ABIL_GROUND_POUND | MM3_ABIL_SPIN_JUMP |
            MM3_ABIL_LONG_JUMP | MM3_ABIL_AIR_TURN
    }
};

static const char *const k_names[MM3_STYLE_COUNT] = {
    "Classic", "New", "World", "Wonder", "Custom"
};

const mm3_physics_profile *mm3_builtin_profile(mm3_style style)
{
    if ((int)style < 0 || style >= MM3_STYLE_COUNT) style = MM3_STYLE_CLASSIC;
    return &k_profiles[style];
}

const char *mm3_style_name(mm3_style style)
{
    if ((int)style < 0 || style >= MM3_STYLE_COUNT) return "?";
    return k_names[style];
}
