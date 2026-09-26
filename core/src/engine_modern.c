/*
 * engine_modern.c - Data-driven engine for the Modern, Athletic, Bloom and
 * Custom styles. Every number comes from s->profile; every move is gated by
 * a MM3_MOVE_* flag, so any combination can be built in Custom.
 *
 * Gravity follows the HD-era scheme (source: NSMB Wii decompilation): the
 * acceleration depends on the current vertical speed band and on whether
 * the jump button is held, which gives the characteristic slow apex.
 */
#include "internal.h"

static int32_t has(const mm3_modern_profile *p, uint32_t move) { return (p->moves & move) != 0; }

static int32_t facing_dir(const mm3_player *pl) { return (pl->flags & MM3_PF_FACING_LEFT) ? -1 : 1; }

static void set_facing(mm3_player *pl, int32_t dir)
{
    if (dir < 0) pl->flags |= MM3_PF_FACING_LEFT;
    else if (dir > 0) pl->flags &= ~(int32_t)MM3_PF_FACING_LEFT;
}

static int32_t band_gravity(const mm3_modern_profile *p, int32_t vy, int32_t held)
{
    int32_t up = -vy, i;
    const int32_t *tbl = held ? p->grav_held : p->grav_released;
    for (i = 0; i < MM3_GRAV_BANDS - 1; i++)
        if (up > p->grav_threshold[i]) return tbl[i];
    return tbl[MM3_GRAV_BANDS - 1];
}

/*
 * Jump tier from what the player is doing, not raw speed, so every style
 * gives the same height for a standing, walking, running and full-speed
 * jump even though their walk/run speeds differ:
 *   0 standing, 1 walking, 2 running, 3 full speed (P-meter full or dash).
 */
static int32_t jump_tier(const mm3_modern_profile *p, const mm3_player *pl)
{
    int32_t a = mm3_abs(pl->vx);
    int32_t pfull = (p->moves & MM3_MOVE_PMETER) && p->pmeter_frames > 0 &&
                    pl->pmeter >= p->pmeter_frames;
    int32_t dash = (p->moves & MM3_MOVE_DASH) && p->dash_frames > 0 &&
                   pl->run_timer >= p->dash_frames;
    if ((pfull || dash) && a > p->run_max) return 3;
    if (a < p->walk_max / 2) return 0;
    if (a <= p->walk_max + MM3_MILLI(100)) return 1;
    return 2;
}

static int32_t jump_speed(const mm3_modern_profile *p, const mm3_player *pl)
{
    return p->jump_vel + p->jump_bonus[jump_tier(p, pl)];
}

static void start_air(mm3_player *pl, int32_t action, int32_t vy)
{
    pl->mode = MM3_MODE_AIR;
    pl->action = action;
    pl->vy = -vy;
    pl->coyote = 0;
    pl->jump_buffer = 0;
    pl->flags |= MM3_PF_HELD_JUMP;
}

