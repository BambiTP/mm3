/*
 * physics_profiles.c - Modern-engine profiles.
 *
 * Retro (8-bit) and Island (16-bit) don't use these tables; their exact
 * constants live in engine_retro.c and engine_island.c.
 *
 * Every number is tagged in docs/PHYSICS.md as:
 *   Exact     - taken from a community decompilation of the source game
 *   Estimated - tuned to match the source game's feel; no public data
 * MM3_MILLI(n) = n thousandths of a pixel per frame (or per frame^2).
 * Gravity values are down-positive; thresholds are up-positive speeds.
 */
#include "mm3/physics.h"

#define M(n) MM3_MILLI(n)

static const mm3_modern_profile k_modern = {
    /* MODERN (source: NSMB Wii). */
    /* walk_max, run_max, dash_max, dash_frames          (Estimated) */
    M(1500), M(3000), 0, 0,
    /* accel, run_accel, friction, turn_decel            (turn: Exact 0.12) */
    M(60), M(60), M(100), M(120),
    /* air_accel, air_turn                               (Estimated) */
    M(40), M(60),
    /* jump_vel 3.628                                    (Exact) */
    M(3628),
    /* jump bonus speed bands 0.7/1.5/2.8, bonus 0/.18/.24/.3 (Exact) */
    {M(700), M(1500), M(2800)}, {0, M(180), M(240), M(300)},
    /* double %, triple % (3rd jump x1.05), chain window, chain min speed */
    100, 105, 10, M(1),
    /* gravity thresholds 2.5/1.5/0.3/-0.12/-3.0          (Exact) */
    {M(2500), M(1500), M(300), M(-120), M(-3000)},
    /* held: .06/.25/.34/.08/.31/.34                     (Exact) */
    {M(60), M(250), M(340), M(80), M(310), M(340)},
    /* released: .34/.34/.34/.25/.34/.34                 (Exact) */
    {M(340), M(340), M(340), M(250), M(340), M(340)},
    /* max_fall 4.0                                      (Exact) */
    M(4000),
    /* spin_jump_vel, spin_gravity, spin_max_fall         (Estimated) */
    M(3400), M(200), M(1800),
    /* twirl_vel, twirl_frames, twirl_max_fall            (Estimated) */
    M(1000), 24, M(1200),
    /* wall_slide_max 2.0 (Exact const, usage inferred), wall jump vx/vy, lock */
    M(2000), M(2000), M(3600), 10,
    /* pound windup frames, fall speed, landing lag       (Estimated) */
    16, M(6000), 12,
    /* long jump / backflip / sideflip (not in this style) */
    0, 0, 0, 0, 0, 0,
    /* crouch_jump_vel                                   (Estimated) */
    M(3200),
    /* slide_friction, slope_accel                       (Estimated) */
    M(30), M(100),
    /* swim: max x 1.125, accel, stroke 1.25, gravity, max fall 3.0, max rise */
    M(1125), M(30), M(1250), M(40), M(3000), M(2000),
    /* climb_speed, climb_fast                           (Estimated) */
    M(1000), M(1500),
    /* throw vx, vy, up vy                               (Estimated) */
    M(4000), M(1500), M(5000),
    /* coyote, buffer */
    0, 0,
    MM3_MOVE_SPIN_JUMP | MM3_MOVE_TWIRL | MM3_MOVE_WALL_JUMP | MM3_MOVE_GROUND_POUND |
        MM3_MOVE_TRIPLE_JUMP | MM3_MOVE_CROUCH_SLIDE | MM3_MOVE_CARRY | MM3_MOVE_SWIM |
        MM3_MOVE_CLIMB | MM3_MOVE_CROUCH_JUMP
};

