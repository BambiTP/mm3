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
#define MM3_MAP_MAX_H 64

typedef enum {
    MM3_TILE_EMPTY = 0,
    MM3_TILE_SOLID = 1
} mm3_tile;

typedef struct {
    int32_t width;   /* in tiles */
    int32_t height;  /* in tiles */
    uint8_t tiles[MM3_MAP_MAX_H][MM3_MAP_MAX_W];
} mm3_tilemap;

enum {
    MM3_PF_ON_GROUND    = 1u << 0,
    MM3_PF_JUMP_RISING  = 1u << 1, /* in a jump that can still be cut short */
    MM3_PF_GROUND_POUND = 1u << 2,
    MM3_PF_SPINNING     = 1u << 3,
    MM3_PF_FACING_LEFT  = 1u << 4
};

typedef struct {
    mm3_fx x, y;          /* top-left corner, subpixels */
    mm3_fx vx, vy;
    mm3_fx w, h;
    mm3_fx spawn_x, spawn_y;
    mm3_fx air_max;       /* horizontal cap while airborne (locked-air styles) */
    int32_t coyote;       /* frames left to still jump after leaving ground */
    int32_t jump_buffer;  /* frames left on a buffered jump press */
    int32_t wall_dir;     /* -1/+1 if touching a wall this frame, else 0 */
    uint32_t flags;
    mm3_buttons prev_buttons;
} mm3_player;

typedef struct {
    uint32_t frame;
    mm3_rng rng;
    mm3_style style;
    mm3_physics_profile profile; /* stored by value: tuning never breaks old levels */
    mm3_tilemap map;
    mm3_player player;
} mm3_game_state;

#endif
