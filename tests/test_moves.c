/*
 * test_moves.c - Checks each physics style against its source game's known
 * numbers and checks that every move works.
 *
 * Heights and distances are measured in tiles (16 px). The 16-bit targets
 * come from measurements of the original game (see docs/PHYSICS.md).
 */
#include <stdio.h>
#include <string.h>
#include "mm3/sim.h"

static int g_fail = 0, g_checks = 0;
static int g_verbose = 0;

#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } else if (g_verbose) { printf("ok   "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static mm3_game_state S;

/* 120-wide flat test room; tall enough for any jump. */
static const char *const k_flat[] = {
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#......................................................................................................................#",
    "#.P....................................................................................................................#",
    "########################################################################################################################",
    "########################################################################################################################"
};

/* Features: wall-jump shaft, water, vine, crate, slopes. */
static const char *const k_features[] = {
    "#..........................................................................................#",
    "#..........................................................................................#",
    "#..........................................................................................#",
    "#...........#...#...........................|..............................................#",
    "#...........#...#...........................|..............................................#",
    "#...........#...#...........................|..............................................#",
    "#...........#...#...........................|..............................................#",
    "#...........#...#...........................|..............................................#",
    "#...........#...#.........#~~~~~~~~~~#......|......................................../\\....#",
    "#...........#...#.........#~~~~~~~~~~#......|......................................./##\\...#",
    "#.P.........#...#.........#~~~~~~~~~~#......|.......C.............uUDd............./####\\..#",
    "###########################~~~~~~~~~~#######################################################",
    "###########################~~~~~~~~~~#######################################################",
    "############################################################################################"
};

static void setup(mm3_style style, const char *const *rows, int n)
{
    mm3_init(&S, 7u, style);
    mm3_load_ascii_level(&S, rows, n);
}
#define SETUP(style, lvl) setup(style, lvl, (int)(sizeof(lvl) / sizeof(lvl[0])))

static void run(int frames, mm3_buttons b) { while (frames-- > 0) mm3_tick(&S, b); }

static double px(mm3_fx v) { return (double)v / MM3_FX_ONE; }
static double tiles(mm3_fx v) { return (double)v / MM3_TILE_FX; }
static int hi16(mm3_fx v) { return (int)mm3_floor_div(v, 256); }

/* Jump with `hold` held for up to `hold_frames`, return feet rise in tiles
   and (optionally) horizontal distance until landing. */
static double jump_rise(mm3_buttons move, mm3_buttons jump, int hold_frames, double *dist)
{
    mm3_fx start_y = S.player.y + S.player.h, best = start_y;
    mm3_fx start_x = S.player.x;
    int f;
    for (f = 0; f < 400; f++) {
        mm3_buttons b = move | (f < hold_frames ? jump : 0);
        mm3_tick(&S, b);
        if (S.player.y + S.player.h < best) best = S.player.y + S.player.h;
        if (f > 2 && S.player.mode == MM3_MODE_GROUND) break;
    }
    if (dist) *dist = tiles(mm3_abs(S.player.x - start_x));
    return tiles(start_y - best);
}

/* ---------------------------------------------------------------- 8-bit */

