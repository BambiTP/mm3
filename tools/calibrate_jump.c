/*
 * calibrate_jump.c - Makes every style jump the same heights.
 *
 * Targets come from the maker 2D styles, measured with the real engine:
 *   T0 standing, T1 walking, T2 running, T3 full speed (P-meter / dash),
 *   plus a tapped standing jump.
 * For each other style it searches the launch numbers that hit those
 * heights while keeping that style's own gravity (its "feel"):
 *   - Athletic / Bloom: jump_vel + jump_bonus[4] (+ released-gravity scale)
 *   - Retro Classic: launch speed per original speed tier + tier-0 fall force
 *   - Island Classic: jump table per speed index + release gravity
 * The Classic engines are simulated here with their exact vertical math
 * (tests/test_moves.c then checks the real engines in real play).
 *
 * Build: cmake -B build -DMM3_BUILD_TOOLS=ON && cmake --build build
 * Run:   ./build/calibrate_jump
 */
#include <stdio.h>
#include <string.h>
#include "mm3/sim.h"

static const char *const k_room[] = {
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#..............................................................................................#",
    "#.P............................................................................................#",
    "################################################################################################"
};

static mm3_game_state S;

/* Tier setup: speed and meters for a jump in tier t. */
static void set_tier(const mm3_modern_profile *p, int t)
{
    mm3_player *pl = &S.player;
    switch (t) {
    case 0: pl->vx = 0; break;
    case 1: pl->vx = p->walk_max; break;
    case 2: pl->vx = p->run_max; break;
    default:
        pl->vx = p->run_max + MM3_MILLI(200);
        pl->pmeter = p->pmeter_frames;
        pl->run_timer = p->dash_frames;
        break;
    }
}

/* Apex rise (1/4096 px) of a modern-engine jump. */
static mm3_fx apex(const mm3_modern_profile *p, int tier, mm3_buttons jump, int hold, int *airtime)
{
    mm3_fx start, best;
    int f;
    mm3_init(&S, 1u, MM3_STYLE_CUSTOM);
    mm3_load_ascii_level(&S, k_room, (int32_t)(sizeof(k_room) / sizeof(k_room[0])));
    mm3_set_custom_profile(&S, p);
    mm3_tick(&S, 0);
    start = best = S.player.y + S.player.h;
    set_tier(p, tier);
    for (f = 0; f < 400; f++) {
        /* full speed needs run held to keep the P-meter / dash */
        mm3_buttons keep = tier == 3 ? (mm3_buttons)(MM3_BTN_RUN | MM3_BTN_RIGHT) : 0;
        mm3_tick(&S, (mm3_buttons)((f < hold ? jump : 0) | keep));
        if (S.player.y + S.player.h < best) best = S.player.y + S.player.h;
        if (f > 2 && S.player.mode == MM3_MODE_GROUND) break;
    }
    if (airtime) *airtime = f;
    return start - best;
}

static void scale_bands(int32_t *dst, const int32_t *src, int pct)
{
    int i;
    for (i = 0; i < MM3_GRAV_BANDS; i++) dst[i] = src[i] * pct / 100;
}

static mm3_fx T[4], TAP, SPIN;

static void targets(void)
{
    const mm3_modern_profile *maker = mm3_builtin_profile(MM3_STYLE_MODERN);
    mm3_modern_profile m = *maker;
    int t;
    m.moves &= ~(uint32_t)(MM3_MOVE_TRIPLE_JUMP | MM3_RULE_AIR_LOCK);
    for (t = 0; t < 4; t++) T[t] = apex(&m, t, MM3_BTN_JUMP, 400, NULL);
    TAP = apex(&m, 0, MM3_BTN_JUMP, 1, NULL);
    SPIN = apex(&m, 0, MM3_BTN_SPIN, 400, NULL);
    printf("targets (px): stand %.2f walk %.2f run %.2f full %.2f tap %.2f spin %.2f\n\n",
           T[0] / 4096.0, T[1] / 4096.0, T[2] / 4096.0, T[3] / 4096.0, TAP / 4096.0, SPIN / 4096.0);
}

