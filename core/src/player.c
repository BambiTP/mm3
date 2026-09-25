/*
 * player.c - Player movement driven entirely by the active physics profile.
 *
 * Order of operations per tick (keep stable! changing it changes replays):
 *   1. read input edges
 *   2. horizontal acceleration
 *   3. jumps / wall jumps / ground pound
 *   4. gravity
 *   5. move X, collide; move Y, collide
 *   6. timers, pit respawn
 */
#include "mm3/sim.h"

static int tile_solid(const mm3_tilemap *m, int32_t tx, int32_t ty)
{
    if (tx < 0 || tx >= m->width) return 1;       /* side walls */
    if (ty < 0 || ty >= m->height) return 0;      /* open sky / bottomless pit */
    return m->tiles[ty][tx] == MM3_TILE_SOLID;
}

/* Does the box [x, x+w) x [y, y+h) (subpixels) overlap any solid tile? */
static int region_solid(const mm3_tilemap *m, mm3_fx x, mm3_fx y, mm3_fx w, mm3_fx h)
{
    int32_t tx0 = mm3_floor_div(x, MM3_TILE_FX);
    int32_t tx1 = mm3_floor_div(x + w - 1, MM3_TILE_FX);
    int32_t ty0 = mm3_floor_div(y, MM3_TILE_FX);
    int32_t ty1 = mm3_floor_div(y + h - 1, MM3_TILE_FX);
    int32_t tx, ty;
    for (ty = ty0; ty <= ty1; ty++)
        for (tx = tx0; tx <= tx1; tx++)
            if (tile_solid(m, tx, ty)) return 1;
    return 0;
}

static int32_t jump_tier(const mm3_physics_profile *p, int32_t speed)
{
    int32_t i;
    for (i = 0; i < MM3_JUMP_TIERS - 1; i++)
        if (speed < p->jump_tier_speed[i]) return i;
    return MM3_JUMP_TIERS - 1;
}

static void respawn(mm3_player *pl)
{
    pl->x = pl->spawn_x;
    pl->y = pl->spawn_y;
    pl->vx = pl->vy = 0;
    pl->coyote = pl->jump_buffer = 0;
    pl->wall_dir = 0;
    pl->flags &= MM3_PF_FACING_LEFT;
}

