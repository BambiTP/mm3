/*
 * calibrate_jump.c - Finds jump numbers for Athletic and Bloom that reach the
 * same heights as the maker 2D styles while keeping their own gravity feel.
 *
 * Build: cmake -B build -DMM3_BUILD_TOOLS=ON && cmake --build build
 * Run:   ./build/calibrate_jump      (prints values for physics_profiles.c)
 *
 * Uses the real engine, so the results match the game exactly.
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

/* Apex rise (1/4096 px) and airtime of a jump with the given profile. */
static mm3_fx apex(const mm3_modern_profile *p, mm3_fx vx, mm3_buttons jump, int hold, int *airtime)
{
    mm3_fx start, best;
    int f;
    mm3_init(&S, 1u, MM3_STYLE_CUSTOM);
    mm3_load_ascii_level(&S, k_room, (int32_t)(sizeof(k_room) / sizeof(k_room[0])));
    mm3_set_custom_profile(&S, p);
    mm3_tick(&S, 0);
    start = best = S.player.y + S.player.h;
    S.player.vx = vx;
    for (f = 0; f < 400; f++) {
        mm3_tick(&S, f < hold ? jump : 0);
        if (S.player.y + S.player.h < best) best = S.player.y + S.player.h;
        if (f > 2 && S.player.mode == MM3_MODE_GROUND) break;
    }
    if (airtime) *airtime = f;
    return start - best;
}

/* Smallest launch value in [lo, hi] whose apex reaches target. */
static int32_t solve(mm3_modern_profile *p, int32_t *field, mm3_fx vx, mm3_buttons jump,
                     int hold, mm3_fx target, int32_t lo, int32_t hi)
{
    while (lo < hi) {
        int32_t mid = lo + (hi - lo) / 2;
        *field = mid;
        if (apex(p, vx, jump, hold, NULL) >= target) hi = mid; else lo = mid + 1;
    }
    *field = lo;
    return lo;
}

static void scale_bands(int32_t *dst, const int32_t *src, int pct)
{
    int i;
    for (i = 0; i < MM3_GRAV_BANDS; i++) dst[i] = src[i] * pct / 100;
}

static void calibrate(const char *name, mm3_style style, int held_pct)
{
    const mm3_modern_profile *maker = mm3_builtin_profile(MM3_STYLE_MODERN);
    mm3_modern_profile p = *mm3_builtin_profile(style);
    /* representative speeds inside each jump-bonus band: still, walk, run, P-speed */
    static const mm3_fx speeds[4] = {0, MM3_MILLI(1200), MM3_MILLI(2500), MM3_MILLI(3000)};
    int32_t v[4];
    int i, rel_pct, lo, hi, t_maker, t_style;
    mm3_fx target;

    /* same speed bands as the maker styles, own gravity strength */
    memcpy(p.jump_bonus_speed, maker->jump_bonus_speed, sizeof(p.jump_bonus_speed));
    memcpy(p.grav_threshold, maker->grav_threshold, sizeof(p.grav_threshold));
    scale_bands(p.grav_held, maker->grav_held, held_pct);
    scale_bands(p.grav_released, maker->grav_released, held_pct);
    p.moves &= ~(uint32_t)(MM3_MOVE_TRIPLE_JUMP | MM3_RULE_AIR_LOCK);

    for (i = 0; i < 4; i++) {
        mm3_modern_profile m = *maker;
        m.moves &= ~(uint32_t)MM3_MOVE_TRIPLE_JUMP;
        target = apex(&m, speeds[i], MM3_BTN_JUMP, 400, NULL);
        p.jump_vel = 0;
        memset(p.jump_bonus, 0, sizeof(p.jump_bonus));
        v[i] = solve(&p, &p.jump_vel, speeds[i], MM3_BTN_JUMP, 400, target, MM3_MILLI(1000), MM3_MILLI(9000));
    }
    p.jump_vel = v[0];
    for (i = 0; i < 4; i++) p.jump_bonus[i] = v[i] - v[0];

    /* tap height: scale the released bands until a 1-frame jump matches */
    target = apex(maker, 0, MM3_BTN_JUMP, 1, NULL);
    lo = 20; hi = 400;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        scale_bands(p.grav_released, maker->grav_released, mid);
        if (apex(&p, 0, MM3_BTN_JUMP, 1, NULL) <= target) hi = mid; else lo = mid + 1;
    }
    rel_pct = lo;
    scale_bands(p.grav_released, maker->grav_released, rel_pct);

    printf("/* %s: held gravity %d%%, released %d%% of maker */\n", name, held_pct, rel_pct);
    printf("jump_vel %d  bonus {0, %d, %d, %d}\n", (int)p.jump_vel, (int)p.jump_bonus[1],
           (int)p.jump_bonus[2], (int)p.jump_bonus[3]);
    printf("held  {");
    for (i = 0; i < MM3_GRAV_BANDS; i++) printf("%d%s", (int)p.grav_held[i], i < 5 ? ", " : "}\n");
    printf("rel   {");
    for (i = 0; i < MM3_GRAV_BANDS; i++) printf("%d%s", (int)p.grav_released[i], i < 5 ? ", " : "}\n");

    /* crouch and spin jumps */
    if (p.moves & MM3_MOVE_CROUCH_JUMP) {
        mm3_modern_profile m = *maker;
        m.jump_vel = maker->crouch_jump_vel; memset(m.jump_bonus, 0, sizeof(m.jump_bonus));
        target = apex(&m, 0, MM3_BTN_JUMP, 400, NULL);
        {
            mm3_modern_profile q = p;
            memset(q.jump_bonus, 0, sizeof(q.jump_bonus));
            printf("crouch_jump_vel %d\n", (int)solve(&q, &q.jump_vel, 0, MM3_BTN_JUMP, 400, target,
                                                      MM3_MILLI(1000), MM3_MILLI(9000)));
        }
    }
    if (p.moves & MM3_MOVE_SPIN_JUMP) {
        target = apex(maker, 0, MM3_BTN_SPIN, 400, NULL);
        printf("spin_jump_vel %d\n", (int)solve(&p, &p.spin_jump_vel, 0, MM3_BTN_SPIN, 400, target,
                                                MM3_MILLI(1000), MM3_MILLI(9000)));
    }

    /* feel check: airtime of a full standing jump */
    apex(maker, 0, MM3_BTN_JUMP, 400, &t_maker);
    apex(&p, 0, MM3_BTN_JUMP, 400, &t_style);
    printf("airtime: maker %d frames, %s %d frames\n\n", t_maker, name, t_style);
}

int main(void)
{
    calibrate("Athletic", MM3_STYLE_ATHLETIC, 130);
    calibrate("Bloom", MM3_STYLE_BLOOM, 65);
    return 0;
}
