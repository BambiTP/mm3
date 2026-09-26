/*
 * engine_retro.c - 8-bit style movement (source: SMB1).
 *
 * This reproduces the source game's player physics algorithm step by step,
 * using its own units, so speeds, jump arcs and quirks match exactly:
 *   - horizontal speed: signed byte in 1/16 px/frame + an 8-bit fraction
 *     that is shared between acceleration and sub-pixel position,
 *   - vertical speed: signed byte in px/frame + 8-bit "move force",
 *   - jump gravity picked at takeoff from 5 speed tiers (+ water tiers),
 *   - no mid-air turning, instant stop when turning slowly, etc.
 * Only collision is shared with the other engines (the original used 8x8
 * metatile probes; levels here also have slopes the original never had).
 *
 * The player is always the "big" form (can crouch). Constants: docs/PHYSICS.md.
 */
#include "internal.h"

/* Tables indexed by jump tier 0-4 (by run speed) and 5-6 (water). */
static const int32_t k_jump_mforce[7] = {0x20, 0x20, 0x1E, 0x28, 0x28, 0x0D, 0x04};
/* Fall force: original except tier 0 (0x70 -> 0x83) so a tapped jump
   matches the other styles. */
static const int32_t k_fall_mforce[7] = {0x83, 0x70, 0x60, 0x90, 0x90, 0x0A, 0x09};
/*
 * Launch speed per tier in 1/256 px/frame (speed byte * 256 + move force).
 * MM3 change: the jump tiers (0-4) are re-tuned by tools/calibrate_jump.c so
 * standing / walking / running jumps reach the same heights as every other
 * style. The original values were {-4.0, -4.0, -4.0, -5.0, -5.0} px/f
 * (PlayerYSpdData $fc,$fc,$fc,$fb,$fb with InitMForceData 0); swim tiers
 * (5, 6) are unchanged.
 */
static const int32_t k_launch[7] = {-1032, -1115, -1080, -1277, -1277, -384, -256};
/* Max speed and acceleration by physics row: 0 = run, 1 = walk, 2 = water. */
static const int32_t k_max_right[3] = {0x28, 0x18, 0x10};
static const int32_t k_friction[3] = {0xE4, 0x98, 0xD0};

#define STATE_GROUND 0
#define STATE_JUMP   1
#define STATE_FALL   2
#define STATE_CLIMB  3

/* Joypad-style bits used by the original: right = 1, left = 2. */
static int32_t lr_bits(mm3_buttons b)
{
    return ((b & MM3_BTN_RIGHT) ? 1 : 0) | ((b & MM3_BTN_LEFT) ? 2 : 0);
}

static int32_t retro_state(const mm3_player *pl)
{
    if (pl->mode == MM3_MODE_CLIMB) return STATE_CLIMB;
    if (pl->mode == MM3_MODE_GROUND) return STATE_GROUND;
    return pl->action == MM3_ACT_JUMP ? STATE_JUMP : STATE_FALL;
}

static void set_state(mm3_player *pl, int32_t st, int32_t in_water)
{
    if (st == STATE_CLIMB) { pl->mode = MM3_MODE_CLIMB; pl->action = MM3_ACT_NONE; }
    else if (st == STATE_GROUND) { pl->mode = MM3_MODE_GROUND; pl->action = MM3_ACT_NONE; }
    else {
        pl->mode = in_water ? MM3_MODE_SWIM : MM3_MODE_AIR;
        pl->action = st == STATE_JUMP ? MM3_ACT_JUMP : MM3_ACT_NONE;
    }
}

/* ImposeFriction: accelerate or brake the horizontal speed. */
static void impose_friction(mm3_player *pl, int32_t bits, int32_t adder, int32_t max_l, int32_t max_r)
{
    int32_t v;
    int32_t accel_right;
    if (bits == 0) {
        if (pl->r_xspeed == 0) return;
        accel_right = pl->r_xspeed < 0;   /* brake toward zero */
    } else {
        accel_right = (bits & 1) != 0;    /* right wins when both are held */
    }
    /* 16-bit add/subtract of {speed byte : fraction} */
    v = pl->r_xspeed * 256 + pl->r_xmf;
    v += accel_right ? adder : -adder;
    pl->r_xmf = v & 0xFF;
    pl->r_xspeed = mm3_s8(mm3_floor_div(v, 256));
    if (accel_right) {
        if (pl->r_xspeed >= max_r) pl->r_xspeed = max_r;
    } else {
        if (pl->r_xspeed < max_l) pl->r_xspeed = max_l;
    }
}