/* All jumps that start from the ground (or coyote time). Returns 1 if jumped. */
static int32_t ground_jump(mm3_game_state *s, mm3_buttons b, int32_t dir, int32_t spin)
{
    mm3_player *pl = &s->player;
    const mm3_modern_profile *p = &s->profile;
    int32_t crouch = (pl->flags & MM3_PF_CROUCH) != 0;
    int32_t speed = mm3_abs(pl->vx);
    int32_t face = facing_dir(pl);

    if (mm3_box_solid(&s->map, pl->x, pl->y - MM3_PX(1), pl->w, pl->h)) return 0;

    /* long jump on the action button while running (styles without a spin jump) */
    if (spin && has(p, MM3_MOVE_LONG_JUMP) && !has(p, MM3_MOVE_SPIN_JUMP) && pl->carry < 0 &&
        speed >= p->run_max * 3 / 4) {
        mm3_player_set_crouch(pl, &s->map, 0);
        pl->vx = mm3_sign(pl->vx) * p->long_jump_vx;
        set_facing(pl, mm3_sign(pl->vx));
        start_air(pl, MM3_ACT_LONG_JUMP, p->long_jump_vy);
        return 1;
    }
    if (spin && !has(p, MM3_MOVE_SPIN_JUMP)) return 0;
    if (spin && has(p, MM3_MOVE_SPIN_JUMP) && pl->carry < 0) {
        start_air(pl, MM3_ACT_SPIN_JUMP, p->spin_jump_vel);
        return 1;
    }
    if (pl->carry < 0 && crouch) {
        if (has(p, MM3_MOVE_LONG_JUMP) && speed >= p->run_max * 3 / 4) {
            mm3_player_set_crouch(pl, &s->map, 0);
            pl->vx = mm3_sign(pl->vx) * p->long_jump_vx;
            set_facing(pl, mm3_sign(pl->vx));
            start_air(pl, MM3_ACT_LONG_JUMP, p->long_jump_vy);
            return 1;
        }
        if (has(p, MM3_MOVE_BACKFLIP) && speed < p->walk_max / 2 &&
            pl->crouch_timer >= p->backflip_charge) {
            mm3_player_set_crouch(pl, &s->map, 0);
            pl->vx = -face * p->backflip_vx;
            start_air(pl, MM3_ACT_BACKFLIP, p->backflip_vy);
            return 1;
        }
        if (has(p, MM3_MOVE_CROUCH_JUMP)) {
            start_air(pl, MM3_ACT_JUMP, p->crouch_jump_vel);
            return 1;
        }
    }
    if (has(p, MM3_MOVE_SIDEFLIP) && pl->carry < 0 && dir != 0 && pl->vx != 0 &&
        mm3_sign(pl->vx) == -dir && speed > p->walk_max / 2) {
        mm3_player_set_crouch(pl, &s->map, 0);
        pl->vx = dir * p->sideflip_vx;
        set_facing(pl, dir);
        start_air(pl, MM3_ACT_SIDEFLIP, p->sideflip_vy);
        return 1;
    }
    {
        int32_t v = jump_speed(p, pl);
        int32_t act = MM3_ACT_JUMP;
        mm3_player_set_crouch(pl, &s->map, 0);
        if (has(p, MM3_MOVE_TRIPLE_JUMP) && pl->carry < 0) {
            if (pl->chain_timer > 0 && speed >= p->chain_min_speed) pl->chain_count++;
            else pl->chain_count = 0;
            if (pl->chain_count > 2) pl->chain_count = 0;
            if (pl->chain_count == 1) { v = v * p->double_jump_pct / 100; act = MM3_ACT_DOUBLE_JUMP; }
            if (pl->chain_count == 2) { v = v * p->triple_jump_pct / 100; act = MM3_ACT_TRIPLE_JUMP; }
        }
        start_air(pl, act, v);
    }
    (void)b;
    return 1;
}

static void climb(mm3_game_state *s, mm3_buttons b, mm3_buttons pressed)
{
    mm3_player *pl = &s->player;
    const mm3_modern_profile *p = &s->profile;
    int32_t sp = (b & MM3_BTN_RUN) ? p->climb_fast : p->climb_speed;
    mm3_fx oy = pl->y;
    mm3_fx dx = 0, dy = 0;
    mm3_hit hit;
    if ((pressed & MM3_BTN_SPIN) && has(p, MM3_MOVE_SPIN_JUMP)) {
        start_air(pl, MM3_ACT_SPIN_JUMP, p->spin_jump_vel);   /* spin off the vine */
        return;
    }
    if (pressed & MM3_BTN_JUMP) {
        start_air(pl, MM3_ACT_JUMP, p->jump_vel * 4 / 5);
        return;
    }
    if (b & MM3_BTN_LEFT) dx = -sp;
    if (b & MM3_BTN_RIGHT) dx = sp;
    if (b & MM3_BTN_UP) dy = -sp;
    if (b & MM3_BTN_DOWN) dy = sp;
    set_facing(pl, mm3_sign(dx));
    pl->vx = pl->vy = 0;
    mm3_move_box(&s->map, &pl->x, &pl->y, pl->w, pl->h, dx, dy, 0, &hit);
    mm3_player_apply_hit(pl, &hit);
    mm3_player_sense(s);
    if (!(pl->flags & MM3_PF_ON_VINE)) {
        if (dy < 0) pl->y = oy;
        else { pl->mode = MM3_MODE_AIR; pl->action = MM3_ACT_NONE; }
    }
    if (hit.ground && dy > 0) { pl->mode = MM3_MODE_GROUND; pl->action = MM3_ACT_NONE; }
}