/* ---------------------------------------------------------- modern */

static void calibrate_modern(const char *name, mm3_style style, int held_pct)
{
    const mm3_modern_profile *maker = mm3_builtin_profile(MM3_STYLE_MODERN);
    mm3_modern_profile p = *mm3_builtin_profile(style);
    int32_t v[4];
    int t, lo, hi, t_maker, t_style;

    memcpy(p.grav_threshold, maker->grav_threshold, sizeof(p.grav_threshold));
    scale_bands(p.grav_held, maker->grav_held, held_pct);
    scale_bands(p.grav_released, maker->grav_released, held_pct);
    p.moves &= ~(uint32_t)(MM3_MOVE_TRIPLE_JUMP | MM3_RULE_AIR_LOCK);
    memset(p.jump_bonus, 0, sizeof(p.jump_bonus));
    for (t = 0; t < 4; t++) {
        lo = MM3_MILLI(1000); hi = MM3_MILLI(9000);
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            p.jump_vel = mid;
            if (apex(&p, t, MM3_BTN_JUMP, 400, NULL) >= T[t]) hi = mid; else lo = mid + 1;
        }
        v[t] = lo;
    }
    p.jump_vel = v[0];
    for (t = 0; t < 4; t++) p.jump_bonus[t] = v[t] - v[0];
    lo = 20; hi = 400;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        scale_bands(p.grav_released, maker->grav_released, mid);
        if (apex(&p, 0, MM3_BTN_JUMP, 1, NULL) <= TAP) hi = mid; else lo = mid + 1;
    }
    scale_bands(p.grav_released, maker->grav_released, lo);
    printf("/* %s: held gravity %d%%, released %d%% of maker */\n", name, held_pct, lo);
    printf("jump_vel %d bonus {0, %d, %d, %d}\n", (int)p.jump_vel, (int)p.jump_bonus[1],
           (int)p.jump_bonus[2], (int)p.jump_bonus[3]);
    apex(maker, 0, MM3_BTN_JUMP, 400, &t_maker);
    apex(&p, 0, MM3_BTN_JUMP, 400, &t_style);
    printf("airtime maker %d, %s %d\n\n", t_maker, name, t_style);
}

/* ---------------------------------------------------- retro classic */

static const int32_t k_retro_jump_force[5] = {0x20, 0x20, 0x1E, 0x28, 0x28};
static const int32_t k_retro_fall_force[5] = {0x70, 0x70, 0x60, 0x90, 0x90};

/* Rise in 1/4096 px of the 8-bit engine for launch v (1/256 px/f, <0 up). */
static mm3_fx retro_rise(int32_t v, int32_t jf, int32_t ff, int hold)
{
    int32_t yspeed = mm3_floor_div(v, 256), ymf = v - yspeed * 256, ydummy = 0;
    int32_t y = 0, best = 0, origin = 0, vforce = jf, f;
    for (f = 0; f < 200; f++) {
        int32_t carry, dy;
        int32_t held = f < hold, prev = f > 0 && f - 1 < hold;
        if (yspeed >= 0) vforce = ff;
        else if (!(held && prev) && origin - y >= 1) vforce = ff;
        ydummy += ymf; carry = ydummy >> 8; ydummy &= 0xFF;
        dy = yspeed + carry;
        ymf += vforce; carry = ymf >> 8; ymf &= 0xFF;
        yspeed = mm3_s8(yspeed + carry);
        if (yspeed >= 4 && ymf >= 0x80) { yspeed = 4; ymf = 0; }
        y += dy;
        if (y < best) best = y;
        if (y > 0) break;
    }
    return -best * MM3_FX_ONE;
}

static int32_t solve_retro(mm3_fx target, int32_t jf, int32_t ff, int hold)
{
    int32_t lo = -4096, hi = -128;       /* launch, 1/256 px/f, more negative = higher */
    while (lo < hi) {
        int32_t mid = lo + (hi - lo + 1) / 2;
        if (retro_rise(mid, jf, ff, hold) >= target) lo = mid; else hi = mid - 1;
    }
    return lo;
}