void mm3_player_tick(mm3_game_state *s, mm3_buttons buttons)
{
    const mm3_physics_profile *p = &s->profile;
    mm3_player *pl = &s->player;
    const mm3_tilemap *m = &s->map;
    mm3_buttons pressed = (mm3_buttons)(buttons & ~pl->prev_buttons);
    int32_t dir = ((buttons & MM3_BTN_RIGHT) ? 1 : 0) - ((buttons & MM3_BTN_LEFT) ? 1 : 0);
    int run = (buttons & MM3_BTN_RUN) != 0;
    int on_ground = (pl->flags & MM3_PF_ON_GROUND) != 0;
    int pounding = (pl->flags & MM3_PF_GROUND_POUND) != 0;
    int32_t facing = (pl->flags & MM3_PF_FACING_LEFT) ? -1 : 1;
    int32_t max_fall;

    /* ---- 1. input edges ---- */
    if (pressed & MM3_BTN_JUMP) pl->jump_buffer = p->jump_buffer_frames + 1;

    /* ---- 2. horizontal ---- */
    if (pounding) {
        pl->vx = 0;
    } else if (on_ground) {
        int32_t max = run ? p->run_max : p->walk_max;
        int32_t accel = run ? p->run_accel : p->walk_accel;
        if (dir == 0) {
            pl->vx = mm3_approach(pl->vx, 0, p->friction);
        } else if (pl->vx != 0 && mm3_sign(pl->vx) == -dir) {
            pl->vx = mm3_approach(pl->vx, 0, p->skid_decel);   /* skidding */
        } else if (mm3_abs(pl->vx) > max) {
            pl->vx = mm3_approach(pl->vx, dir * max, p->friction); /* let go of RUN */
        } else {
            pl->vx = mm3_approach(pl->vx, dir * max, accel);
        }
        if (dir != 0) facing = dir;
    } else if (dir != 0) {
        int32_t max;
        if (p->abilities & MM3_ABIL_AIR_TURN) {
            max = run ? p->run_max : p->walk_max;
            facing = dir;
        } else {
            max = pl->air_max; /* locked to takeoff speed tier */
        }
        /* Never brake an above-cap speed just because the cap is lower. */
        if (!(mm3_sign(pl->vx) == dir && mm3_abs(pl->vx) >= max))
            pl->vx = mm3_approach(pl->vx, dir * max, p->air_accel);
    }

    /* ---- 3. jumps ---- */
    {
        int can_jump = on_ground || pl->coyote > 0;
        int spin = (pressed & MM3_BTN_SPIN) && (p->abilities & MM3_ABIL_SPIN_JUMP);

        if (can_jump && !pounding && (pl->jump_buffer > 0 || spin)) {
            int32_t speed = mm3_abs(pl->vx);
            if (spin) {
                pl->vy = -p->spin_jump_vel;
                pl->flags |= MM3_PF_SPINNING;
            } else if ((p->abilities & MM3_ABIL_LONG_JUMP) && (buttons & MM3_BTN_DOWN) &&
                       run && speed >= p->walk_max) {
                pl->vx = facing * p->long_jump_vx;
                pl->vy = -p->long_jump_vy;
            } else {
                pl->vy = -p->jump_vel[jump_tier(p, speed)];
            }
            pl->air_max = mm3_max(mm3_abs(pl->vx) > p->walk_max ? p->run_max : p->walk_max,
                                  mm3_abs(pl->vx));
            pl->flags |= MM3_PF_JUMP_RISING;
            pl->flags &= ~(uint32_t)MM3_PF_ON_GROUND;
            pl->coyote = 0;
            pl->jump_buffer = 0;
            on_ground = 0;
        } else if (!on_ground && pl->wall_dir != 0 && pl->jump_buffer > 0 &&
                   (p->abilities & MM3_ABIL_WALL_JUMP) && !pounding) {
            pl->vx = -pl->wall_dir * p->wall_jump_vx;
            pl->vy = -p->wall_jump_vy;
            facing = -pl->wall_dir;
            pl->air_max = p->run_max;
            pl->flags |= MM3_PF_JUMP_RISING;
            pl->flags &= ~(uint32_t)MM3_PF_SPINNING;
            pl->jump_buffer = 0;
        } else if (!on_ground && !pounding && (pressed & MM3_BTN_DOWN) &&
                   (p->abilities & MM3_ABIL_GROUND_POUND)) {
            pl->flags |= MM3_PF_GROUND_POUND;
            pl->flags &= ~(uint32_t)(MM3_PF_JUMP_RISING | MM3_PF_SPINNING);
            pl->vx = 0;
            pounding = 1;
        }
    }

    /* ---- 4. gravity ---- */
    if (pounding) {
        pl->vy = p->ground_pound_vy;
    } else {
        int32_t g;
        int holding = (buttons & MM3_BTN_JUMP) ||
                      ((pl->flags & MM3_PF_SPINNING) && (buttons & MM3_BTN_SPIN));
        if (pl->vy < 0)
            g = ((pl->flags & MM3_PF_JUMP_RISING) && holding) ? p->gravity_hold
                                                              : p->gravity_release;
        else
            g = p->gravity_fall;
        pl->vy += g;
        if (pl->vy >= 0) pl->flags &= ~(uint32_t)MM3_PF_JUMP_RISING;

        max_fall = p->max_fall;
        if (pl->flags & MM3_PF_SPINNING) max_fall = mm3_min(max_fall, p->spin_fall_max);
        if ((p->abilities & MM3_ABIL_WALL_JUMP) && pl->wall_dir != 0 && dir == pl->wall_dir)
            max_fall = mm3_min(max_fall, p->wall_slide_max);
        if (pl->vy > max_fall) pl->vy = max_fall;
    }

    /* ---- 5. move + collide (speeds clamped below one tile: no tunneling) ---- */
    pl->vx = mm3_clamp(pl->vx, -(MM3_TILE_FX - 1), MM3_TILE_FX - 1);
    pl->vy = mm3_clamp(pl->vy, -(MM3_TILE_FX - 1), MM3_TILE_FX - 1);

    pl->x += pl->vx;
    if (region_solid(m, pl->x, pl->y, pl->w, pl->h)) {
        if (pl->vx > 0)
            pl->x = mm3_floor_div(pl->x + pl->w - 1, MM3_TILE_FX) * MM3_TILE_FX - pl->w;
        else
            pl->x = (mm3_floor_div(pl->x, MM3_TILE_FX) + 1) * MM3_TILE_FX;
        pl->vx = 0;
    }

    pl->flags &= ~(uint32_t)MM3_PF_ON_GROUND;
    pl->y += pl->vy;
    if (region_solid(m, pl->x, pl->y, pl->w, pl->h)) {
        if (pl->vy > 0) {
            pl->y = mm3_floor_div(pl->y + pl->h - 1, MM3_TILE_FX) * MM3_TILE_FX - pl->h;
            pl->flags |= MM3_PF_ON_GROUND;
            pl->flags &= ~(uint32_t)(MM3_PF_GROUND_POUND | MM3_PF_SPINNING | MM3_PF_JUMP_RISING);
        } else {
            pl->y = (mm3_floor_div(pl->y, MM3_TILE_FX) + 1) * MM3_TILE_FX;
            pl->flags &= ~(uint32_t)MM3_PF_JUMP_RISING;
        }
        pl->vy = 0;
    }

    if (region_solid(m, pl->x - 1, pl->y, 1, pl->h))
        pl->wall_dir = -1;
    else if (region_solid(m, pl->x + pl->w, pl->y, 1, pl->h))
        pl->wall_dir = 1;
    else
        pl->wall_dir = 0;

    /* ---- 6. timers, facing, pit ---- */
    if (pl->flags & MM3_PF_ON_GROUND) {
        pl->coyote = p->coyote_frames;
        /* Locked-air styles keep the speed tier you had when leaving the ground. */
        pl->air_max = mm3_abs(pl->vx) > p->walk_max ? p->run_max : p->walk_max;
    } else if (pl->coyote > 0)
        pl->coyote--;
    if (pl->jump_buffer > 0) pl->jump_buffer--;

    if (facing < 0) pl->flags |= MM3_PF_FACING_LEFT;
    else pl->flags &= ~(uint32_t)MM3_PF_FACING_LEFT;

    if (pl->y > (m->height + 4) * MM3_TILE_FX) respawn(pl);

    pl->prev_buttons = buttons;
}