void mm3_engine_modern_tick(mm3_game_state *s, mm3_buttons b)
{
    mm3_player *pl = &s->player;
    const mm3_modern_profile *p = &s->profile;
    const mm3_tilemap *m = &s->map;
    mm3_buttons pressed = (mm3_buttons)(b & ~pl->prev_buttons);
    mm3_buttons released = (mm3_buttons)(pl->prev_buttons & ~b);
    int32_t dir = ((b & MM3_BTN_RIGHT) ? 1 : 0) - ((b & MM3_BTN_LEFT) ? 1 : 0);
    int32_t run = (b & MM3_BTN_RUN) != 0;
    int32_t held = (b & (MM3_BTN_JUMP | MM3_BTN_SPIN)) != 0;
    int32_t on_ground, in_water, at_surface;
    int32_t max_fall;
    mm3_hit hit;

    if (!held) pl->flags &= ~(int32_t)MM3_PF_HELD_JUMP;
    if (pl->lock_timer > 0) { pl->lock_timer--; dir = 0; }
    if (pressed & MM3_BTN_JUMP) pl->jump_buffer = p->buffer_frames + 1;

    mm3_player_sense(s);
    in_water = has(p, MM3_MOVE_SWIM) && (pl->flags & MM3_PF_IN_WATER);
    at_surface = in_water &&
        mm3_tile_at_point(m, pl->x + pl->w / 2, pl->y - MM3_PX(2)) != MM3_TILE_WATER;

    /* ---- carrying ---- */
    if (has(p, MM3_MOVE_CARRY)) {
        if (pl->carry < 0 && (pressed & MM3_BTN_RUN) && pl->mode != MM3_MODE_CLIMB &&
            !(pl->flags & MM3_PF_CROUCH))
            mm3_player_try_pickup(s);
        else if (pl->carry >= 0 && (released & MM3_BTN_RUN))
            mm3_player_throw(s, b, p->throw_vx, p->throw_vy,
                             has(p, MM3_MOVE_THROW_UP) ? p->throw_up_vy : 0);
    }
    if (has(p, MM3_MOVE_KICK) && pl->carry < 0 && !(has(p, MM3_MOVE_CARRY) && run))
        mm3_player_try_kick(s, p->throw_vx);

    /* ---- climbing ---- */
    if (has(p, MM3_MOVE_CLIMB) && pl->mode != MM3_MODE_CLIMB && (pl->flags & MM3_PF_ON_VINE) &&
        (b & MM3_BTN_UP) && pl->carry < 0 && pl->action != MM3_ACT_POUND_FALL &&
        (pl->mode == MM3_MODE_GROUND || pl->vy >= 0)) {
        pl->mode = MM3_MODE_CLIMB;
        pl->action = MM3_ACT_NONE;
        mm3_player_set_crouch(pl, m, 0);
    }
    if (pl->mode == MM3_MODE_CLIMB) {
        climb(s, b, pressed);
        if (pl->mode == MM3_MODE_CLIMB) return;
    }

    on_ground = pl->mode == MM3_MODE_GROUND;

    /* ---- timers ---- */
    if (on_ground) {
        /* air-lock rule: in the air you can't go faster than at takeoff */
        pl->air_cap = mm3_max(mm3_abs(pl->vx), p->walk_max);
        pl->coyote = p->coyote_frames;
        if (pl->chain_timer > 0 && --pl->chain_timer == 0) pl->chain_count = 0;
    } else if (pl->coyote > 0) {
        pl->coyote--;
    }
    if (pl->action == MM3_ACT_POUND_LAND) {
        if (--pl->action_timer <= 0) pl->action = MM3_ACT_NONE;
        dir = 0;
    }

    /* ---- P-meter: fills while running at full speed on the ground ---- */
    if (has(p, MM3_MOVE_PMETER)) {
        int32_t at_speed = run && dir != 0 && mm3_abs(pl->vx) >= p->run_max - MM3_MILLI(50);
        if (on_ground) {
            if (at_speed) { if (pl->pmeter < p->pmeter_frames) pl->pmeter++; }
            else if (pl->pmeter > 0) pl->pmeter--;
        } else if (pl->pmeter < p->pmeter_frames && pl->pmeter > 0) {
            pl->pmeter--;                    /* drains in the air unless full */
        }
    }

    /* ---- crouch / crouch slide / roll ---- */
    if (pl->action == MM3_ACT_ROLL && --pl->action_timer <= 0) pl->action = MM3_ACT_NONE;
    if (on_ground && pl->action != MM3_ACT_ROLL) {
        int32_t want = (b & MM3_BTN_DOWN) != 0 && pl->action != MM3_ACT_POUND_LAND;
        mm3_player_set_crouch(pl, m, want && pl->carry < 0);
        if ((pl->flags & MM3_PF_CROUCH) && mm3_abs(pl->vx) < p->walk_max / 2) pl->crouch_timer++;
        else pl->crouch_timer = 0;
        if ((pl->flags & MM3_PF_CROUCH) && has(p, MM3_MOVE_ROLL) && (pressed & MM3_BTN_SPIN)) {
            pl->action = MM3_ACT_ROLL;
            pl->action_timer = p->roll_frames;
            pl->vx = facing_dir(pl) * p->roll_vx;
            pressed = (mm3_buttons)(pressed & ~MM3_BTN_SPIN);
        } else if ((pl->flags & MM3_PF_CROUCH) && has(p, MM3_MOVE_CROUCH_SLIDE) &&
            (mm3_abs(pl->vx) > p->walk_max / 2 || pl->slope != 0))
            pl->action = MM3_ACT_CROUCH_SLIDE;
        else if (pl->action == MM3_ACT_CROUCH_SLIDE)
            pl->action = MM3_ACT_NONE;
    }

    /* ---- swimming ---- */
    if (in_water && !(at_surface && (pl->jump_buffer > 0))) {
        pl->action = MM3_ACT_NONE;
        pl->flags &= ~(int32_t)MM3_PF_TWIRLED;
        if (dir != 0) {
            pl->vx = mm3_approach(pl->vx, dir * p->swim_max_x, p->swim_accel);
            set_facing(pl, dir);
        } else {
            pl->vx = mm3_approach(pl->vx, 0, p->swim_accel);
        }
        if (pl->jump_buffer > 0) {
            pl->vy = mm3_max(pl->vy - p->swim_stroke, -p->swim_max_rise);
            if (pl->vy > -p->swim_stroke) pl->vy = -p->swim_stroke;
            pl->jump_buffer = 0;
        }
        pl->vy = mm3_min(pl->vy + p->swim_gravity, p->swim_max_fall);
        if (!on_ground) pl->mode = MM3_MODE_SWIM;
    } else {
        int32_t speed_cap;
        pl->flags &= ~(int32_t)MM3_PF_SKIDDING;

        /* ---- horizontal ---- */
        speed_cap = run ? p->run_max : p->walk_max;
        if (has(p, MM3_MOVE_DASH) && run && dir != 0 && mm3_abs(pl->vx) >= p->run_max - MM3_MILLI(50)) {
            if (pl->run_timer < p->dash_frames) pl->run_timer++;
        } else if (!(has(p, MM3_MOVE_DASH) && run && !on_ground)) {
            pl->run_timer = 0;
        }
        if (has(p, MM3_MOVE_DASH) && pl->run_timer >= p->dash_frames && p->dash_frames > 0)
            speed_cap = p->dash_max;
        if (has(p, MM3_MOVE_PMETER) && run && pl->pmeter >= p->pmeter_frames && p->pmeter_frames > 0)
            speed_cap = p->sprint_max;

        if (pl->action == MM3_ACT_POUND_WINDUP || pl->action == MM3_ACT_POUND_FALL) {
            pl->vx = 0;
        } else if (on_ground) {
            if (pl->action == MM3_ACT_ROLL) {
                pl->vx = mm3_approach(pl->vx, 0, p->slide_friction);
            } else if (pl->action == MM3_ACT_CROUCH_SLIDE) {
                pl->vx = mm3_approach(pl->vx, 0, p->slide_friction);
                if (pl->slope != 0) pl->vx += (pl->slope > 0 ? -1 : 1) * p->slope_accel * mm3_abs(pl->slope) / 2;
                pl->vx = mm3_clamp(pl->vx, -p->run_max * 3 / 2, p->run_max * 3 / 2);
            } else if (pl->flags & MM3_PF_CROUCH) {
                if (has(p, MM3_MOVE_CRAWL) && dir != 0) {
                    pl->vx = mm3_approach(pl->vx, dir * p->crawl_speed, p->accel);
                    set_facing(pl, dir);
                } else {
                    pl->vx = mm3_approach(pl->vx, 0, p->friction);
                }
            } else if (dir == 0) {
                pl->vx = mm3_approach(pl->vx, 0, p->friction);
            } else if (pl->vx != 0 && mm3_sign(pl->vx) == -dir) {
                pl->vx = mm3_approach(pl->vx, 0, p->turn_decel);
                if (mm3_abs(pl->vx) > p->walk_max / 2) pl->flags |= MM3_PF_SKIDDING;
                set_facing(pl, dir);
            } else if (mm3_abs(pl->vx) > speed_cap) {
                pl->vx = mm3_approach(pl->vx, dir * speed_cap, p->friction);
                set_facing(pl, dir);
            } else {
                pl->vx = mm3_approach(pl->vx, dir * speed_cap, run ? p->run_accel : p->accel);
                set_facing(pl, dir);
            }
        } else if (dir != 0 && pl->action != MM3_ACT_LONG_JUMP) {
            int32_t air_cap = mm3_max(speed_cap, p->walk_max);
            if (has(p, MM3_RULE_AIR_LOCK)) air_cap = mm3_min(air_cap, pl->air_cap);
            if (pl->vx != 0 && mm3_sign(pl->vx) == -dir)
                pl->vx = mm3_approach(pl->vx, dir * air_cap, p->air_turn);
            else if (mm3_abs(pl->vx) < air_cap)
                pl->vx = mm3_approach(pl->vx, dir * air_cap, p->air_accel);
            if (pl->action != MM3_ACT_BACKFLIP) set_facing(pl, dir);
        }

        /* ---- jumps and air moves ---- */
        if ((pl->jump_buffer > 0 || (pressed & MM3_BTN_SPIN)) &&
            (on_ground || pl->coyote > 0 || (in_water && at_surface)) &&
            pl->action != MM3_ACT_POUND_LAND) {
            int32_t spin = (pressed & MM3_BTN_SPIN) && pl->jump_buffer == 0;
            if (ground_jump(s, b, dir, spin)) on_ground = 0;
        } else if (!on_ground) {
            int32_t wall = (pl->flags & MM3_PF_WALL_L) ? -1 : ((pl->flags & MM3_PF_WALL_R) ? 1 : 0);
            if (pl->jump_buffer > 0 && has(p, MM3_MOVE_WALL_JUMP) && wall != 0 &&
                pl->carry < 0 && pl->action != MM3_ACT_POUND_FALL &&
                (pl->action == MM3_ACT_WALL_SLIDE || pl->vy > 0)) {
                pl->vx = -wall * p->wall_jump_vx;
                set_facing(pl, -wall);
                pl->lock_timer = p->wall_jump_lock;
                pl->flags &= ~(int32_t)MM3_PF_TWIRLED;
                start_air(pl, MM3_ACT_WALL_JUMP, p->wall_jump_vy);
            } else if ((pressed & MM3_BTN_SPIN) && has(p, MM3_MOVE_TWIRL) &&
                       !(pl->flags & MM3_PF_TWIRLED) && pl->action != MM3_ACT_POUND_FALL &&
                       pl->action != MM3_ACT_POUND_WINDUP) {
                pl->flags |= MM3_PF_TWIRLED;
                pl->action = MM3_ACT_TWIRL;
                pl->action_timer = p->twirl_frames;
                if (pl->vy > -p->twirl_vel) pl->vy = -p->twirl_vel;
            } else if ((pressed & MM3_BTN_DOWN) && has(p, MM3_MOVE_GROUND_POUND) &&
                       pl->carry < 0 && pl->action != MM3_ACT_POUND_WINDUP &&
                       pl->action != MM3_ACT_POUND_FALL) {
                pl->action = MM3_ACT_POUND_WINDUP;
                pl->action_timer = p->pound_windup;
                pl->vx = pl->vy = 0;
            }
            /* wall slide: falling while pushing into a wall */
            if (has(p, MM3_MOVE_WALL_JUMP) && pl->carry < 0 && pl->vy > 0 && wall != 0 &&
                dir == wall && (pl->action == MM3_ACT_NONE || pl->action == MM3_ACT_JUMP ||
                pl->action == MM3_ACT_DOUBLE_JUMP || pl->action == MM3_ACT_TRIPLE_JUMP ||
                pl->action == MM3_ACT_WALL_JUMP || pl->action == MM3_ACT_SPIN_JUMP ||
                pl->action == MM3_ACT_TWIRL || pl->action == MM3_ACT_SIDEFLIP ||
                pl->action == MM3_ACT_BACKFLIP || pl->action == MM3_ACT_LONG_JUMP))
                pl->action = MM3_ACT_WALL_SLIDE;
            else if (pl->action == MM3_ACT_WALL_SLIDE && (wall == 0 || dir != wall))
                pl->action = MM3_ACT_NONE;
        }

        /* ---- gravity ---- */
        max_fall = p->max_fall;
        if (pl->action == MM3_ACT_POUND_WINDUP) {
            pl->vy = 0;
            if (--pl->action_timer <= 0) { pl->action = MM3_ACT_POUND_FALL; pl->vy = p->pound_speed; }
        } else if (pl->action == MM3_ACT_POUND_FALL) {
            pl->vy = p->pound_speed;
            max_fall = p->pound_speed;
        } else if (!on_ground || pl->vy < 0) {
            int32_t g = band_gravity(p, pl->vy, held);
            if (pl->action == MM3_ACT_SPIN_JUMP) {
                if (pl->vy > 0) g = p->spin_gravity;
                max_fall = p->spin_max_fall;
            } else if (pl->action == MM3_ACT_TWIRL) {
                max_fall = p->twirl_max_fall;
                if (--pl->action_timer <= 0) pl->action = MM3_ACT_NONE;
            } else if (pl->action == MM3_ACT_WALL_SLIDE) {
                max_fall = p->wall_slide_max;
            } else if (has(p, MM3_RULE_SLOWFALL) && held && pl->vy > 0) {
                g = mm3_min(g, p->slowfall_gravity);   /* hold jump: fall slower */
            }
            pl->vy += g;
            if (pl->vy > max_fall) pl->vy = mm3_max(max_fall, pl->vy - g * 2);
        } else {
            pl->vy = band_gravity(p, 0, 0); /* press into the ground to stay stuck */
        }
    }
    if (pl->jump_buffer > 0) pl->jump_buffer--;

    /* ---- move + collide ---- */
    mm3_move_box(m, &pl->x, &pl->y, pl->w, pl->h, pl->vx, pl->vy, on_ground && pl->vy >= 0, &hit);
    mm3_player_apply_hit(pl, &hit);

    if ((hit.blocked_r && pl->vx > 0) || (hit.blocked_l && pl->vx < 0)) pl->vx = 0;
    if (hit.head && pl->vy < 0) pl->vy = 0;
    if (hit.ground && pl->vy >= 0) {
        if (pl->mode != MM3_MODE_GROUND) {
            int32_t act = pl->action;
            pl->mode = MM3_MODE_GROUND;
            pl->flags &= ~(int32_t)MM3_PF_TWIRLED;
            if (act == MM3_ACT_POUND_FALL) {
                pl->action = MM3_ACT_POUND_LAND;
                pl->action_timer = p->pound_land_lag;
            } else {
                pl->action = MM3_ACT_NONE;
            }
            if (act == MM3_ACT_JUMP || act == MM3_ACT_DOUBLE_JUMP) pl->chain_timer = p->chain_frames;
            else { pl->chain_timer = 0; pl->chain_count = 0; }
        }
        pl->vy = 0;
    } else if (!hit.ground) {
        if (pl->mode == MM3_MODE_GROUND) {
            pl->mode = MM3_MODE_AIR;
            if (pl->action == MM3_ACT_CROUCH_SLIDE || pl->action == MM3_ACT_POUND_LAND)
                pl->action = MM3_ACT_NONE;
            if (pl->vy < 0) pl->coyote = 0;
        }
    }
    mm3_player_sense(s);
    if (has(p, MM3_MOVE_SWIM) && (pl->flags & MM3_PF_IN_WATER) && pl->mode == MM3_MODE_AIR)
        pl->mode = MM3_MODE_SWIM;
    if (pl->mode == MM3_MODE_SWIM && !(pl->flags & MM3_PF_IN_WATER)) pl->mode = MM3_MODE_AIR;
    if (pl->mode != MM3_MODE_GROUND && pl->action != MM3_ACT_JUMP && (pl->flags & MM3_PF_CROUCH) &&
        pl->action != MM3_ACT_NONE)
        mm3_player_set_crouch(pl, m, 0);
}