static void calibrate_retro(void)
{
    static const int tier_target[5] = {0, 1, 1, 2, 2};
    int32_t v[5];
    int i, ff;
    printf("/* Retro Classic: launch per speed tier (1/256 px/f) */\n{");
    for (i = 0; i < 5; i++) {
        v[i] = solve_retro(T[tier_target[i]], k_retro_jump_force[i], k_retro_fall_force[i], 400);
        printf("%d%s", (int)v[i], i < 4 ? ", " : "}\n");
    }
    for (i = 0; i < 5; i++)
        printf("  tier %d rise %.2f px (target %.2f)\n", i,
               retro_rise(v[i], k_retro_jump_force[i], k_retro_fall_force[i], 400) / 4096.0,
               T[tier_target[i]] / 4096.0);
    /* tier-0 fall force for the tapped jump */
    {
        int best_ff = 0x70;
        mm3_fx best_err = 0x7FFFFFFF;
        for (ff = 0x20; ff <= 0xFF; ff++) {
            mm3_fx err = mm3_abs(retro_rise(v[0], k_retro_jump_force[0], ff, 1) - TAP);
            if (err < best_err) { best_err = err; best_ff = ff; }
        }
        printf("tier 0 fall force 0x%02X: tap %.2f px (target %.2f)\n\n", best_ff,
               retro_rise(v[0], k_retro_jump_force[0], best_ff, 1) / 4096.0, TAP / 4096.0);
    }
}

/* --------------------------------------------------- island classic */

/* Rise (1/4096 px) of the 16-bit engine: launch j (high byte, <0 up). */
static mm3_fx island_rise(int32_t j, int32_t g_rel, int hold)
{
    int32_t vy = j, y = 0, best = 0, f;
    /* the launch frame applies gravity before the first move */
    vy += 0 < hold ? 3 : g_rel;
    for (f = 1; f < 200; f++) {
        int32_t a;
        y += vy;                          /* move first (1/16 px) */
        if (y < best) best = y;
        if (y > 0) break;
        a = vy;
        if (a >= 0 && a > 0x40) a = 0x40;
        a += f < hold ? 3 : g_rel;
        vy = a;
    }
    return -best * 256;
}

static int32_t solve_island(mm3_fx target, int32_t g_rel, int hold)
{
    int32_t j, best_j = -80;
    mm3_fx best_err = 0x7FFFFFFF;
    for (j = -140; j <= -20; j++) {
        mm3_fx err = mm3_abs(island_rise(j, g_rel, hold) - target);
        if (err < best_err) { best_err = err; best_j = j; }
    }
    return best_j;
}

static void calibrate_island(void)
{
    static const int idx_target[8] = {0, 1, 1, 2, 2, 3, 3, 3};
    int32_t j[8], spin, g, best_g = 6;
    mm3_fx best_err = 0x7FFFFFFF;
    int i;
    for (i = 0; i < 8; i++) j[i] = solve_island(T[idx_target[i]], 6, 400);
    spin = solve_island(SPIN, 6, 400);
    for (g = 3; g <= 20; g++) {
        mm3_fx err = mm3_abs(island_rise(j[0], g, 1) - TAP);
        if (err < best_err) { best_err = err; best_g = g; }
    }
    printf("/* Island Classic: jump table {normal, spin} by speed index */\n");
    for (i = 0; i < 8; i++)
        printf("  idx %d: %d, %d   rise %.2f px (target %.2f)\n", i, (int)j[i], (int)spin,
               island_rise(j[i], best_g, 400) / 4096.0, T[idx_target[i]] / 4096.0);
    printf("spin rise %.2f (target %.2f)\n", island_rise(spin, best_g, 400) / 4096.0, SPIN / 4096.0);
    printf("release gravity %d: tap %.2f px (target %.2f)\n\n", (int)best_g,
           island_rise(j[0], best_g, 1) / 4096.0, TAP / 4096.0);
}

int main(void)
{
    targets();
    calibrate_modern("Athletic", MM3_STYLE_ATHLETIC, 130);
    calibrate_modern("Bloom", MM3_STYLE_BLOOM, 65);
    calibrate_retro();
    calibrate_island();
    return 0;
}
