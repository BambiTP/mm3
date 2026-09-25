/*
 * collide.c - Tile collision shared by every engine and object.
 *
 * Solid tiles block from every side. Semisolid tiles and slopes only carry
 * things standing on them; slopes are resolved at the center of the feet.
 */
#include "internal.h"

#define T MM3_TILE_FX
#define STEP_UP   MM3_PX(10)  /* max ledge height walked over without jumping */
#define STICK_DOWN MM3_PX(12) /* max drop followed while walking downhill     */

int32_t mm3_tile_at(const mm3_tilemap *m, int32_t tx, int32_t ty)
{
    if (tx < 0 || tx >= m->width) return MM3_TILE_SOLID;   /* side walls */
    if (ty < 0 || ty >= m->height) return MM3_TILE_EMPTY;  /* sky / pits */
    return m->tiles[ty][tx];
}

int32_t mm3_is_slope(int32_t tile)
{
    return tile >= MM3_TILE_SLOPE_UP && tile <= MM3_TILE_SLOPE_DOWN_LO;
}

int32_t mm3_tile_at_point(const mm3_tilemap *m, mm3_fx x, mm3_fx y)
{
    return mm3_tile_at(m, mm3_floor_div(x, T), mm3_floor_div(y, T));
}

int32_t mm3_box_solid(const mm3_tilemap *m, mm3_fx x, mm3_fx y, mm3_fx w, mm3_fx h)
{
    int32_t tx0 = mm3_floor_div(x, T), tx1 = mm3_floor_div(x + w - 1, T);
    int32_t ty0 = mm3_floor_div(y, T), ty1 = mm3_floor_div(y + h - 1, T);
    int32_t tx, ty;
    for (ty = ty0; ty <= ty1; ty++)
        for (tx = tx0; tx <= tx1; tx++)
            /* A solid directly under a slope is the slope's filling: its
               surface is the slope, so it never acts as a wall. */
            if (mm3_tile_at(m, tx, ty) == MM3_TILE_SOLID &&
                !mm3_is_slope(mm3_tile_at(m, tx, ty - 1))) return 1;
    return 0;
}

/* Height of a slope's surface above the tile bottom at local x (0..T-1). */
static mm3_fx slope_height(int32_t tile, mm3_fx lx)
{
    switch (tile) {
    case MM3_TILE_SLOPE_UP:      return lx;
    case MM3_TILE_SLOPE_DOWN:    return T - lx;
    case MM3_TILE_SLOPE_UP_LO:   return lx / 2;
    case MM3_TILE_SLOPE_UP_HI:   return T / 2 + lx / 2;
    case MM3_TILE_SLOPE_DOWN_HI: return T - lx / 2;
    case MM3_TILE_SLOPE_DOWN_LO: return T / 2 - lx / 2;
    default:                     return 0;
    }
}

static int32_t slope_steepness(int32_t tile)
{
    switch (tile) {
    case MM3_TILE_SLOPE_UP:      return 2;
    case MM3_TILE_SLOPE_DOWN:    return -2;
    case MM3_TILE_SLOPE_UP_LO:
    case MM3_TILE_SLOPE_UP_HI:   return 1;
    case MM3_TILE_SLOPE_DOWN_HI:
    case MM3_TILE_SLOPE_DOWN_LO: return -1;
    default:                     return 0;
    }
}

/*
 * Highest standable surface for feet at `bottom` (and previous bottom
 * `prev_bottom`), searching from `bottom - up_range` to `bottom + down_range`.
 * Returns 1 and fills *surf / *steep when found.
 */
