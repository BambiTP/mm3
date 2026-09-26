/*
 * engine_island.c - 16-bit style movement (source: SMW).
 *
 * Reproduces the source game's player algorithm in its own units:
 *   - speeds are 16-bit "4.12" values (1/4096 px per frame = our unit);
 *     the high byte (1/16 px per frame) drives position and comparisons,
 *   - acceleration 1.5 units/frame, skid 2.5 (walk) / 5 (run), friction 1,
 *   - top speed 20 / 36 / 48 units (walk / run / P-speed sprint); the
 *     accelerate-then-brake cycle makes the speed oscillate just above the
 *     cap (e.g. 47-48-47-48-49 when sprinting), exactly like the original,
 *   - P-meter +2 per frame while running at >= 35 units, -1 otherwise,
 *     full at 112,
 *   - jump launch speed from a table indexed by |speed|, separate spin
 *     jump column, gravity 3 (button held) or 6, fall speed capped at 64
 *     before gravity is added.
 * Collision is shared with the other engines. Constants: docs/PHYSICS.md.
 */
#include "internal.h"

#define IN_AIR_JUMP    0x0B
#define IN_AIR_PJUMP   0x0C
#define IN_AIR_FALL    0x24
#define PMETER_FULL    0x70

/* Jump launch Y speed (high byte), pairs of {normal, spin} by |X speed|/8. */
/*
 * MM3 change: re-tuned by tools/calibrate_jump.c so standing / walking /
 * running / P-speed jumps reach the same heights as every other style, and
 * the spin jump matches the maker spin jump. The original table
 * (DATA_00D2BD) was normal -0x50,-0x52,-0x55,-0x57,-0x5A,-0x5C,-0x5F,-0x61
 * and spin -0x4A,-0x4C,-0x4E,-0x50,-0x52,-0x55,-0x57,-0x5A.
 */
static const int32_t k_jump_table[16] = {
    -82, -73, -88, -73, -88, -73, -90, -73,
    -90, -73, -93, -73, -93, -73, -93, -73
};
/* Max X speed (high byte) by (row << 1 | dir): walk, run, run, sprint. */
static const int32_t k_max_speed[8] = {-20, 20, -36, 36, -36, 36, -48, 48};
/* Water max X speed: ground, air. */
static const int32_t k_water_max[4] = {-8, 8, -16, 16};
/* Water: most negative (upward) speed by up/down held. */
static const int32_t k_water_rise[4] = {-24, -8, -48, -48};
static const int32_t k_pmeter_step[3] = {-1, -1, 2};

#define ACCEL      384   /* 0x0180 */
#define SKID_WALK  640   /* 0x0280 */
#define SKID_RUN   1280  /* 0x0500 */
#define FRICTION   256   /* 0x0100 */

static int32_t hi(int32_t v16) { return mm3_floor_div(v16, 256); }

/* P-meter update; returns the (possibly promoted) speed row. */
static int32_t pmeter_step(mm3_player *pl, int32_t row)
{
    pl->pmeter += k_pmeter_step[row];
    if (pl->pmeter < 0) pl->pmeter = 0;
    if (pl->pmeter >= PMETER_FULL) { pl->pmeter = PMETER_FULL; row++; }
    return row;
}

/* Brake toward zero by one unit; never overshoot (CODE_00D76B). */
static void friction(mm3_player *pl)
{
    int32_t f = hi(pl->vx) >= 0 ? -FRICTION : FRICTION;
    int32_t t = pl->vx + f;
    if (t != 0 && mm3_sign(t) == mm3_sign(f)) t = 0;
    pl->vx = t;
}

/* Accelerate toward a max, or brake if at/over it (CODE_00D742). */
static void accel_toward(mm3_player *pl, int32_t max_hi, int32_t accel)
{
    int32_t diff = hi(pl->vx) - max_hi;
    if (diff == 0 || mm3_sign(diff) == mm3_sign(max_hi)) friction(pl);
    else pl->vx += accel;
}

