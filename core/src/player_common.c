/*
 * player_common.c - Player helpers shared by all three engines:
 * crouch hitbox, water/vine sensing, carrying crates, pits and poses.
 */
#include "internal.h"

void mm3_player_reset(mm3_player *pl)
{
    int32_t sx = pl->spawn_x, sy = pl->spawn_y;
    int32_t facing = pl->flags & MM3_PF_FACING_LEFT;
    int32_t keep_prev = pl->prev_buttons;
    int32_t i;
    int32_t *p = (int32_t *)pl;
    for (i = 0; i < (int32_t)(sizeof(*pl) / sizeof(int32_t)); i++) p[i] = 0;
    pl->spawn_x = sx; pl->spawn_y = sy;
    pl->x = sx; pl->y = sy;
    pl->w = MM3_STAND_W; pl->h = MM3_STAND_H;
    pl->flags = facing;
    pl->carry = -1;
    pl->mode = MM3_MODE_GROUND;
    pl->prev_buttons = keep_prev;
    pl->r_moving_dir = 1;
    pl->r_vforce_down = 0x28; /* level-start default of the 8-bit engine */
}

void mm3_player_set_crouch(mm3_player *pl, const mm3_tilemap *m, int32_t crouch)
{
    int32_t is = (pl->flags & MM3_PF_CROUCH) != 0;
    if (crouch && !is) {
        pl->y += MM3_STAND_H - MM3_CROUCH_H;
        pl->h = MM3_CROUCH_H;
        pl->flags |= MM3_PF_CROUCH;
    } else if (!crouch && is) {
        mm3_fx ny = pl->y - (MM3_STAND_H - MM3_CROUCH_H);
        if (mm3_box_solid(m, pl->x, ny, pl->w, MM3_STAND_H)) return; /* no headroom */
        pl->y = ny;
        pl->h = MM3_STAND_H;
        pl->flags &= ~(int32_t)MM3_PF_CROUCH;
    }
}

void mm3_player_sense(mm3_game_state *s)
{
    mm3_player *pl = &s->player;
    mm3_fx cx = pl->x + pl->w / 2, cy = pl->y + pl->h / 2;
    int32_t t = mm3_tile_at_point(&s->map, cx, cy);
    pl->flags &= ~(int32_t)(MM3_PF_IN_WATER | MM3_PF_ON_VINE);
    if (t == MM3_TILE_WATER) pl->flags |= MM3_PF_IN_WATER;
    if (t == MM3_TILE_VINE) pl->flags |= MM3_PF_ON_VINE;
}

void mm3_player_apply_hit(mm3_player *pl, const mm3_hit *hit)
{
    pl->flags &= ~(int32_t)(MM3_PF_WALL_L | MM3_PF_WALL_R | MM3_PF_HEAD_BUMP | MM3_PF_ON_SLOPE);
    if (hit->wall_l) pl->flags |= MM3_PF_WALL_L;
    if (hit->wall_r) pl->flags |= MM3_PF_WALL_R;
    if (hit->head) pl->flags |= MM3_PF_HEAD_BUMP;
    pl->slope = hit->ground ? hit->slope : 0;
    if (pl->slope != 0) pl->flags |= MM3_PF_ON_SLOPE;
}

/* ---- carrying ---- */