/* MoveObjectHorizontally: returns whole pixels moved this frame. */
static int32_t move_horizontal(mm3_player *pl)
{
    int32_t s = pl->r_xspeed & 0xFF;
    int32_t lo = (s << 4) & 0xFF;      /* low nybble moved to high */
    int32_t hi = s >> 4;               /* high nybble, sign extended */
    int32_t carry;
    if (hi >= 8) hi -= 16;
    pl->r_xmf += lo;
    carry = pl->r_xmf >> 8;
    pl->r_xmf &= 0xFF;
    return hi + carry;
}

/* ImposeGravity for the player (downward only, max 4 px/frame). */
static int32_t move_vertical(mm3_player *pl)
{
    int32_t carry, dy;
    pl->r_ydummy += pl->r_ymf;
    carry = pl->r_ydummy >> 8;
    pl->r_ydummy &= 0xFF;
    dy = pl->r_yspeed + carry;
    pl->r_ymf += pl->r_vforce;
    carry = pl->r_ymf >> 8;
    pl->r_ymf &= 0xFF;
    pl->r_yspeed = mm3_s8(pl->r_yspeed + carry);
    if (pl->r_yspeed >= 4 && pl->r_ymf >= 0x80) {
        pl->r_yspeed = 4;
        pl->r_ymf = 0;
    }
    return dy;
}

/* GetPlayerAnimSpeed side effects: running flag and slow turnaround. */
static void anim_speed_rules(mm3_player *pl, mm3_buttons raw)
{
    int32_t abs_speed = mm3_abs(pl->r_xspeed);
    int32_t non_a = raw & (MM3_BTN_LEFT | MM3_BTN_RIGHT | MM3_BTN_UP | MM3_BTN_DOWN |
                           MM3_BTN_RUN | MM3_BTN_START);
    int32_t facing = (pl->flags & MM3_PF_FACING_LEFT) ? 2 : 1;
    if (abs_speed >= 0x1C) { pl->r_running_speed = abs_speed; return; }
    if (non_a == 0) return;
    if (lr_bits(raw) == pl->r_moving_dir) { pl->r_running_speed = 0; return; }
    if (abs_speed < 0x0B) {                /* too slow to skid: turn instantly */
        pl->r_moving_dir = facing;
        pl->r_xspeed = 0;
        pl->r_xmf = 0;
    }
}

static void init_jump(mm3_player *pl, int32_t swimming)
{
    int32_t tier = 0, a = mm3_abs(pl->r_xspeed);
    pl->r_swim_timer = 0x20;
    pl->r_ydummy = 0;
    pl->r_ymf = 0;
    pl->r_jump_origin = mm3_fx_to_px(pl->y);
    if (a >= 0x09) tier++;
    if (a >= 0x10) tier++;
    if (a >= 0x19) tier++;
    if (a >= 0x1C) tier++;
    if (swimming) tier = 5;
    pl->r_vforce = k_jump_mforce[tier];
    pl->r_vforce_down = k_fall_mforce[tier];
    pl->r_yspeed = mm3_floor_div(k_launch[tier], 256);
    pl->r_ymf = k_launch[tier] - pl->r_yspeed * 256;
}

