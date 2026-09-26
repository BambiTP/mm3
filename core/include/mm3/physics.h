/*
 * physics.h - Physics styles.
 *
 * Retro and Island each run a dedicated engine that reproduces the source
 * game's movement algorithm (see engine_retro.c / engine_island.c). Modern,
 * Athletic, Bloom and Custom share one data-driven engine (engine_modern.c)
 * whose behavior comes entirely from an mm3_modern_profile plus move flags.
 *
 * Units: speeds in 1/4096 px per frame, accelerations in 1/4096 px per
 * frame^2, 60 frames per second. Positive Y points down.
 * Source values, their origin and confidence are listed in docs/PHYSICS.md.
 */
#ifndef MM3_PHYSICS_H
#define MM3_PHYSICS_H

#include <stdint.h>
#include "fixed.h"

/*
 * The first five styles follow the level-maker versions of their source
 * games: the four 2D ones share one physics profile and differ only in
 * moves (like the source maker game); Athletic has its own. The two
 * "Classic" styles run the exact original-game engines.
 */
typedef enum {
    MM3_STYLE_RETRO          = 0, /* maker SMB1 style                         */
    MM3_STYLE_ARCADE         = 1, /* maker SMB3 style                         */
    MM3_STYLE_ISLAND         = 2, /* maker SMW style                          */
    MM3_STYLE_MODERN         = 3, /* maker NSMBU style                        */
    MM3_STYLE_ATHLETIC       = 4, /* maker 3D World style                     */
    MM3_STYLE_BLOOM          = 5, /* extra: Wonder-inspired                   */
    MM3_STYLE_RETRO_CLASSIC  = 6, /* exact original SMB1 engine               */
    MM3_STYLE_ISLAND_CLASSIC = 7, /* exact original SMW engine                */
    MM3_STYLE_CUSTOM         = 8, /* user-tunable, every move available       */
    MM3_STYLE_COUNT
} mm3_style;

typedef enum {
    MM3_ENGINE_RETRO = 0,
    MM3_ENGINE_ISLAND = 1,
    MM3_ENGINE_MODERN = 2
} mm3_engine;

/* Moves a style allows (modern engine; the retro/island engines have fixed
   move sets that match their source games). */
enum {
    MM3_MOVE_SPIN_JUMP    = 1u << 0,  /* SPIN on ground: spinning jump        */
    MM3_MOVE_TWIRL        = 1u << 1,  /* SPIN in air: once-per-jump hover     */
    MM3_MOVE_WALL_JUMP    = 1u << 2,  /* wall slide + wall kick               */
    MM3_MOVE_GROUND_POUND = 1u << 3,  /* DOWN in air                          */
    MM3_MOVE_TRIPLE_JUMP  = 1u << 4,  /* consecutive running jumps            */
    MM3_MOVE_LONG_JUMP    = 1u << 5,  /* run + DOWN + JUMP                    */
    MM3_MOVE_BACKFLIP     = 1u << 6,  /* crouch still + JUMP                  */
    MM3_MOVE_SIDEFLIP     = 1u << 7,  /* JUMP while skidding                  */
    MM3_MOVE_CROUCH_SLIDE = 1u << 8,  /* crouch while running / on slopes     */
    MM3_MOVE_DASH         = 1u << 9,  /* keep running to reach a faster dash  */
    MM3_MOVE_CARRY        = 1u << 10, /* pick up / throw crates with RUN      */
    MM3_MOVE_SWIM         = 1u << 11,
    MM3_MOVE_CLIMB        = 1u << 12,
    MM3_MOVE_CROUCH_JUMP  = 1u << 13,
    MM3_MOVE_PMETER       = 1u << 14, /* run on the ground to fill; full = sprint */
    MM3_MOVE_KICK         = 1u << 15, /* touching a resting crate kicks it      */
    MM3_MOVE_THROW_UP     = 1u << 16, /* hold UP when releasing RUN             */
    MM3_MOVE_CRAWL        = 1u << 17, /* move slowly while crouched             */
    MM3_MOVE_ROLL         = 1u << 18, /* SPIN while crouched: forward roll      */
    MM3_RULE_AIR_LOCK     = 1u << 19, /* can't gain speed in the air past takeoff */
    MM3_RULE_SLOWFALL     = 1u << 20  /* holding jump while falling falls slower */
};

#define MM3_GRAV_BANDS 6

typedef struct {
    /* Horizontal (ground) */
    int32_t walk_max, run_max, dash_max;
    int32_t dash_frames;        /* frames at run_max before dashing           */
    int32_t accel, run_accel;   /* toward target speed                         */
    int32_t friction;           /* no direction held                           */
    int32_t turn_decel;         /* pushing against current motion (skid)       */
    int32_t air_accel, air_turn;
    /* Jump */
    int32_t jump_vel;                       /* base launch speed (positive = up) */
    int32_t jump_bonus[4];                  /* added per tier: stand/walk/run/full */
    int32_t double_jump_pct, triple_jump_pct; /* % of launch speed               */
    int32_t chain_frames;                   /* window to chain the next jump     */
    int32_t chain_min_speed;                /* |vx| needed to chain              */
    /* Gravity bands: first band whose threshold is below vy (up-positive). */
    int32_t grav_threshold[MM3_GRAV_BANDS - 1];
    int32_t grav_held[MM3_GRAV_BANDS];      /* jump held                         */
    int32_t grav_released[MM3_GRAV_BANDS];  /* jump not held                     */
    int32_t max_fall;
    /* Moves */
    int32_t spin_jump_vel, spin_gravity, spin_max_fall;
    int32_t twirl_vel, twirl_frames, twirl_max_fall;
    int32_t wall_slide_max, wall_jump_vx, wall_jump_vy, wall_jump_lock;
    int32_t pound_windup, pound_speed, pound_land_lag;
    int32_t long_jump_vx, long_jump_vy;
    int32_t backflip_vx, backflip_vy;
    int32_t sideflip_vx, sideflip_vy;
    int32_t crouch_jump_vel;
    int32_t slide_friction, slope_accel;
    /* Water / climbing / carrying */
    int32_t swim_max_x, swim_accel, swim_stroke, swim_gravity, swim_max_fall, swim_max_rise;
    int32_t climb_speed, climb_fast;
    int32_t throw_vx, throw_vy, throw_up_vy;
    /* Forgiveness windows (frames) */
    int32_t coyote_frames, buffer_frames;
    /* Maker rules */
    int32_t sprint_max, pmeter_frames;   /* P-meter: frames at run speed to fill */
    int32_t slowfall_gravity;
    int32_t backflip_charge;             /* frames crouched before a backflip     */
    int32_t crawl_speed, roll_vx, roll_frames;
    uint32_t moves;
} mm3_modern_profile;

mm3_engine mm3_style_engine(mm3_style style);
/* Built-in modern-engine profile (also used as Custom's starting point). */
const mm3_modern_profile *mm3_builtin_profile(mm3_style style);
const char *mm3_style_name(mm3_style style);

#endif
