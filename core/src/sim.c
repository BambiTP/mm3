/*
 * sim.c - Tick order, level loading, state hashing.
 */
#include <string.h>
#include "internal.h"

void mm3_init(mm3_game_state *s, uint32_t seed, mm3_style style)
{
    /* Zero everything (including padding) so memcmp/save-states are stable. */
    memset(s, 0, sizeof(*s));
    mm3_rng_seed(&s->rng, seed);
    mm3_set_style(s, style);
    mm3_player_reset(&s->player);
}

void mm3_set_style(mm3_game_state *s, mm3_style style)
{
    if ((int)style < 0 || style >= MM3_STYLE_COUNT) style = MM3_STYLE_RETRO;
    s->style = style;
    s->profile = *mm3_builtin_profile(style);
}

void mm3_set_custom_profile(mm3_game_state *s, const mm3_modern_profile *p)
{
    s->style = MM3_STYLE_CUSTOM;
    s->profile = *p;
}

/*
 * LEVEL KEY
 *   .  empty          #  solid          -  one-way platform
 *   /  slope up 45    \  slope down 45
 *   u  22.5 up (low half)    U  22.5 up (high half)
 *   D  22.5 down (high half) d  22.5 down (low half)
 *   ~  water          |  vine           C  crate (on empty)
 *   P  player start (on empty)
 */
static int32_t tile_for_char(char c)
{
    switch (c) {
    case '#': return MM3_TILE_SOLID;
    case '-': return MM3_TILE_SEMISOLID;
    case '/': return MM3_TILE_SLOPE_UP;
    case '\\': return MM3_TILE_SLOPE_DOWN;
    case 'u': return MM3_TILE_SLOPE_UP_LO;
    case 'U': return MM3_TILE_SLOPE_UP_HI;
    case 'D': return MM3_TILE_SLOPE_DOWN_HI;
    case 'd': return MM3_TILE_SLOPE_DOWN_LO;
    case '~': return MM3_TILE_WATER;
    case '|': return MM3_TILE_VINE;
    default:  return MM3_TILE_EMPTY;
    }
}

void mm3_place_player(mm3_game_state *s, int32_t tile_x, int32_t tile_y_floor)
{
    mm3_player *pl = &s->player;
    pl->spawn_x = tile_x * MM3_TILE_FX + (MM3_TILE_FX - MM3_STAND_W) / 2;
    pl->spawn_y = (tile_y_floor + 1) * MM3_TILE_FX - MM3_STAND_H;
    mm3_player_reset(pl);
}

void mm3_load_ascii_level(mm3_game_state *s, const char *const *rows, int32_t row_count)
{
    int32_t cols = (int32_t)strlen(rows[0]);
    int32_t x, y, i;

    if (cols > MM3_MAP_MAX_W) cols = MM3_MAP_MAX_W;
    if (row_count > MM3_MAP_MAX_H) row_count = MM3_MAP_MAX_H;
    memset(&s->map, 0, sizeof(s->map));
    for (i = 0; i < MM3_MAX_OBJS; i++) memset(&s->objs[i], 0, sizeof(s->objs[i]));
    s->map.width = cols;
    s->map.height = row_count;
    mm3_place_player(s, 1, row_count - 1);
    for (y = 0; y < row_count; y++) {
        for (x = 0; x < cols; x++) {
            char c = rows[y][x];
            s->map.tiles[y][x] = (uint8_t)tile_for_char(c);
            if (c == 'P') mm3_place_player(s, x, y);
            if (c == 'C')
                mm3_object_spawn(s, MM3_OBJ_CRATE, x * MM3_TILE_FX + MM3_PX(1),
                                 (y + 1) * MM3_TILE_FX - MM3_PX(14));
        }
    }
}

static const char *const k_demo_level[] = {
    "...............................................................................................................#",
    "...............................................................................................................#",
    "...............................................................................................................#",
    "...............................................................................................................#",
    "...........................................................................|...................................#",
    "..............................................###...#####..................|...................................#",
    "................................................#...#...................###|####...............................#",
    "................................................#...#......................|...................................#",
    "................................................#...#......................|...................................#",
    "................................................#...#......................|...................................#",
    ".........................................----...#...#......................|...................................#",
    "................................................#...#......................|...................................#",
    "................................................#...#......................|...................................#",
    "...................................-----........#...#......................|...................................#",
    "....................................................#....#~~~~~~~~~~~~~#...|............./\\....................#",
    "........................uU######\\...................#....#~~~~~~~~~~~~~#...|............/##\\.........######....#",
    "..P...................uU#########\\..................#....#~~~~~~~~~~~~~#...|.....C.CC../####\\..................#",
    "##########################################################~~~~~~~~~~~~~########################....#############",
    "##########################################################~~~~~~~~~~~~~########################....#############",
    "###############################################################################################....#############"
};

void mm3_load_demo_level(mm3_game_state *s)
{
    mm3_load_ascii_level(s, k_demo_level,
                         (int32_t)(sizeof(k_demo_level) / sizeof(k_demo_level[0])));
}

int32_t mm3_player_stuck(const mm3_game_state *s)
{
    const mm3_player *pl = &s->player;
    return mm3_box_solid(&s->map, pl->x, pl->y, pl->w, pl->h);
}

void mm3_tick(mm3_game_state *s, mm3_buttons buttons)
{
    mm3_player *pl = &s->player;
    switch (mm3_style_engine((mm3_style)s->style)) {
    case MM3_ENGINE_RETRO:  mm3_engine_retro_tick(s, buttons); break;
    case MM3_ENGINE_ISLAND: mm3_engine_island_tick(s, buttons); break;
    default:                mm3_engine_modern_tick(s, buttons); break;
    }
    mm3_player_update_carry(s);
    mm3_objects_tick(s);
    mm3_player_check_pit(s);
    mm3_player_update_pose(s, buttons);
    pl->prev_buttons = buttons;
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

/* Hash a struct made only of 32-bit fields, value by value. */
static uint32_t h_words(uint32_t h, const void *p, size_t bytes)
{
    const int32_t *w = (const int32_t *)p;
    size_t i;
    for (i = 0; i < bytes / sizeof(int32_t); i++) h = h_u32(h, (uint32_t)w[i]);
    return h;
}

uint32_t mm3_state_hash(const mm3_game_state *s)
{
    uint32_t h = FNV_OFFSET;
    int32_t x, y;

    h = h_u32(h, s->frame);
    h = h_u32(h, s->rng.s);
    h = h_u32(h, (uint32_t)s->style);
    h = h_words(h, &s->profile, sizeof(s->profile));
    h = h_u32(h, (uint32_t)s->map.width);
    h = h_u32(h, (uint32_t)s->map.height);
    for (y = 0; y < s->map.height; y++)
        for (x = 0; x < s->map.width; x++)
            h = h_u32(h, s->map.tiles[y][x]);
    h = h_words(h, &s->player, sizeof(s->player));
    h = h_words(h, s->objs, sizeof(s->objs));
    return h;
}
