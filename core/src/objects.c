/*
 * objects.c - Throwable crates. Updated in ascending slot order every tick.
 */
#include "internal.h"

#define CRATE_GRAVITY  MM3_MILLI(250)
#define CRATE_MAX_FALL MM3_PX(4)
#define CRATE_FRICTION MM3_MILLI(80)

int32_t mm3_object_spawn(mm3_game_state *s, int32_t type, mm3_fx x, mm3_fx y)
{
    int32_t i;
    for (i = 0; i < MM3_MAX_OBJS; i++) {
        mm3_object *o = &s->objs[i];
        if (o->type != MM3_OBJ_NONE) continue;
        o->type = type;
        o->state = MM3_OBJS_REST;
        o->w = MM3_PX(14);
        o->h = MM3_PX(14);
        o->x = x;
        o->y = y;
        o->vx = o->vy = 0;
        o->on_ground = 0;
        return i;
    }
    return -1;
}

void mm3_objects_tick(mm3_game_state *s)
{
    int32_t i;
    for (i = 0; i < MM3_MAX_OBJS; i++) {
        mm3_object *o = &s->objs[i];
        mm3_hit hit;
        if (o->type == MM3_OBJ_NONE || o->state == MM3_OBJS_CARRIED) continue;
        o->vy = mm3_min(o->vy + CRATE_GRAVITY, CRATE_MAX_FALL);
        if (mm3_tile_at_point(&s->map, o->x + o->w / 2, o->y + o->h / 2) == MM3_TILE_WATER) {
            o->vy = mm3_min(o->vy, MM3_MILLI(600));   /* sinks slowly */
            o->vx = mm3_approach(o->vx, 0, CRATE_FRICTION / 2);
        }
        mm3_move_box(&s->map, &o->x, &o->y, o->w, o->h, o->vx, o->vy, o->on_ground, &hit);
        o->on_ground = hit.ground;
        if (hit.head && o->vy < 0) o->vy = 0;
        if ((hit.blocked_l && o->vx < 0) || (hit.blocked_r && o->vx > 0)) o->vx = -o->vx / 2;
        if (hit.ground) {
            o->vy = 0;
            o->vx = mm3_approach(o->vx, 0, CRATE_FRICTION);
            if (o->vx == 0) o->state = MM3_OBJS_REST;
        }
        if (o->y > (s->map.height + 3) * MM3_TILE_FX) o->type = MM3_OBJ_NONE; /* fell in a pit */
    }
}
