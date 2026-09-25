/*
 * world.h - The complete game state.
 *
 * mm3_game_state is plain old data: no pointers, fixed-size arrays, no heap.
 * That makes save-states a memcpy, rollback netcode possible, and keeps the
 * core portable to devices without malloc.
 */
#ifndef MM3_WORLD_H
#define MM3_WORLD_H

#include <stdint.h>
#include "fixed.h"
#include "rng.h"
#include "input.h"
#include "physics.h"

#define MM3_TILE_PX   16
#define MM3_TILE_FX   MM3_PX(MM3_TILE_PX)
#define MM3_MAP_MAX_W 256
#define MM3_MAP_MAX_H 32
#define MM3_MAX_OBJS  16

typedef enum {
    MM3_TILE_EMPTY = 0,
    MM3_TILE_SOLID,
    MM3_TILE_SEMISOLID,     /* one-way platform: land on it from above       */
    MM3_TILE_SLOPE_UP,      /* 45 degrees, rises to the right                */
    MM3_TILE_SLOPE_DOWN,    /* 45 degrees, falls to the right                */
    MM3_TILE_SLOPE_UP_LO,   /* 22.5 degrees rising, lower half (0 -> 8 px)   */
    MM3_TILE_SLOPE_UP_HI,   /* 22.5 degrees rising, upper half (8 -> 16 px)  */
    MM3_TILE_SLOPE_DOWN_HI, /* 22.5 degrees falling, upper half (16 -> 8 px) */
    MM3_TILE_SLOPE_DOWN_LO, /* 22.5 degrees falling, lower half (8 -> 0 px)  */
    MM3_TILE_WATER,
    MM3_TILE_VINE,
    MM3_TILE_COUNT
} mm3_tile;

typedef struct {
    int32_t width;   /* in tiles */
    int32_t height;  /* in tiles */
    uint8_t tiles[MM3_MAP_MAX_H][MM3_MAP_MAX_W];
} mm3_tilemap;

/* ---- objects (crates) ---- */

typedef enum {
    MM3_OBJ_NONE = 0,
    MM3_OBJ_CRATE
} mm3_obj_type;

typedef enum {
    MM3_OBJS_REST = 0,
    MM3_OBJS_CARRIED,
    MM3_OBJS_THROWN
} mm3_obj_state;

/* Every field is int32_t so the struct has no padding and hashes cleanly. */
typedef struct {
    int32_t type, state;
    int32_t x, y, vx, vy, w, h;
    int32_t on_ground;
} mm3_object;

/* ---- player ---- */

typedef enum {
    MM3_MODE_GROUND = 0,
    MM3_MODE_AIR,
    MM3_MODE_SWIM,
    MM3_MODE_CLIMB
} mm3_player_mode;

/* What the player is doing, for rules and for the renderer. */
typedef enum {
    MM3_ACT_NONE = 0,
    MM3_ACT_JUMP,
    MM3_ACT_DOUBLE_JUMP,
    MM3_ACT_TRIPLE_JUMP,
    MM3_ACT_SPIN_JUMP,
    MM3_ACT_TWIRL,
    MM3_ACT_LONG_JUMP,
    MM3_ACT_BACKFLIP,
    MM3_ACT_SIDEFLIP,
    MM3_ACT_WALL_SLIDE,
    MM3_ACT_WALL_JUMP,
    MM3_ACT_POUND_WINDUP,
    MM3_ACT_POUND_FALL,
    MM3_ACT_POUND_LAND,
    MM3_ACT_CROUCH_SLIDE,
    MM3_ACT_THROW
} mm3_action;

/* Pose for the renderer (derived deterministically every tick). */
typedef enum {
    MM3_POSE_IDLE = 0,
    MM3_POSE_WALK,
    MM3_POSE_RUN,
    MM3_POSE_SPRINT,
    MM3_POSE_SKID,
    MM3_POSE_CROUCH,
    MM3_POSE_LOOK_UP,
    MM3_POSE_JUMP,
    MM3_POSE_FALL,
    MM3_POSE_SPIN,
    MM3_POSE_TWIRL,
    MM3_POSE_TRIPLE,
    MM3_POSE_LONG_JUMP,
    MM3_POSE_BACKFLIP,
    MM3_POSE_SIDEFLIP,
    MM3_POSE_WALL_SLIDE,
    MM3_POSE_POUND,
    MM3_POSE_SLIDE,
    MM3_POSE_SWIM,
    MM3_POSE_CLIMB,
    MM3_POSE_THROW,
    MM3_POSE_COUNT
} mm3_pose;

enum {
    MM3_PF_FACING_LEFT = 1u << 0,
    MM3_PF_CROUCH      = 1u << 1,
    MM3_PF_HELD_JUMP   = 1u << 2,  /* jump button still held since takeoff  */
    MM3_PF_TWIRLED     = 1u << 3,  /* twirl used this airtime                */
    MM3_PF_IN_WATER    = 1u << 4,
    MM3_PF_ON_VINE     = 1u << 5,  /* overlapping a climbable tile           */
    MM3_PF_SKIDDING    = 1u << 6,
    MM3_PF_WALL_L      = 1u << 7,  /* touching a wall this frame             */
    MM3_PF_WALL_R      = 1u << 8,
    MM3_PF_HEAD_BUMP   = 1u << 9,
    MM3_PF_ON_SLOPE    = 1u << 10,
    MM3_PF_LOOK_UP     = 1u << 11
};

/*
 * Every field is int32_t so the struct has no padding and the state hash can
 * walk it as an array. Engine-private fields are documented per engine.
 */
typedef struct {
    int32_t x, y;          /* top-left corner, 1/4096 px                        */
    int32_t vx, vy;        /* 1/4096 px per frame                              */
    int32_t w, h;          /* current hitbox (shrinks when crouching)          */
    int32_t spawn_x, spawn_y;
    int32_t mode;          /* mm3_player_mode                                  */
    int32_t action;        /* mm3_action                                       */
    int32_t flags;         /* MM3_PF_*                                          */
    int32_t pose, anim;    /* renderer only reads these                        */
    int32_t prev_buttons;
    int32_t carry;         /* object index being carried, or -1                */
    int32_t slope;         /* -2..2: ground steepness under the feet (+ = rising right) */
    /* shared timers */
    int32_t coyote, jump_buffer, action_timer, lock_timer;
    int32_t chain_count, chain_timer;   /* consecutive jumps (modern)          */
    int32_t run_timer;                  /* dash charge (modern) / run timer (retro) */
    int32_t pmeter;                     /* island P-meter 0..112               */
    int32_t throw_timer;                /* frames left in the throw pose        */
    /* retro engine: exact 8-bit style state */
    int32_t r_xspeed, r_xmf;            /* signed px/16 byte, fraction         */
    int32_t r_yspeed, r_ymf, r_ydummy;  /* signed px byte, force, fraction     */
    int32_t r_vforce, r_vforce_down;
    int32_t r_jump_origin, r_swim_timer, r_climb_timer;
    int32_t r_running_speed, r_moving_dir;
    /* island engine: exact 16-bit style state */
    int32_t i_in_air;                   /* 0, 0x0B jump, 0x0C p-jump, 0x24 falling */
    int32_t i_spin;
} mm3_player;

typedef struct {
    uint32_t frame;
    mm3_rng rng;
    int32_t style;                 /* mm3_style                               */
    mm3_modern_profile profile;    /* by value: tuning never breaks old levels */
    mm3_tilemap map;
    mm3_player player;
    mm3_object objs[MM3_MAX_OBJS];
} mm3_game_state;

#endif
