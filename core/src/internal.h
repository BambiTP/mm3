/*
 * internal.h - Shared declarations between core source files. Not public API.
 */
#ifndef MM3_INTERNAL_H
#define MM3_INTERNAL_H

#include "mm3/sim.h"

#define MM3_STAND_W MM3_PX(12)
#define MM3_STAND_H MM3_PX(26)
#define MM3_CROUCH_H MM3_PX(14)

/* ---- collide.c ---- */

typedef struct {
    int32_t ground;   /* standing on something after the move               */
    int32_t head;     /* hit a ceiling while moving up                       */
    int32_t wall_l;   /* blocked (or touching) on the left                   */
    int32_t wall_r;   /* blocked (or touching) on the right                  */
    int32_t blocked_l;/* the move was actually stopped by a wall on the left */
    int32_t blocked_r;/* ... on the right                                    */
    int32_t slope;    /* steepness under the feet: -2..2 (+ = rising right)  */
} mm3_hit;

int32_t mm3_tile_at(const mm3_tilemap *m, int32_t tx, int32_t ty);
int32_t mm3_is_slope(int32_t tile);
/* Any SOLID tile overlapping the box? (slopes/semisolids are not walls) */
int32_t mm3_box_solid(const mm3_tilemap *m, mm3_fx x, mm3_fx y, mm3_fx w, mm3_fx h);
/* Tile under a point given in fixed-point world coordinates. */
int32_t mm3_tile_at_point(const mm3_tilemap *m, mm3_fx x, mm3_fx y);
/* Move an axis-aligned box by (dx, dy) with tile collision. X first, then Y.
   grounded_before enables step-up on small ledges and sticking to slopes. */
void mm3_move_box(const mm3_tilemap *m, mm3_fx *x, mm3_fx *y, mm3_fx w, mm3_fx h,
                  mm3_fx dx, mm3_fx dy, int32_t grounded_before, mm3_hit *out);

/* ---- player_common.c ---- */

void mm3_player_set_crouch(mm3_player *pl, const mm3_tilemap *m, int32_t crouch);
void mm3_player_sense(mm3_game_state *s);          /* water / vine flags      */
void mm3_player_apply_hit(mm3_player *pl, const mm3_hit *hit);
int32_t mm3_player_try_pickup(mm3_game_state *s);  /* returns 1 if picked up  */
void mm3_player_throw(mm3_game_state *s, mm3_buttons b, mm3_fx vx, mm3_fx vy, mm3_fx up_vy);
void mm3_player_update_carry(mm3_game_state *s);
void mm3_player_try_kick(mm3_game_state *s, mm3_fx vx);
void mm3_player_check_pit(mm3_game_state *s);
void mm3_player_reset(mm3_player *pl);
void mm3_player_update_pose(mm3_game_state *s, mm3_buttons b);

/* ---- engines ---- */

void mm3_engine_retro_tick(mm3_game_state *s, mm3_buttons b);
void mm3_engine_island_tick(mm3_game_state *s, mm3_buttons b);
void mm3_engine_modern_tick(mm3_game_state *s, mm3_buttons b);

/* ---- objects.c ---- */

void mm3_objects_tick(mm3_game_state *s);
int32_t mm3_object_spawn(mm3_game_state *s, int32_t type, mm3_fx x, mm3_fx y);

#endif