static void do_throw_release(mm3_game_state *s, mm3_buttons b)
{
    /* kick 0x2E, toss up 0x70 (units of 1/16 px per frame) */
    mm3_player_throw(s, b, MM3_SUB16(0x2E), 0, MM3_SUB16(0x70));
}

static void horizontal(mm3_player *pl, mm3_buttons b, int32_t on_ground)
{
    int32_t lr = b & (MM3_BTN_LEFT | MM3_BTN_RIGHT);
    pl->flags &= ~(int32_t)MM3_PF_SKIDDING;
    if (lr) {
        int32_t dir = (b & MM3_BTN_RIGHT) ? 1 : 0;
        int32_t accel = dir ? ACCEL : -ACCEL;
        int32_t row = 0;
        int32_t sh = hi(pl->vx);
        int32_t skid = sh != 0 && mm3_sign(sh) != (dir ? 1 : -1);
        int32_t run = (b & MM3_BTN_RUN) != 0;
        if (dir) pl->flags &= ~(int32_t)MM3_PF_FACING_LEFT;
        else pl->flags |= MM3_PF_FACING_LEFT;
        if (skid) {
            accel = (dir ? 1 : -1) * (run ? SKID_RUN : SKID_WALK);
            if (on_ground) pl->flags |= MM3_PF_SKIDDING;
        }
        if (run) {
            row = 1;
            if (mm3_abs(sh) >= 0x23 && (on_ground || pl->i_in_air == IN_AIR_PJUMP)) row = 2;
        }
        row = pmeter_step(pl, row);
        accel_toward(pl, k_max_speed[(row << 1) | dir], accel);
    } else {
        pmeter_step(pl, 0);
        if (on_ground) friction(pl);
    }
}

static void gravity(mm3_player *pl, mm3_buttons b)
{
    int32_t held = (b & (MM3_BTN_JUMP | MM3_BTN_SPIN)) != 0;
    int32_t a = hi(pl->vy);
    if (a >= 0) {
        if (a >= 0x40) a = 0x40;
        if (pl->i_in_air == IN_AIR_JUMP) pl->i_in_air = IN_AIR_FALL;
    }
    /* held 3 (original); released 11 instead of the original 6 so a tapped
       jump matches the other styles (MM3 change) */
    a += held ? 3 : 11;
    pl->vy = a * 256;
}

static void swim(mm3_game_state *s, mm3_buttons b, mm3_buttons pressed, int32_t on_ground)
{
    mm3_player *pl = &s->player;
    int32_t y = hi(pl->vy);
    int32_t ud = ((b & MM3_BTN_DOWN) ? 1 : 0) | ((b & MM3_BTN_UP) ? 2 : 0);
    pl->i_spin = 0;
    if (pressed & (MM3_BTN_JUMP | MM3_BTN_SPIN)) {
        if (on_ground) { pl->i_in_air = IN_AIR_JUMP; y = -0x10; }
        y -= 0x20;                                  /* swim stroke */
    }
    if ((s->frame & 3) == 0) y += 2;               /* slow sinking */
    if (y >= 0) { if (y > 0x40) y = 0x40; }
    else if (y < k_water_rise[ud]) y = k_water_rise[ud];
    pl->vy = y * 256;

    mm3_player_set_crouch(pl, &s->map, on_ground && (b & MM3_BTN_DOWN) && !(pressed & MM3_BTN_JUMP));
    if ((b & (MM3_BTN_LEFT | MM3_BTN_RIGHT)) && !(pl->flags & MM3_PF_CROUCH)) {
        int32_t dir = (b & MM3_BTN_RIGHT) ? 1 : 0;
        if (dir) pl->flags &= ~(int32_t)MM3_PF_FACING_LEFT;
        else pl->flags |= MM3_PF_FACING_LEFT;
        accel_toward(pl, k_water_max[(on_ground ? 0 : 2) | dir], dir ? ACCEL : -ACCEL);
    } else {
        friction(pl);
    }
}