void mm3_engine_retro_tick(mm3_game_state *s, mm3_buttons b)
{
    mm3_player *pl = &s->player;
    const mm3_tilemap *m = &s->map;
    mm3_buttons pressed = (mm3_buttons)(b & ~pl->prev_buttons);
    int32_t st = retro_state(pl);
    int32_t in_water, at_surface;
    int32_t lr = lr_bits(b), ud_down = (b & MM3_BTN_DOWN) != 0;
    int32_t a_held = (b & MM3_BTN_JUMP) != 0, a_prev = (pl->prev_buttons & MM3_BTN_JUMP) != 0;
    int32_t facing = (pl->flags & MM3_PF_FACING_LEFT) ? 2 : 1;
    int32_t dx = 0, dy = 0;
    mm3_hit hit;

    /* frame timers */
    if (pl->r_swim_timer > 0) pl->r_swim_timer--;
    if (pl->run_timer > 0) pl->run_timer--;
    if (pl->r_climb_timer > 0) pl->r_climb_timer--;

    mm3_player_sense(s);
    in_water = (pl->flags & MM3_PF_IN_WATER) != 0;
    /* near the top of the water: the tile above the head isn't water */
    at_surface = in_water &&
        mm3_tile_at_point(m, pl->x + pl->w / 2, pl->y - MM3_PX(4)) != MM3_TILE_WATER;

    /* Grab a vine by holding UP (the original grabbed on touch; that felt
       bad, so this follows the maker game). Not while still rising from a
       jump off the vine. */
    if (st != STATE_CLIMB && (pl->flags & MM3_PF_ON_VINE) && (b & MM3_BTN_UP) &&
        (st == STATE_GROUND || pl->r_yspeed >= 0)) {
        st = STATE_CLIMB;
        pl->r_xspeed = 0; pl->r_xmf = 0;
        pl->r_yspeed = 0; pl->r_ymf = 0;
        mm3_player_set_crouch(pl, m, 0);
    }

    /* Down + left/right on the ground cancels both (original quirk). */
    if (ud_down && st == STATE_GROUND && lr) { lr = 0; ud_down = 0; }

    /* Crouch flag only changes on the ground (kept during a crouch jump). */
    if (st == STATE_GROUND) mm3_player_set_crouch(pl, m, ud_down);

    if (st == STATE_CLIMB && (pressed & MM3_BTN_JUMP)) {
        /* MM3 addition: jump off the vine */
        init_jump(pl, 0);
        st = STATE_JUMP;
    }
    if (st == STATE_CLIMB) {
        /* PlayerPhysicsSub, climbing branch */
        if (b & MM3_BTN_UP)        { pl->r_yspeed = -1; pl->r_ymf = 0x20; }
        else if (b & MM3_BTN_DOWN) { pl->r_yspeed = 1;  pl->r_ymf = 0xFF; }
        else                       { pl->r_yspeed = 0;  pl->r_ymf = 0x00; }
    } else {
        int32_t row = 0, fr = 0, abs_speed;
        /* CheckForJumping (A pressed this frame; no buffering in the original) */
        if (pressed & MM3_BTN_JUMP) {
            if (in_water && !at_surface) {
                /* swim stroke: from the ground, or once the stroke timer allows */
                if (st == STATE_GROUND || pl->r_swim_timer != 0 || pl->r_yspeed >= 0) {
                    init_jump(pl, 1);
                    st = STATE_JUMP;
                }
            } else if (st == STATE_GROUND || in_water) {
                /* normal jump; at the water surface this jumps out (MM3 addition:
                   the original's water levels had no surface to leave) */
                init_jump(pl, 0);
                st = STATE_JUMP;
            }
        }
        /* X_Physics: pick max speed row and acceleration */
        abs_speed = mm3_abs(pl->r_xspeed);
        if (st != STATE_GROUND) {
            if (abs_speed >= 0x19) { row = 0; fr = 0; }
            else { row = 1; fr = 1; if (pl->r_running_speed != 0 || abs_speed >= 0x21) fr = 2; }
        } else if (in_water) {
            row = 2; fr = 1;
            if (pl->r_running_speed != 0 || abs_speed >= 0x21) fr = 2;
        } else if (lr != pl->r_moving_dir) {
            row = 1; fr = 1;
            if (pl->r_running_speed != 0 || abs_speed >= 0x21) fr = 2;
        } else if (b & MM3_BTN_RUN) {
            pl->run_timer = 0x0A;
            row = 0; fr = 0;
        } else if (pl->run_timer != 0) {
            row = 0; fr = 0;
        } else {
            row = 1; fr = 1;
            if (pl->r_running_speed != 0 || abs_speed >= 0x21) fr = 2;
        }
        {
            int32_t adder = k_friction[fr];
            if (facing != pl->r_moving_dir) adder <<= 1;  /* turning brakes harder */

            if (st == STATE_GROUND) {
                anim_speed_rules(pl, b);
                if (lr) facing = lr;
                impose_friction(pl, lr, adder, -k_max_right[row], k_max_right[row]);
                dx = move_horizontal(pl);
            } else {
                if (st == STATE_JUMP) {
                    /* holding A keeps the low gravity until the jump is released */
                    if (pl->r_yspeed >= 0) pl->r_vforce = pl->r_vforce_down;
                    else if (!(a_held && a_prev) &&
                             pl->r_jump_origin - mm3_fx_to_px(pl->y) >= 1)
                        pl->r_vforce = pl->r_vforce_down;
                    if (in_water) {
                        anim_speed_rules(pl, b);
                        if (at_surface) pl->r_vforce = 0x18;
                        if (lr) facing = lr;
                    }
                } else {
                    pl->r_vforce = pl->r_vforce_down;
                }
                if (lr) impose_friction(pl, lr, adder, -k_max_right[row], k_max_right[row]);
                dx = move_horizontal(pl);
                dy = move_vertical(pl);
            }
        }
    }

    if (st == STATE_CLIMB) {
        /* ClimbingSub */
        int32_t carry;
        pl->r_ydummy += pl->r_ymf;
        carry = pl->r_ydummy >> 8;
        pl->r_ydummy &= 0xFF;
        dy = pl->r_yspeed + carry;
        /* MM3 change: slide smoothly along the vine instead of the
           original's 24-frame side hops (k_climb_adder), which felt stiff. */
        if (lr & 1) { dx = 1; facing = 1; }
        else if (lr & 2) { dx = -1; facing = 2; }
    } else {
        pl->r_climb_timer = 0x18;
    }

    /* moving direction follows the speed sign */
    if (pl->r_xspeed != 0) pl->r_moving_dir = pl->r_xspeed > 0 ? 1 : 2;

    /* ---- collision ---- */
    {
        mm3_fx oy = pl->y;
        int32_t was_ground = st == STATE_GROUND;
        mm3_move_box(m, &pl->x, &pl->y, pl->w, pl->h, dx * MM3_FX_ONE, dy * MM3_FX_ONE,
                     was_ground, &hit);
        mm3_player_apply_hit(pl, &hit);
        if (st == STATE_CLIMB) {
            mm3_player_sense(s);
            if (!(pl->flags & MM3_PF_ON_VINE)) {
                if (dy < 0) { pl->y = oy; }            /* top of the vine */
                else { st = STATE_FALL; pl->r_yspeed = 0; pl->r_ymf = 0; }
            }
            if (hit.ground && dy > 0) { st = STATE_GROUND; }
        } else {
            if ((hit.blocked_r && pl->r_xspeed > 0) || (hit.blocked_l && pl->r_xspeed < 0)) {
                pl->r_xspeed = 0;
            }
            if (hit.head && pl->r_yspeed < 0) {
                pl->r_yspeed = 1;                      /* bonk */
                pl->r_ymf = 0;
                st = STATE_FALL;
            }
            if (hit.ground && st != STATE_GROUND && pl->r_yspeed >= 0) {
                st = STATE_GROUND;
                pl->r_yspeed = 0;
                pl->r_ymf = 0;
            } else if (!hit.ground && st == STATE_GROUND) {
                st = STATE_FALL;                        /* walked off a ledge */
            }
        }
    }

    set_state(pl, st, (pl->flags & MM3_PF_IN_WATER) != 0);
    if (facing == 2) pl->flags |= MM3_PF_FACING_LEFT;
    else pl->flags &= ~(int32_t)MM3_PF_FACING_LEFT;
    pl->flags &= ~(int32_t)MM3_PF_SKIDDING;
    if (st == STATE_GROUND && lr && pl->r_xspeed != 0 && lr != pl->r_moving_dir)
        pl->flags |= MM3_PF_SKIDDING;

    /* public speeds for the renderer and objects */
    pl->vx = pl->r_xspeed * 256;
    pl->vy = pl->r_yspeed * MM3_FX_ONE + pl->r_ymf * 16;
}
