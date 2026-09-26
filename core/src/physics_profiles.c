/*
 * physics_profiles.c - Modern-engine profiles.
 *
 * The four 2D maker styles (Retro, Arcade, Island, Modern) share one set of
 * numbers, like the level-maker game they follow, where every 2D style uses
 * the same HD-era physics and only the move set changes. Those numbers
 * start from the NSMB Wii decompilation (jump, gravity, max fall: Exact)
 * with speeds and accelerations tuned to the maker game's documented rules
 * (Estimated). Athletic and Bloom have their own (Estimated) numbers.
 *
 * The Classic styles don't use these tables; their exact constants live in
 * engine_retro.c and engine_island.c. Sources: docs/PHYSICS.md.
 *
 * MM3_MILLI(n) = n thousandths of a pixel per frame (or per frame^2).
 * Gravity values are down-positive; thresholds are up-positive speeds.
 */
#include "mm3/physics.h"

#define M(n) MM3_MILLI(n)

/* Shared numbers of the 2D maker styles; MOVES differs per style. */
#define MAKER_2D(MOVES) {                                                      \
    /* walk, run, dash, dash frames (Estimated) */                            \
    M(1500), M(2500), 0, 0,                                                    \
    /* accel, run accel, friction, turn (turn 0.12 Exact) */                  \
    M(60), M(60), M(100), M(120),                                              \
    /* air accel is lower than on the ground */                               \
    M(35), M(60),                                                              \
    /* jump 3.628 + speed bonus (Exact) */                                    \
    M(3628),                                                                   \
    {M(700), M(1500), M(2800)}, {0, M(180), M(240), M(300)},                   \
    /* double %, triple % (x1.05 Exact), chain window, chain min speed */     \
    100, 105, 10, M(1),                                                        \
    /* gravity bands (Exact) */                                               \
    {M(2500), M(1500), M(300), M(-120), M(-3000)},                             \
    {M(60), M(250), M(340), M(80), M(310), M(340)},                            \
    {M(340), M(340), M(340), M(250), M(340), M(340)},                          \
    M(4000),                                                                   \
    /* spin jump */                                                           \
    M(3400), M(200), M(1800),                                                  \
    /* twirl */                                                               \
    M(1000), 24, M(1200),                                                      \
    /* wall slide / wall jump */                                              \
    M(2000), M(2000), M(3600), 10,                                             \
    /* ground pound */                                                        \
    16, M(6000), 12,                                                           \
    /* long jump / backflip / sideflip (not in these styles) */               \
    0, 0, 0, 0, 0, 0,                                                          \
    /* crouch jump */                                                         \
    M(3200),                                                                   \
    /* slide friction, slope accel */                                         \
    M(30), M(100),                                                             \
    /* swim: 1.125 / stroke 1.25 / max fall 3.0 (Exact consts) */             \
    M(1125), M(30), M(1250), M(40), M(3000), M(2000),                          \
    /* climb */                                                               \
    M(1000), M(1500),                                                          \
    /* throw vx, vy, up */                                                    \
    M(4000), M(1500), M(5000),                                                 \
    /* coyote, buffer */                                                      \
    0, 0,                                                                      \
    /* sprint (P-speed) 3.0, P-meter fill 56 frames, slowfall gravity */      \
    M(3000), 56, M(250),                                                       \
    /* backflip charge, crawl, roll */                                        \
    0, 0, 0, 0,                                                                \
    MOVES }

#define MAKER_COMMON (MM3_MOVE_WALL_JUMP | MM3_MOVE_CROUCH_SLIDE | MM3_MOVE_SWIM | \
                      MM3_MOVE_CLIMB | MM3_MOVE_CROUCH_JUMP | MM3_MOVE_PMETER |      \
                      MM3_MOVE_KICK | MM3_RULE_AIR_LOCK | MM3_RULE_SLOWFALL)

/* 8-bit maker style: no carrying (kick only). */
static const mm3_modern_profile k_retro = MAKER_2D(MAKER_COMMON);
/* SMB3 maker style: carry and kick. */
static const mm3_modern_profile k_arcade = MAKER_2D(MAKER_COMMON | MM3_MOVE_CARRY);
/* SMW maker style: carry, toss up, spin jump. */
static const mm3_modern_profile k_island = MAKER_2D(MAKER_COMMON | MM3_MOVE_CARRY |
                                                   MM3_MOVE_THROW_UP | MM3_MOVE_SPIN_JUMP);
/* NSMBU maker style: spin jump, twirl, ground pound, triple jump, carry. */
static const mm3_modern_profile k_modern = MAKER_2D(MAKER_COMMON | MM3_MOVE_CARRY |
                                                   MM3_MOVE_SPIN_JUMP | MM3_MOVE_TWIRL |
                                                   MM3_MOVE_GROUND_POUND | MM3_MOVE_TRIPLE_JUMP);