/* Move with last frame's speeds and resolve collisions (CODE_00CD24). */
static void move_and_collide(mm3_game_state *s)
{
    mm3_player *pl = &s->player;
    int32_t was_ground = pl->mode == MM3_MODE_GROUND;
    mm3_hit hit;
    if (pl->vy < 0 && (pl->flags & MM3_PF_HEAD_BUMP)) pl->vy = 0;
    mm3_move_box(&s->map, &pl->x, &pl->y, pl->w, pl->h, hi(pl->vx) * 256, hi(pl->vy) * 256,
                 was_ground, &hit);
    mm3_player_apply_hit(pl, &hit);
    if ((hit.blocked_r && pl->vx > 0) || (hit.blocked_l && pl->vx < 0)) pl->vx = 0;
    if (hit.head && pl->vy < 0) pl->vy = 0;
    if (hit.ground && pl->vy >= 0) {
        if (pl->mode != MM3_MODE_GROUND) pl->i_spin = 0;
        pl->mode = MM3_MODE_GROUND;
        pl->i_in_air = 0;
        pl->vy = 0;
    } else if (!hit.ground) {
        if (pl->mode == MM3_MODE_GROUND) pl->i_in_air = IN_AIR_FALL;
        pl->mode = MM3_MODE_AIR;
    }
}

/*
 * Frame order follows the original: first move with the speeds computed last
 * frame and collide, then read input and compute new speeds. So a jump
 * pressed this frame first moves the player next frame.
 */