static int32_t overlaps(mm3_fx ax, mm3_fx ay, mm3_fx aw, mm3_fx ah,
                        mm3_fx bx, mm3_fx by, mm3_fx bw, mm3_fx bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

int32_t mm3_player_try_pickup(mm3_game_state *s)
{
    mm3_player *pl = &s->player;
    int32_t i;
    if (pl->carry >= 0) return 0;
    for (i = 0; i < MM3_MAX_OBJS; i++) {
        mm3_object *o = &s->objs[i];
        if (o->type != MM3_OBJ_CRATE || o->state == MM3_OBJS_CARRIED) continue;
        /* reach a few pixels past the hitbox on both sides */
        if (overlaps(pl->x - MM3_PX(4), pl->y, pl->w + MM3_PX(8), pl->h, o->x, o->y, o->w, o->h)) {
            o->state = MM3_OBJS_CARRIED;
            o->vx = o->vy = 0;
            pl->carry = i;
            mm3_player_update_carry(s);
            return 1;
        }
    }
    return 0;
}

void mm3_player_update_carry(mm3_game_state *s)
{
    mm3_player *pl = &s->player;
    mm3_object *o;
    if (pl->carry < 0) return;
    o = &s->objs[pl->carry];
    /* held in front of the body at hand height */
    o->x = (pl->flags & MM3_PF_FACING_LEFT) ? pl->x - o->w + MM3_PX(4) : pl->x + pl->w - MM3_PX(4);
    o->y = pl->y + pl->h - o->h - MM3_PX(6);
    o->vx = pl->vx;
    o->vy = 0;
}

void mm3_player_throw(mm3_game_state *s, mm3_buttons b, mm3_fx vx, mm3_fx vy, mm3_fx up_vy)
{
    mm3_player *pl = &s->player;
    mm3_object *o;
    int32_t dir = (pl->flags & MM3_PF_FACING_LEFT) ? -1 : 1;
    if (pl->carry < 0) return;
    o = &s->objs[pl->carry];
    pl->carry = -1;
    o->state = MM3_OBJS_THROWN;
    if ((b & MM3_BTN_UP) && up_vy > 0) {
        o->vx = pl->vx / 2;
        o->vy = -up_vy;
        o->x = pl->x + (pl->w - o->w) / 2;
        o->y = pl->y - o->h;
    } else if (b & MM3_BTN_DOWN) {
        o->vx = 0;
        o->vy = 0;
        o->state = MM3_OBJS_REST; /* set down gently */
    } else {
        o->vx = dir * vx;
        o->vy = -vy;
    }
    /* never leave the crate inside a wall */
    if (mm3_box_solid(&s->map, o->x, o->y, o->w, o->h)) {
        o->x = pl->x + (pl->w - o->w) / 2;
        o->y = pl->y;
    }
    pl->throw_timer = 12;
}

/* Touching a resting crate sends it sliding away from the player. */
void mm3_player_try_kick(mm3_game_state *s, mm3_fx vx)
{
    mm3_player *pl = &s->player;
    int32_t i;
    for (i = 0; i < MM3_MAX_OBJS; i++) {
        mm3_object *o = &s->objs[i];
        mm3_fx pc = pl->x + pl->w / 2, oc = o->x + o->w / 2;
        if (o->type != MM3_OBJ_CRATE || o->state != MM3_OBJS_REST) continue;
        if (!overlaps(pl->x, pl->y, pl->w, pl->h, o->x, o->y, o->w, o->h)) continue;
        o->state = MM3_OBJS_THROWN;
        o->vx = (oc >= pc ? 1 : -1) * vx;
        o->vy = 0;
        pl->throw_timer = 8;
    }
}

void mm3_player_check_pit(mm3_game_state *s)
{
    mm3_player *pl = &s->player;
    if (pl->y > (s->map.height + 3) * MM3_TILE_FX) {
        if (pl->carry >= 0) {
            s->objs[pl->carry].state = MM3_OBJS_REST;
            pl->carry = -1;
        }
        mm3_player_reset(pl);
    }
}

/* ---- pose ---- */

void mm3_player_update_pose(mm3_game_state *s, mm3_buttons b)
{
    mm3_player *pl = &s->player;
    int32_t speed = mm3_abs(pl->vx);
    int32_t pose = MM3_POSE_IDLE;
    int32_t engine = mm3_style_engine((mm3_style)s->style);
    int32_t sprint = 0, run = 0;

    if (engine == MM3_ENGINE_RETRO) {
        run = mm3_abs(pl->r_xspeed) >= 0x1C;
    } else if (engine == MM3_ENGINE_ISLAND) {
        run = speed >= MM3_SUB16(0x23);
        sprint = pl->pmeter >= 0x70 && speed >= MM3_SUB16(0x2C);
    } else {
        run = speed > s->profile.walk_max + MM3_MILLI(100);
        sprint = (s->profile.dash_frames > 0 && pl->run_timer >= s->profile.dash_frames) ||
                 ((s->profile.moves & MM3_MOVE_PMETER) && s->profile.pmeter_frames > 0 &&
                  pl->pmeter >= s->profile.pmeter_frames && speed > s->profile.run_max);
    }

    if (pl->throw_timer > 0) {
        pl->throw_timer--;
        pose = MM3_POSE_THROW;
    } else if (pl->mode == MM3_MODE_CLIMB) {
        pose = MM3_POSE_CLIMB;
    } else if (pl->mode == MM3_MODE_SWIM) {
        pose = MM3_POSE_SWIM;
    } else if (pl->mode == MM3_MODE_GROUND) {
        if (pl->action == MM3_ACT_POUND_LAND) pose = MM3_POSE_CROUCH;
        else if (pl->action == MM3_ACT_ROLL) pose = MM3_POSE_TRIPLE;
        else if (pl->action == MM3_ACT_CROUCH_SLIDE) pose = MM3_POSE_SLIDE;
        else if (pl->flags & MM3_PF_CROUCH) pose = MM3_POSE_CROUCH;
        else if (pl->flags & MM3_PF_SKIDDING) pose = MM3_POSE_SKID;
        else if (speed == 0) pose = (b & MM3_BTN_UP) ? MM3_POSE_LOOK_UP : MM3_POSE_IDLE;
        else pose = sprint ? MM3_POSE_SPRINT : (run ? MM3_POSE_RUN : MM3_POSE_WALK);
    } else {
        switch (pl->action) {
        case MM3_ACT_SPIN_JUMP:    pose = MM3_POSE_SPIN; break;
        case MM3_ACT_TWIRL:        pose = MM3_POSE_TWIRL; break;
        case MM3_ACT_TRIPLE_JUMP:  pose = MM3_POSE_TRIPLE; break;
        case MM3_ACT_LONG_JUMP:    pose = MM3_POSE_LONG_JUMP; break;
        case MM3_ACT_BACKFLIP:     pose = MM3_POSE_BACKFLIP; break;
        case MM3_ACT_SIDEFLIP:     pose = MM3_POSE_SIDEFLIP; break;
        case MM3_ACT_WALL_SLIDE:   pose = MM3_POSE_WALL_SLIDE; break;
        case MM3_ACT_POUND_WINDUP:
        case MM3_ACT_POUND_FALL:   pose = MM3_POSE_POUND; break;
        default:
            if (pl->flags & MM3_PF_CROUCH) pose = MM3_POSE_CROUCH;
            else if (sprint && pl->vy < 0) pose = MM3_POSE_SPRINT;
            else pose = pl->vy < 0 ? MM3_POSE_JUMP : MM3_POSE_FALL;
        }
    }
    /* animation clock: advances with movement, and steadily in the air */
    if (pose != pl->pose) pl->anim = 0;
    pl->anim += 1 + speed / MM3_PX(1);
    pl->pose = pose;
}
