/*
 * test_determinism.c - Proves the simulation is deterministic.
 *
 *  1. Same seed + same inputs, run twice   -> identical hash.
 *  2. Save-state (memcpy) mid-run + resume  -> identical hash.
 *  3. Final hash matches a GOLDEN value     -> identical across compilers,
 *     operating systems, CPUs and WebAssembly (CI runs this everywhere).
 *
 * If you intentionally change gameplay, run `test_determinism --print` and
 * paste the new values into k_golden below (this invalidates old replays).
 */
#include <stdio.h>
#include <string.h>
#include "mm3/sim.h"

#define RUN_FRAMES 3600 /* one minute of gameplay per style */

static const uint32_t k_golden[MM3_STYLE_COUNT] = {
    0x761131E6u /* Retro          */,
    0x36D0566Bu /* Arcade         */,
    0x87042EC4u /* Island         */,
    0xE7293114u /* Modern         */,
    0xFF243BEAu /* Athletic       */,
    0x85ED7B7Au /* Bloom          */,
    0x8C46E05Fu /* Retro Classic  */,
    0x3FB94F26u /* Island Classic */,
    0x98234E65u /* Custom         */
};

/* Deterministic "bot" input: holds a random button combo for 8-40 frames. */
static mm3_buttons scripted_input(mm3_rng *r, uint32_t frame, mm3_buttons *held, uint32_t *until)
{
    if (frame >= *until) {
        uint32_t roll = mm3_rng_range(r, 100);
        mm3_buttons b = 0;
        if (roll < 60) b |= MM3_BTN_RIGHT;
        else if (roll < 80) b |= MM3_BTN_LEFT;
        if (mm3_rng_range(r, 2)) b |= MM3_BTN_RUN;
        if (mm3_rng_range(r, 3) == 0) b |= MM3_BTN_JUMP;
        if (mm3_rng_range(r, 8) == 0) b |= MM3_BTN_UP;
        if (mm3_rng_range(r, 10) == 0) b |= MM3_BTN_DOWN;
        if (mm3_rng_range(r, 12) == 0) b |= MM3_BTN_SPIN;
        *held = b;
        *until = frame + 8 + mm3_rng_range(r, 33);
    }
    /* Tap jump on and off inside long holds so jumps actually repeat. */
    if ((*held & MM3_BTN_JUMP) && (frame % 24) >= 18) return (mm3_buttons)(*held & ~MM3_BTN_JUMP);
    return *held;
}

static int g_stuck_frames;

static uint32_t run(mm3_style style, mm3_game_state *out, int save_at)
{
    static mm3_game_state s, saved;
    mm3_rng input_rng;
    mm3_buttons held = 0;
    uint32_t until = 0, f;

    mm3_init(&s, 12345u, style);
    mm3_load_demo_level(&s);
    mm3_rng_seed(&input_rng, 777u + (uint32_t)style);

    for (f = 0; f < RUN_FRAMES; f++) {
        if (save_at >= 0 && f == (uint32_t)save_at) {
            memcpy(&saved, &s, sizeof(s));
            memcpy(&s, &saved, sizeof(s)); /* round-trip through the save */
        }
        mm3_tick(&s, scripted_input(&input_rng, f, &held, &until));
        if (mm3_player_stuck(&s)) g_stuck_frames++;
    }
    if (out) memcpy(out, &s, sizeof(s));
    return mm3_state_hash(&s);
}

int main(int argc, char **argv)
{
    int print = argc > 1 && strcmp(argv[1], "--print") == 0;
    int failures = 0;
    int st;

    for (st = 0; st < MM3_STYLE_COUNT; st++) {
        static mm3_game_state end;
        uint32_t a = run((mm3_style)st, &end, -1);
        uint32_t b = run((mm3_style)st, NULL, -1);
        uint32_t c = run((mm3_style)st, NULL, RUN_FRAMES / 2);

        if (print) {
            printf("    0x%08Xu, /* %s  x=%ld px */\n", (unsigned)a,
                   mm3_style_name((mm3_style)st), (long)mm3_fx_to_px(end.player.x));
            continue;
        }
        if (a != b) { printf("FAIL %s: rerun hash %08X != %08X\n", mm3_style_name((mm3_style)st), (unsigned)a, (unsigned)b); failures++; }
        if (a != c) { printf("FAIL %s: save-state hash %08X != %08X\n", mm3_style_name((mm3_style)st), (unsigned)a, (unsigned)c); failures++; }
        if (a != k_golden[st]) { printf("FAIL %s: golden hash %08X != expected %08X\n", mm3_style_name((mm3_style)st), (unsigned)a, (unsigned)k_golden[st]); failures++; }
        if (g_stuck_frames) {
            printf("FAIL %s: player inside a wall for %d frames\n", mm3_style_name((mm3_style)st), g_stuck_frames);
            failures++;
            g_stuck_frames = 0;
        }
        if (end.player.x == end.player.spawn_x && end.player.y == end.player.spawn_y) {
            printf("FAIL %s: player never moved\n", mm3_style_name((mm3_style)st));
            failures++;
        }
    }
    if (!print) printf(failures ? "%d failure(s)\n" : "All determinism checks passed.\n", failures);
    return failures ? 1 : 0;
}