void mm3_engine_island_tick(mm3_game_state *s, mm3_buttons b)
{
    mm3_player *pl = &s->player;
    const mm3_tilemap *m = &s->map;
    mm3_buttons pressed = (mm3_buttons)(b & ~pl->prev_buttons);
    mm3_buttons released = (mm3_buttons)(pl->prev_buttons & ~b);
    int32_t on_ground, in_water, at_surface;
    mm3_hit hit;

    if (pl->mode != MM3_MODE_CLIMB) move_and_collide(s);
    on_ground = pl->mode == MM3_MODE_GROUND;

    mm3_player_sense(s);
    in_water = (pl->flags & MM3_PF_IN_WATER) != 0;
    at_surface = in_water &&
        mm3_tile_at_point(m, pl->x + pl->w / 2, pl->y - MM3_PX(2)) != MM3_TILE_WATER;
    if (in_water && pl->mode == MM3_MODE_AIR) pl->mode = MM3_MODE_SWIM;
    if (!in_water && pl->mode == MM3_MODE_SWIM) pl->mode = MM3_MODE_AIR;

    /* ---- carrying: hold RUN to grab, release to kick / toss / drop ---- */
    if (pl->carry < 0 && (b & MM3_BTN_RUN) && pl->mode != MM3_MODE_CLIMB)
        mm3_player_try_pickup(s);
    else if (pl->carry >= 0 && (released & MM3_BTN_RUN))
        do_throw_release(s, b);

    /* ---- climbing (vines): 8 units/frame, 16 holding RUN ---- */
    if (pl->mode != MM3_MODE_CLIMB && (pl->flags & MM3_PF_ON_VINE) && (b & MM3_BTN_UP) &&
        pl->carry < 0 && pl->vy >= 0) {
        pl->mode = MM3_MODE_CLIMB;
        pl->vx = pl->vy = 0;
        pl->i_spin = 0;
        pl->pmeter = 0;
        mm3_player_set_crouch(pl, m, 0);
    }
    if (pl->mode == MM3_MODE_CLIMB) {
        int32_t sp = (b & MM3_BTN_RUN) ? 16 : 8;
        mm3_fx oy = pl->y;
        int32_t dx = 0, dy = 0;
        if (pressed & (MM3_BTN_JUMP | MM3_BTN_SPIN)) {
            pl->mode = MM3_MODE_AIR;
            pl->i_in_air = IN_AIR_JUMP;
            pl->vy = -0x50 * 256;                    /* vine jump */
            if (pressed & MM3_BTN_SPIN) pl->i_spin = 1;  /* spin off the vine */
        } else {
            if (b & MM3_BTN_LEFT) dx = -sp;
            if (b & MM3_BTN_RIGHT) dx = sp;
            if (b & MM3_BTN_UP) dy = -sp;
            if (b & MM3_BTN_DOWN) dy = sp;
            if (dx) { if (dx < 0) pl->flags |= MM3_PF_FACING_LEFT; else pl->flags &= ~(int32_t)MM3_PF_FACING_LEFT; }
            mm3_move_box(m, &pl->x, &pl->y, pl->w, pl->h, dx * 256, dy * 256, 0, &hit);
            mm3_player_apply_hit(pl, &hit);
            mm3_player_sense(s);
            if (!(pl->flags & MM3_PF_ON_VINE)) {
                if (dy < 0) pl->y = oy;               /* top of the vine */
                else { pl->mode = MM3_MODE_AIR; pl->i_in_air = IN_AIR_FALL; }
            }
            if (hit.ground && dy > 0) { pl->mode = MM3_MODE_GROUND; pl->i_in_air = 0; }
        }
        pl->action = MM3_ACT_NONE;
        return;
    }

    /* ---- swimming ---- */
    if (in_water && !(at_surface && (pressed & (MM3_BTN_JUMP | MM3_BTN_SPIN)))) {
        swim(s, b, pressed, on_ground);
    } else {
        /* ---- ground: ducking and jumping (CODE_00D5F2) ---- */
        if (on_ground || in_water) {
            int32_t duck = on_ground && (b & MM3_BTN_DOWN) != 0 && pl->slope == 0;
            int32_t jumping = (pressed & (MM3_BTN_JUMP | MM3_BTN_SPIN)) != 0 &&
                              !mm3_box_solid(m, pl->x, pl->y - MM3_PX(1), pl->w, pl->h);
            if (jumping) {
                int32_t idx = (mm3_abs(hi(pl->vx)) >> 2) & 0xFE;
                if ((pressed & MM3_BTN_SPIN) && pl->carry < 0) { idx++; pl->i_spin = 1; }
                pl->vy = k_jump_table[idx] * 256;
                pl->i_in_air = pl->pmeter >= PMETER_FULL ? IN_AIR_PJUMP : IN_AIR_JUMP;
                pl->mode = MM3_MODE_AIR;
                on_ground = 0;
            }
            mm3_player_set_crouch(pl, m, duck || (jumping && (pl->flags & MM3_PF_CROUCH)));
            if (!jumping && on_ground && (pl->flags & MM3_PF_CROUCH)) {
                pmeter_step(pl, 0);
                friction(pl);
                pl->flags &= ~(int32_t)MM3_PF_SKIDDING;
            } else {
                horizontal(pl, b, on_ground);
            }
        } else {
            horizontal(pl, b, on_ground);
        }
        gravity(pl, b);
    }

    /* ---- slopes: ducking on a slope slides downhill ---- */
    if (on_ground && pl->slope != 0 && (b & MM3_BTN_DOWN)) {
        int32_t down_dir = pl->slope > 0 ? -1 : 1;
        pl->vx += down_dir * 0x60 * mm3_abs(pl->slope);
        pl->vx = mm3_clamp(pl->vx, -48 * 256, 48 * 256);
        pl->flags |= MM3_PF_CROUCH;
    }

    pl->action = pl->i_spin ? MM3_ACT_SPIN_JUMP :
                 (pl->mode == MM3_MODE_AIR && pl->vy < 0 ? MM3_ACT_JUMP : MM3_ACT_NONE);
    if (on_ground && pl->slope != 0 && (b & MM3_BTN_DOWN)) pl->action = MM3_ACT_CROUCH_SLIDE;
}
