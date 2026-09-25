/*
 * sim.h - Public API of the deterministic simulation.
 *
 * Same initial state + same sequence of mm3_tick() inputs = bit-identical
 * result on every compiler, CPU and platform. mm3_state_hash() lets tests,
 * replays and netplay verify that.
 */
#ifndef MM3_SIM_H
#define MM3_SIM_H

#include <stdint.h>
#include "world.h"

#define MM3_TICKS_PER_SECOND 60

void mm3_init(mm3_game_state *s, uint32_t seed, mm3_style style);

/* Switch style; copies the built-in profile into the state. */
void mm3_set_style(mm3_game_state *s, mm3_style style);

/* Replace the physics with a custom profile (style becomes CUSTOM). */
void mm3_set_custom_profile(mm3_game_state *s, const mm3_physics_profile *p);

/* Small hard-coded test level used by the demo and tests. */
void mm3_load_demo_level(mm3_game_state *s);

/* Advance exactly one 1/60 s step. */
void mm3_tick(mm3_game_state *s, mm3_buttons buttons);

/* Endian-independent FNV-1a hash of every gameplay-relevant field. */
uint32_t mm3_state_hash(const mm3_game_state *s);

/* Internal: player update (player.c). */
void mm3_player_tick(mm3_game_state *s, mm3_buttons buttons);

#endif