static void test_retro(void)
{
    int f, maxv = 0;
    double rise, dist;

    SETUP(MM3_STYLE_RETRO, k_flat);
    run(120, MM3_BTN_RIGHT);
    CHECK(S.player.r_xspeed == 0x18, "retro walk top speed = 0x18 (1.5 px/f): got 0x%X", S.player.r_xspeed);

    SETUP(MM3_STYLE_RETRO, k_flat);
    for (f = 0; f < 150; f++) { mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN); if (S.player.r_xspeed > maxv) maxv = S.player.r_xspeed; }
    CHECK(maxv == 0x28, "retro run top speed = 0x28 (2.5 px/f): got 0x%X", maxv);

    SETUP(MM3_STYLE_RETRO, k_flat);
    rise = jump_rise(0, MM3_BTN_JUMP, 200, NULL);
    CHECK(rise > 3.8 && rise < 4.2, "retro standing full jump ~4 tiles: %.2f", rise);

    SETUP(MM3_STYLE_RETRO, k_flat);
    run(12, 0);
    rise = jump_rise(0, MM3_BTN_JUMP, 1, NULL);
    CHECK(rise > 0.9 && rise < 2.2, "retro tapped jump is short: %.2f", rise);

    SETUP(MM3_STYLE_RETRO, k_flat);
    run(90, MM3_BTN_RIGHT | MM3_BTN_RUN);
    rise = jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_JUMP, 200, &dist);
    CHECK(rise > 4.8 && rise < 5.25, "retro running full jump ~5 tiles: %.2f", rise);

    /* no mid-air turning: facing stays when pressing back in the air */
    SETUP(MM3_STYLE_RETRO, k_flat);
    run(60, MM3_BTN_RIGHT | MM3_BTN_RUN);
    mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_JUMP);
    run(10, MM3_BTN_LEFT | MM3_BTN_JUMP);
    CHECK(!(S.player.flags & MM3_PF_FACING_LEFT), "retro can't turn around mid-air");
    CHECK(S.player.r_xspeed > 0, "retro keeps momentum in the air (speed 0x%X)", S.player.r_xspeed);

    /* crouch */
    SETUP(MM3_STYLE_RETRO, k_flat);
    run(5, MM3_BTN_DOWN);
    CHECK(S.player.pose == MM3_POSE_CROUCH, "retro crouch pose");
}

/* --------------------------------------------------------------- 16-bit */