static const mm3_modern_profile k_athletic = {
    /* ATHLETIC (source: 3D World, 2D plane). All Estimated. */
    M(1600), M(2600), M(3600), 60,
    M(80), M(80), M(150), M(250),
    M(60), M(100),
    M(3900),
    {M(1000), M(2000), M(3000)}, {0, M(150), M(300), M(450)},
    100, 100, 0, 0,
    {M(2500), M(1500), M(300), M(-120), M(-3000)},
    {M(220), M(220), M(220), M(150), M(300), M(300)},
    {M(450), M(450), M(450), M(350), M(450), M(450)},
    M(4500),
    0, 0, 0,
    0, 0, 0,
    M(2000), M(2400), M(4000), 12,
    18, M(6500), 10,
    /* long jump vx/vy, backflip vx/vy, sideflip vx/vy */
    M(4400), M(2600), M(800), M(5600), M(1200), M(5200),
    M(3300),
    M(40), M(120),
    M(1200), M(40), M(1400), M(40), M(2500), M(2200),
    M(1100), M(1600),
    M(4000), M(1500), M(5000),
    3, 4,
    MM3_MOVE_WALL_JUMP | MM3_MOVE_GROUND_POUND | MM3_MOVE_LONG_JUMP | MM3_MOVE_BACKFLIP |
        MM3_MOVE_SIDEFLIP | MM3_MOVE_CROUCH_SLIDE | MM3_MOVE_DASH | MM3_MOVE_CARRY |
        MM3_MOVE_SWIM | MM3_MOVE_CLIMB
};

static const mm3_modern_profile k_bloom = {
    /* BLOOM (source: Wonder). All Estimated. */
    M(1250), M(2400), 0, 0,
    M(70), M(70), M(120), M(200),
    M(50), M(80),
    M(3500),
    {M(700), M(1500), M(2200)}, {0, M(100), M(200), M(300)},
    100, 100, 0, 0,
    {M(2500), M(1500), M(300), M(-120), M(-3000)},
    {M(180), M(180), M(200), M(80), M(250), M(300)},
    {M(400), M(400), M(400), M(250), M(400), M(400)},
    M(3600),
    M(3300), M(180), M(1600),
    M(900), 30, M(1000),
    M(1600), M(1900), M(3500), 10,
    16, M(5500), 10,
    0, 0, 0, 0, 0, 0,
    M(3300),
    M(30), M(100),
    M(1100), M(30), M(1300), M(35), M(2500), M(2000),
    M(1000), M(1500),
    M(3800), M(1500), M(5000),
    6, 8,
    MM3_MOVE_SPIN_JUMP | MM3_MOVE_TWIRL | MM3_MOVE_WALL_JUMP | MM3_MOVE_GROUND_POUND |
        MM3_MOVE_CROUCH_SLIDE | MM3_MOVE_CARRY | MM3_MOVE_SWIM | MM3_MOVE_CLIMB |
        MM3_MOVE_CROUCH_JUMP
};

static const mm3_modern_profile k_custom = {
    /* CUSTOM: Modern's numbers with every move switched on. */
    M(1500), M(3000), M(3600), 60,
    M(60), M(60), M(100), M(120),
    M(40), M(60),
    M(3628),
    {M(700), M(1500), M(2800)}, {0, M(180), M(240), M(300)},
    100, 105, 10, M(1),
    {M(2500), M(1500), M(300), M(-120), M(-3000)},
    {M(60), M(250), M(340), M(80), M(310), M(340)},
    {M(340), M(340), M(340), M(250), M(340), M(340)},
    M(4000),
    M(3400), M(200), M(1800),
    M(1000), 24, M(1200),
    M(2000), M(2000), M(3600), 10,
    16, M(6000), 12,
    M(4400), M(2600), M(800), M(5600), M(1200), M(5200),
    M(3200),
    M(30), M(100),
    M(1125), M(30), M(1250), M(40), M(3000), M(2000),
    M(1000), M(1500),
    M(4000), M(1500), M(5000),
    4, 4,
    0xFFFFu
};

static const char *const k_names[MM3_STYLE_COUNT] = {
    "Retro", "Island", "Modern", "Athletic", "Bloom", "Custom"
};

mm3_engine mm3_style_engine(mm3_style style)
{
    if (style == MM3_STYLE_RETRO) return MM3_ENGINE_RETRO;
    if (style == MM3_STYLE_ISLAND) return MM3_ENGINE_ISLAND;
    return MM3_ENGINE_MODERN;
}

const mm3_modern_profile *mm3_builtin_profile(mm3_style style)
{
    switch (style) {
    case MM3_STYLE_ATHLETIC: return &k_athletic;
    case MM3_STYLE_BLOOM:    return &k_bloom;
    case MM3_STYLE_CUSTOM:   return &k_custom;
    default:                 return &k_modern;
    }
}

const char *mm3_style_name(mm3_style style)
{
    if ((int)style < 0 || style >= MM3_STYLE_COUNT) return "?";
    return k_names[style];
}