static int32_t find_ground(const mm3_tilemap *m, mm3_fx x, mm3_fx w, mm3_fx prev_bottom,
                           mm3_fx bottom, mm3_fx down_range, mm3_fx *surf, int32_t *steep)
{
    int32_t found = 0;
    int32_t tx0 = mm3_floor_div(x, T), tx1 = mm3_floor_div(x + w - 1, T);
    int32_t ty0 = mm3_floor_div(bottom - T, T), ty1 = mm3_floor_div(bottom + down_range, T);
    mm3_fx fx = x + w / 2;
    int32_t ftx = mm3_floor_div(fx, T);
    int32_t tx, ty;
    mm3_fx best = 0;
    int32_t best_steep = 0;

    /* Slopes: sampled at the center of the feet. Flat tops under any part
       of the box also count; the highest surface wins so the box never
       sinks into a plateau next to a slope. */
    for (ty = ty0; ty <= ty1; ty++) {
        int32_t tile = mm3_tile_at(m, ftx, ty);
        if (mm3_is_slope(tile)) {
            mm3_fx s = (ty + 1) * T - slope_height(tile, mm3_floor_mod(fx, T));
            if (s >= bottom - T && s <= bottom + down_range && (!found || s < best)) {
                best = s; best_steep = slope_steepness(tile); found = 1;
            }
        }
    }
    /* Flat tops: solid and semisolid tiles under the box. */
    for (ty = ty0; ty <= ty1; ty++) {
        mm3_fx top = ty * T;
        if (top < prev_bottom - MM3_PX(2)) continue;  /* was already below it */
        if (top > bottom + down_range) continue;
        for (tx = tx0; tx <= tx1; tx++) {
            int32_t tile = mm3_tile_at(m, tx, ty);
            int32_t ok = 0;
            if (tile == MM3_TILE_SOLID)
                ok = mm3_tile_at(m, tx, ty - 1) != MM3_TILE_SOLID; /* only real tops */
            else if (tile == MM3_TILE_SEMISOLID)
                ok = prev_bottom <= top;
            if (ok && (!found || top < best)) { best = top; best_steep = 0; found = 1; }
        }
    }
    if (found) { *surf = best; *steep = best_steep; }
    return found;
}

void mm3_move_box(const mm3_tilemap *m, mm3_fx *x, mm3_fx *y, mm3_fx w, mm3_fx h,
                  mm3_fx dx, mm3_fx dy, int32_t grounded_before, mm3_hit *out)
{
    mm3_fx prev_bottom;
    out->ground = out->head = out->wall_l = out->wall_r = 0;
    out->slope = 0;

    /* Never move a full tile in one step: no tunneling. */
    dx = mm3_clamp(dx, -(T - 1), T - 1);
    dy = mm3_clamp(dy, -(T - 1), T - 1);

    /* ---- X ---- */
    if (dx != 0) {
        *x += dx;
        if (mm3_box_solid(m, *x, *y, w, h)) {
            mm3_fx up;
            int32_t stepped = 0;
            if (grounded_before) {
                for (up = MM3_PX(1); up <= STEP_UP; up += MM3_PX(1)) {
                    if (!mm3_box_solid(m, *x, *y - up, w, h)) { *y -= up; stepped = 1; break; }
                }
            }
            if (!stepped) {
                if (dx > 0) { *x = mm3_floor_div(*x + w - 1, T) * T - w; out->wall_r = 1; }
                else        { *x = (mm3_floor_div(*x, T) + 1) * T;      out->wall_l = 1; }
            }
        }
    }

    /* ---- Y ---- */
    prev_bottom = *y + h;
    *y += dy;
    if (dy < 0) {
        if (mm3_box_solid(m, *x, *y, w, h)) {
            *y = (mm3_floor_div(*y, T) + 1) * T;
            out->head = 1;
        }
    } else {
        mm3_fx surf;
        int32_t steep;
        mm3_fx reach = grounded_before ? STICK_DOWN : 0;
        if (find_ground(m, *x, w, prev_bottom, *y + h, reach, &surf, &steep)) {
            if (*y + h >= surf || grounded_before) {
                *y = surf - h;
                out->ground = 1;
                out->slope = steep;
            }
        }
        /* Safety: never end inside a solid tile vertically. */
        if (!out->ground && mm3_box_solid(m, *x, *y, w, h)) {
            *y = mm3_floor_div(*y + h - 1, T) * T - h;
            out->ground = 1;
        }
    }

    /* Wall contact (for wall slides) even without horizontal motion. */
    if (mm3_box_solid(m, *x - 1, *y, 1, h - MM3_PX(2))) out->wall_l = 1;
    if (mm3_box_solid(m, *x + w, *y, 1, h - MM3_PX(2))) out->wall_r = 1;
}