static void test_island(void)
{
    int f, lo, hi, pm_frame = -1, top_frame = -1, samples[64];
    double rise, dist;

    /* walking speed oscillates just above 20 */
    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(100, MM3_BTN_RIGHT);
    lo = 99; hi = -99;
    for (f = 0; f < 20; f++) { mm3_tick(&S, MM3_BTN_RIGHT); if (hi16(S.player.vx) < lo) lo = hi16(S.player.vx); if (hi16(S.player.vx) > hi) hi = hi16(S.player.vx); }
    CHECK(hi == 21 && lo >= 19, "island walk speed peaks at 21 (range %d-%d)", lo, hi);

    /* P-meter: full after ~80 frames, speed 49 after ~90 */
    SETUP(MM3_STYLE_ISLAND, k_flat);
    for (f = 1; f <= 200; f++) {
        mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN);
        if (pm_frame < 0 && S.player.pmeter >= 0x70) pm_frame = f;
        if (top_frame < 0 && hi16(S.player.vx) >= 49) top_frame = f;
    }
    CHECK(pm_frame >= 76 && pm_frame <= 84, "island P-meter full at ~80 frames: %d", pm_frame);
    CHECK(top_frame >= 84 && top_frame <= 96, "island reaches 49 at ~90 frames: %d", top_frame);
    for (f = 0; f < 10; f++) { mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN); samples[f] = hi16(S.player.vx); }
    lo = 99; hi = -99;
    for (f = 0; f < 10; f++) { if (samples[f] < lo) lo = samples[f]; if (samples[f] > hi) hi = samples[f]; }
    CHECK(hi == 49 && lo == 47, "island sprint speed oscillates 47-49: %d %d %d %d %d", samples[0], samples[1], samples[2], samples[3], samples[4]);

    /* launch speeds: 77/82/87/92 normal, 71/75/79/84 spin (first airborne frame) */
    {
        static const int want[4] = {77, 82, 87, 92};
        static const int want_spin[4] = {71, 75, 79, 84};
        static const int speeds[4] = {0, 21, 37, 49};
        int i, spin;
        for (spin = 0; spin < 2; spin++) {
            for (i = 0; i < 4; i++) {
                SETUP(MM3_STYLE_ISLAND, k_flat);
                S.player.vx = speeds[i] * 256;
                S.player.flags |= 0;
                mm3_tick(&S, spin ? MM3_BTN_SPIN : MM3_BTN_JUMP);
                CHECK(-hi16(S.player.vy) == (spin ? want_spin[i] : want[i]),
                      "island %s launch at speed %d = %d: got %d", spin ? "spin" : "jump",
                      speeds[i], spin ? want_spin[i] : want[i], -hi16(S.player.vy));
            }
        }
    }

    /* heights: normal (running) 5, sprint 6 (barely), spin 4/5, minimum 2 */
    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(60, MM3_BTN_RIGHT | MM3_BTN_RUN);
    S.player.pmeter = 0; S.player.vx = 37 * 256;
    rise = jump_rise(MM3_BTN_RIGHT, MM3_BTN_JUMP, 200, NULL);
    CHECK(rise > 4.6 && rise < 5.5, "island running jump ~5 tiles: %.2f", rise);

    /* takeoff at 48-49 (the high point of the sprint oscillation); at 47 the
       table gives one step less, which is why the original's max is "barely" */
    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(150, MM3_BTN_RIGHT | MM3_BTN_RUN);
    while (hi16(S.player.vx) < 48) mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN);
    rise = jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_JUMP, 200, &dist);
    CHECK(rise > 5.5 && rise < 6.1, "island sprint jump just under 6 tiles: %.2f", rise);
    CHECK(dist > 10.5 && dist < 13.5, "island sprint jump ~12 tiles long: %.2f", dist);

    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(150, MM3_BTN_RIGHT | MM3_BTN_RUN);
    rise = jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_SPIN, 200, &dist);
    CHECK(rise > 4.5 && rise < 5.1, "island sprint spin jump ~5 tiles: %.2f", rise);
    CHECK(dist > 9.5 && dist < 12.5, "island sprint spin jump ~11 tiles long: %.2f", dist);

    SETUP(MM3_STYLE_ISLAND, k_flat);
    S.player.vx = 37 * 256;
    rise = jump_rise(MM3_BTN_RIGHT, MM3_BTN_SPIN, 200, NULL);
    CHECK(rise > 3.8 && rise < 4.6, "island running spin jump ~4 tiles: %.2f", rise);

    SETUP(MM3_STYLE_ISLAND, k_flat);
    rise = jump_rise(0, MM3_BTN_JUMP, 1, NULL);
    CHECK(rise > 1.5 && rise < 2.3, "island minimum jump ~2 tiles: %.2f", rise);

    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(100, MM3_BTN_RIGHT);
    rise = jump_rise(MM3_BTN_RIGHT, MM3_BTN_JUMP, 200, &dist);
    CHECK(dist > 4.0 && dist < 6.0, "island walking jump ~5 tiles long: %.2f", dist);

    /* spin jump state, look up, duck */
    SETUP(MM3_STYLE_ISLAND, k_flat);
    mm3_tick(&S, MM3_BTN_SPIN);
    run(3, MM3_BTN_SPIN);
    CHECK(S.player.pose == MM3_POSE_SPIN, "island spin jump pose");
    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(3, MM3_BTN_UP);
    CHECK(S.player.pose == MM3_POSE_LOOK_UP, "island look up pose");
    SETUP(MM3_STYLE_ISLAND, k_flat);
    run(3, MM3_BTN_DOWN);
    CHECK(S.player.pose == MM3_POSE_CROUCH, "island duck pose");
}

/* --------------------------------------------------------------- modern */

