#include <string.h>
#include "mm3/sim.h"

void mm3_init(mm3_game_state *s, uint32_t seed, mm3_style style)
{
    /* Zero everything (including padding) so memcmp/save-states are stable. */
    memset(s, 0, sizeof(*s));
    mm3_rng_seed(&s->rng, seed);
    mm3_set_style(s, style);
    s->player.w = MM3_PX(12);
    s->player.h = MM3_PX(14);
}

void mm3_set_style(mm3_game_state *s, mm3_style style)
{
    s->style = style;
    s->profile = *mm3_builtin_profile(style);
}

void mm3_set_custom_profile(mm3_game_state *s, const mm3_physics_profile *p)
{
    s->style = MM3_STYLE_CUSTOM;
    s->profile = *p;
}

/* '#' = solid, '.' = empty, 'P' = player spawn. */
static const char *const k_demo_level[] = {
    "................................................................",
    "................................................................",
    "................................................................",
    "..........................................#.....#...............",
    "..........................................#.....#...............",
    "..........................................#.....#...............",
    "......................####................#.....#...............",
    "..........................................#.....#.........###...",
    "...............###........................#.....#...............",
    "..........................................#.....#.....###.......",
    ".........................................##.....#...............",
    ".....................#.........#.........##.........###.........",
    "..P.................##.........##........##.....................",
    "...................###.........###.......##.....................",
    "######################....###########################...########",
    "######################....###########################...########"
};

void mm3_load_demo_level(mm3_game_state *s)
{
    int32_t rows = (int32_t)(sizeof(k_demo_level) / sizeof(k_demo_level[0]));
    int32_t cols = (int32_t)strlen(k_demo_level[0]);
    int32_t x, y;

    memset(&s->map, 0, sizeof(s->map));
    s->map.width = cols;
    s->map.height = rows;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < cols; x++) {
            char c = k_demo_level[y][x];
            s->map.tiles[y][x] = (c == '#') ? MM3_TILE_SOLID : MM3_TILE_EMPTY;
            if (c == 'P') {
                s->player.spawn_x = x * MM3_TILE_FX + (MM3_TILE_FX - s->player.w) / 2;
                s->player.spawn_y = (y + 1) * MM3_TILE_FX - s->player.h;
            }
        }
    }
    s->player.x = s->player.spawn_x;
    s->player.y = s->player.spawn_y;
    s->player.vx = s->player.vy = 0;
}

void mm3_tick(mm3_game_state *s, mm3_buttons buttons)
{
    mm3_player_tick(s, buttons);
    s->frame++;
}

/* ---- hashing: feed fields as little-endian bytes, never raw structs ---- */

#define FNV_OFFSET 2166136261u
#define FNV_PRIME  16777619u

static uint32_t h_u32(uint32_t h, uint32_t v)
{
    int i;
    for (i = 0; i < 4; i++) {
        h ^= (v >> (i * 8)) & 0xFFu;
        h *= FNV_PRIME;
    }
    return h;
}

static uint32_t h_i32(uint32_t h, int32_t v) { return h_u32(h, (uint32_t)v); }

static uint32_t h_profile(uint32_t h, const mm3_physics_profile *p)
{
    int i;
    h = h_i32(h, p->walk_max);     h = h_i32(h, p->run_max);
    h = h_i32(h, p->walk_accel);   h = h_i32(h, p->run_accel);
    h = h_i32(h, p->air_accel);    h = h_i32(h, p->friction);
    h = h_i32(h, p->skid_decel);   h = h_i32(h, p->gravity_hold);
    h = h_i32(h, p->gravity_release); h = h_i32(h, p->gravity_fall);
    h = h_i32(h, p->max_fall);
    for (i = 0; i < MM3_JUMP_TIERS; i++) h = h_i32(h, p->jump_vel[i]);
    for (i = 0; i < MM3_JUMP_TIERS - 1; i++) h = h_i32(h, p->jump_tier_speed[i]);
    h = h_i32(h, p->coyote_frames); h = h_i32(h, p->jump_buffer_frames);
    h = h_i32(h, p->wall_slide_max); h = h_i32(h, p->wall_jump_vx);
    h = h_i32(h, p->wall_jump_vy);   h = h_i32(h, p->ground_pound_vy);
    h = h_i32(h, p->spin_jump_vel);  h = h_i32(h, p->spin_fall_max);
    h = h_i32(h, p->long_jump_vx);   h = h_i32(h, p->long_jump_vy);
    h = h_u32(h, p->abilities);
    return h;
}

uint32_t mm3_state_hash(const mm3_game_state *s)
{
    const mm3_player *pl = &s->player;
    uint32_t h = FNV_OFFSET;
    int32_t x, y;

    h = h_u32(h, s->frame);
    h = h_u32(h, s->rng.s);
    h = h_i32(h, (int32_t)s->style);
    h = h_profile(h, &s->profile);

    h = h_i32(h, s->map.width);
    h = h_i32(h, s->map.height);
    for (y = 0; y < s->map.height; y++)
        for (x = 0; x < s->map.width; x++)
            h = h_u32(h, s->map.tiles[y][x]);

    h = h_i32(h, pl->x);  h = h_i32(h, pl->y);
    h = h_i32(h, pl->vx); h = h_i32(h, pl->vy);
    h = h_i32(h, pl->w);  h = h_i32(h, pl->h);
    h = h_i32(h, pl->spawn_x); h = h_i32(h, pl->spawn_y);
    h = h_i32(h, pl->air_max);
    h = h_i32(h, pl->coyote); h = h_i32(h, pl->jump_buffer);
    h = h_i32(h, pl->wall_dir);
    h = h_u32(h, pl->flags);
    h = h_u32(h, pl->prev_buttons);
    return h;
}