static const mm3_modern_profile k_athletic = {
    /* ATHLETIC (maker 3D World style). All Estimated. */
    M(1600), M(2600), M(3600), 90,
    M(80), M(80), M(150), M(250),
    M(60), M(100),
    /* Jump: same heights as the maker styles, snappier arc (1.3x gravity,
       airtime 47 vs 54 frames). Calibrated by tools/calibrate_jump.c. */
    16171,
    {M(700), M(1500), M(2800)}, {0, 813, 1088, 1407},
    100, 100, 0, 0,
    {M(2500), M(1500), M(300), M(-120), M(-3000)},
    {319, 1331, 1810, 426, 1651, 1810},
    {1657, 1657, 1657, 1218, 1657, 1657},
    M(4500),
    0, 0, 0,
    0, 0, 0,
    M(2000), M(2400), M(4000), 12,
    18, M(6500), 10,
    /* long jump vx/vy, backflip vx/vy, sideflip vx/vy */
    M(4400), M(2600), M(800), M(5600), 0, 0,
    M(3300),
    M(40), M(120),
    M(1200), M(40), M(1400), M(40), M(2500), M(2200),
    M(1100), M(1600),
    M(4000), M(1500), M(5000),
    3, 4,
    0, 0, 0,
    /* backflip needs ~1 s of crouching; crawl; roll speed and length */
    60, M(700), M(3200), 20,
    MM3_MOVE_WALL_JUMP | MM3_MOVE_GROUND_POUND | MM3_MOVE_LONG_JUMP | MM3_MOVE_BACKFLIP |
        MM3_MOVE_CROUCH_SLIDE | MM3_MOVE_DASH | MM3_MOVE_CARRY | MM3_MOVE_SWIM |
        MM3_MOVE_CLIMB | MM3_MOVE_CRAWL | MM3_MOVE_ROLL | MM3_MOVE_KICK
};

static const mm3_modern_profile k_bloom = {
    /* BLOOM (source: Wonder). All Estimated. */
    M(1250), M(2400), 0, 0,
    M(70), M(70), M(120), M(200),
    M(50), M(80),
    /* Jump: same heights as the maker styles, floatier arc (0.65x gravity,
       airtime 64 vs 54 frames). Calibrated by tools/calibrate_jump.c. */
    13060,
    {M(700), M(1500), M(2800)}, {0, 520, 698, 891},
    100, 100, 0, 0,
    {M(2500), M(1500), M(300), M(-120), M(-3000)},
    {159, 665, 905, 213, 825, 905},
    {1086, 1086, 1086, 798, 1086, 1086},
    M(3600),
    12327, M(180), M(1600),
    M(900), 30, M(1000),
    M(1600), M(1900), M(3500), 10,
    16, M(5500), 10,
    0, 0, 0, 0, 0, 0,
    11825,   /* crouch jump, calibrated to the maker height */
    M(30), M(100),
    M(1100), M(30), M(1300), M(35), M(2500), M(2000),
    M(1000), M(1500),
    M(3800), M(1500), M(5000),
    6, 8,
    0, 0, 0,
    0, 0, 0, 0,
    MM3_MOVE_SPIN_JUMP | MM3_MOVE_TWIRL | MM3_MOVE_WALL_JUMP | MM3_MOVE_GROUND_POUND |
        MM3_MOVE_CROUCH_SLIDE | MM3_MOVE_CARRY | MM3_MOVE_SWIM | MM3_MOVE_CLIMB |
        MM3_MOVE_CROUCH_JUMP | MM3_MOVE_KICK
};

static const mm3_modern_profile k_custom = {
    /* CUSTOM: maker numbers with every move switched on. */
    M(1500), M(2500), M(3600), 90,
    M(60), M(60), M(100), M(120),
    M(35), M(60),
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
    M(3000), 56, M(250),
    30, M(700), M(3200), 20,
    /* everything except the air lock (Custom plays free-form) */
    0xFFFFFFFFu & ~(uint32_t)MM3_RULE_AIR_LOCK
};

static const char *const k_names[MM3_STYLE_COUNT] = {
    "Retro", "Arcade", "Island", "Modern", "Athletic", "Bloom",
    "Retro Classic", "Island Classic", "Custom"
};

mm3_engine mm3_style_engine(mm3_style style)
{
    if (style == MM3_STYLE_RETRO_CLASSIC) return MM3_ENGINE_RETRO;
    if (style == MM3_STYLE_ISLAND_CLASSIC) return MM3_ENGINE_ISLAND;
    return MM3_ENGINE_MODERN;
}

const mm3_modern_profile *mm3_builtin_profile(mm3_style style)
{
    switch (style) {
    case MM3_STYLE_RETRO:    return &k_retro;
    case MM3_STYLE_ARCADE:   return &k_arcade;
    case MM3_STYLE_ISLAND:   return &k_island;
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