static void test_modern_moves(void)
{
    double r1, r3;
    int i;

    /* triple jump: three running jumps in a row, third is highest */
    SETUP(MM3_STYLE_MODERN, k_flat);
    run(60, MM3_BTN_RIGHT | MM3_BTN_RUN);
    r1 = jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_JUMP, 30, NULL);
    CHECK(S.player.chain_timer > 0, "modern chain window opens after landing");
    mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_DOUBLE_JUMP, "modern second jump is a double jump (action %d)", (int)S.player.action);
    jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_JUMP, 30, NULL);
    mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_TRIPLE_JUMP, "modern third jump is a triple jump (action %d)", (int)S.player.action);
    r3 = jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_JUMP, 30, NULL) ;
    CHECK(r3 > r1, "modern triple jump is higher than the first (%.2f > %.2f)", r3, r1);

    /* ground pound */
    SETUP(MM3_STYLE_MODERN, k_flat);
    run(12, MM3_BTN_JUMP);
    mm3_tick(&S, MM3_BTN_DOWN);
    CHECK(S.player.action == MM3_ACT_POUND_WINDUP, "modern ground pound windup");
    for (i = 0; i < 120 && S.player.mode != MM3_MODE_GROUND; i++) mm3_tick(&S, 0);
    CHECK(S.player.action == MM3_ACT_POUND_LAND, "modern ground pound lands");

    /* twirl only once per jump */
    SETUP(MM3_STYLE_MODERN, k_flat);
    run(10, MM3_BTN_JUMP);
    mm3_tick(&S, MM3_BTN_SPIN);
    CHECK(S.player.action == MM3_ACT_TWIRL, "modern twirl in the air");
    run(30, 0);
    mm3_tick(&S, MM3_BTN_SPIN);
    CHECK(S.player.action != MM3_ACT_TWIRL, "modern twirl can't repeat in one jump");

    /* wall slide + wall jump inside the shaft (x = 13..15) */
    SETUP(MM3_STYLE_MODERN, k_features);
    mm3_place_player(&S, 13, 10);
    run(5, 0);
    run(12, MM3_BTN_JUMP | MM3_BTN_LEFT);
    for (i = 0; i < 60 && S.player.action != MM3_ACT_WALL_SLIDE; i++) mm3_tick(&S, MM3_BTN_LEFT);
    CHECK(S.player.action == MM3_ACT_WALL_SLIDE, "modern wall slide against a wall");
    mm3_tick(&S, MM3_BTN_LEFT | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_WALL_JUMP && S.player.vx > 0 && S.player.vy < 0, "modern wall jump kicks away and up");

    /* no wall = no mid-air jump */
    SETUP(MM3_STYLE_MODERN, k_flat);
    run(20, MM3_BTN_JUMP);
    run(2, 0);
    mm3_tick(&S, MM3_BTN_JUMP);
    CHECK(S.player.action != MM3_ACT_WALL_JUMP && S.player.vy > -MM3_PX(3), "modern can't jump in mid-air without a wall");

    /* spin jump from the ground */
    SETUP(MM3_STYLE_MODERN, k_flat);
    mm3_tick(&S, MM3_BTN_SPIN);
    CHECK(S.player.action == MM3_ACT_SPIN_JUMP, "modern spin jump");

    /* Retro and Athletic have no triple jump */
    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    run(60, MM3_BTN_RIGHT | MM3_BTN_RUN);
    jump_rise(MM3_BTN_RIGHT | MM3_BTN_RUN, MM3_BTN_JUMP, 30, NULL);
    mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_JUMP, "athletic has no double jump");
}

static void test_athletic_moves(void)
{
    double r_jump, r_back;
    int i;

    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    r_jump = jump_rise(0, MM3_BTN_JUMP, 200, NULL);
    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    run(5, MM3_BTN_DOWN);
    mm3_tick(&S, MM3_BTN_DOWN | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_BACKFLIP, "athletic backflip from a still crouch");
    r_back = jump_rise(0, MM3_BTN_JUMP, 200, NULL);
    CHECK(r_back > r_jump, "backflip is higher than a normal jump (%.2f > %.2f)", r_back, r_jump);

    /* long jump: run, crouch, jump */
    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    run(90, MM3_BTN_RIGHT | MM3_BTN_RUN);
    mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN | MM3_BTN_DOWN);
    mm3_tick(&S, MM3_BTN_RIGHT | MM3_BTN_RUN | MM3_BTN_DOWN | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_LONG_JUMP && S.player.vx > S.profile.run_max,
          "athletic long jump is faster than running (%.2f px/f)", px(S.player.vx));

    /* side flip: jump while skidding */
    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    run(60, MM3_BTN_RIGHT | MM3_BTN_RUN);
    mm3_tick(&S, MM3_BTN_LEFT);
    mm3_tick(&S, MM3_BTN_LEFT | MM3_BTN_JUMP);
    CHECK(S.player.action == MM3_ACT_SIDEFLIP && S.player.vx < 0, "athletic side flip reverses direction");

    /* dash: keep running to go faster than run speed */
    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    run(200, MM3_BTN_RIGHT | MM3_BTN_RUN);
    CHECK(S.player.vx > S.profile.run_max, "athletic dash beats run speed (%.2f px/f)", px(S.player.vx));

    /* crouch slide */
    SETUP(MM3_STYLE_ATHLETIC, k_flat);
    run(60, MM3_BTN_RIGHT | MM3_BTN_RUN);
    mm3_tick(&S, MM3_BTN_DOWN);
    CHECK(S.player.action == MM3_ACT_CROUCH_SLIDE, "athletic crouch slide while running");
    for (i = 0; i < 5; i++) mm3_tick(&S, MM3_BTN_DOWN);
    CHECK(S.player.pose == MM3_POSE_SLIDE, "slide pose");
}

/* ----------------------------------------------------- environment moves */

static void test_environment(mm3_style style)
{
    const char *name = mm3_style_name(style);
    int i;
    mm3_fx y0;

    /* swimming: fall into the pool (x = 27..36), stroke upward */
    SETUP(style, k_features);
    mm3_place_player(&S, 31, 12);
    S.player.y = 8 * MM3_TILE_FX; S.player.mode = MM3_MODE_AIR;
    run(60, 0);
    CHECK(S.player.flags & MM3_PF_IN_WATER, "%s: in water", name);
    CHECK(S.player.mode == MM3_MODE_SWIM || S.player.mode == MM3_MODE_GROUND, "%s: swimming mode", name);
    y0 = S.player.y;
    for (i = 0; i < 20; i++) mm3_tick(&S, (i % 8) == 0 ? MM3_BTN_JUMP : 0);
    CHECK(S.player.y < y0, "%s: swim strokes rise (%.1f px)", name, px(y0 - S.player.y));

    /* climbing: vine at x = 44 */
    SETUP(style, k_features);
    mm3_place_player(&S, 44, 10);
    run(3, MM3_BTN_UP);
    CHECK(S.player.mode == MM3_MODE_CLIMB, "%s: grab vine with UP", name);
    y0 = S.player.y;
    run(30, MM3_BTN_UP);
    CHECK(S.player.y < y0 - MM3_PX(8), "%s: climb up (%.1f px)", name, px(y0 - S.player.y));

    /* carrying (crate at x = 52) */
    if (style != MM3_STYLE_RETRO) {
        SETUP(style, k_features);
        mm3_place_player(&S, 51, 10);
        run(3, 0);
        run(3, MM3_BTN_RUN);
        CHECK(S.player.carry >= 0, "%s: pick up crate with RUN", name);
        run(10, MM3_BTN_RUN | MM3_BTN_LEFT);
        mm3_tick(&S, MM3_BTN_LEFT);
        CHECK(S.player.carry < 0 && S.objs[0].vx < 0, "%s: release RUN throws the crate", name);
    }

    /* slopes: walk over the gentle bump and the 45-degree hill */
    SETUP(style, k_features);
    mm3_place_player(&S, 60, 10);
    for (i = 0; i < 600 && S.player.x < 88 * MM3_TILE_FX; i++) {
        mm3_tick(&S, MM3_BTN_RIGHT);
        if (mm3_player_stuck(&S)) break;
    }
    CHECK(S.player.x >= 88 * MM3_TILE_FX - MM3_PX(1) && !mm3_player_stuck(&S),
          "%s: walk over slopes without sinking (x=%.1f tiles)", name, tiles(S.player.x));
}

int main(int argc, char **argv)
{
    int st;
    g_verbose = argc > 1 && strcmp(argv[1], "-v") == 0;
    test_retro();
    test_island();
    test_modern_moves();
    test_athletic_moves();
    for (st = 0; st < MM3_STYLE_COUNT; st++) test_environment((mm3_style)st);
    printf("%d/%d checks passed\n", g_checks - g_fail, g_checks);
    return g_fail ? 1 : 0;
}
