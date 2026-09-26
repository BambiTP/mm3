/*
 * main.c - SDL2 frontend (Windows, macOS, Linux, Web via Emscripten,
 * and later Android/iOS). Art is procedural placeholder (see art.c).
 *
 * Keyboard: Arrows/WASD move, Z/Space/K jump, X/Shift/J run & carry,
 *           C/L spin, 1-9 physics style, R restart, Esc quit.
 * Gamepad:  D-pad/stick move, A jump, B spin, X/Y run & carry.
 */
#include <stdio.h>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "mm3/sim.h"
#include "../platform.h"
#include "art.h"

#define VIEW_W 448
#define VIEW_H 256

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_GameController *pad;
    mm3_game_state state;
    mm3_pacer pacer;
    Uint64 last_counter;
    int cam_x, cam_y;
    int running;
} app_t;

static app_t g_app;

static const char *const k_hints[MM3_STYLE_COUNT] = {
    "Z JUMP  X RUN  WALL JUMP  DOWN SLIDE  UP VINE  TOUCH CRATE: KICK",
    "Z JUMP  X RUN / HOLD CRATE  WALL JUMP  DOWN SLIDE  UP VINE",
    "Z JUMP  C SPIN  X HOLD CRATE (UP+RELEASE: TOSS UP)  WALL JUMP",
    "Z JUMP X3  C SPIN/TWIRL  DOWN POUND  WALL JUMP  X CARRY",
    "X+C LONG JUMP  HOLD DOWN 1S+Z BACKFLIP  DOWN+C ROLL  POUND",
    "Z JUMP  C SPIN/TWIRL  DOWN POUND  WALL JUMP  X CARRY",
    "ORIGINAL 8-BIT ENGINE  Z JUMP  X RUN  NO AIR TURN",
    "ORIGINAL 16-BIT ENGINE  Z JUMP  C SPIN  X RUN/CARRY",
    "EVERY MOVE FROM EVERY STYLE"
};

static void update_title(app_t *a)
{
    char title[96];
    snprintf(title, sizeof(title), "MM3 prototype - %s physics (1-6 to switch)",
             mm3_style_name((mm3_style)a->state.style));
    SDL_SetWindowTitle(a->window, title);
}

static void restart(app_t *a, mm3_style style)
{
    mm3_init(&a->state, 1u, style);
    mm3_load_demo_level(&a->state);
    update_title(a);
}

static mm3_buttons read_buttons(app_t *a)
{
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    mm3_buttons b = 0;
    if (k[SDL_SCANCODE_LEFT] || k[SDL_SCANCODE_A]) b |= MM3_BTN_LEFT;
    if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) b |= MM3_BTN_RIGHT;
    if (k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_W]) b |= MM3_BTN_UP;
    if (k[SDL_SCANCODE_DOWN] || k[SDL_SCANCODE_S]) b |= MM3_BTN_DOWN;
    if (k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_SPACE] || k[SDL_SCANCODE_K]) b |= MM3_BTN_JUMP;
    if (k[SDL_SCANCODE_X] || k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_J]) b |= MM3_BTN_RUN;
    if (k[SDL_SCANCODE_C] || k[SDL_SCANCODE_L]) b |= MM3_BTN_SPIN;
    if (k[SDL_SCANCODE_RETURN]) b |= MM3_BTN_START;
    if (a->pad) {
        SDL_GameController *p = a->pad;
        Sint16 ax = SDL_GameControllerGetAxis(p, SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 ay = SDL_GameControllerGetAxis(p, SDL_CONTROLLER_AXIS_LEFTY);
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || ax < -16000) b |= MM3_BTN_LEFT;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || ax > 16000) b |= MM3_BTN_RIGHT;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_UP) || ay < -16000) b |= MM3_BTN_UP;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || ay > 16000) b |= MM3_BTN_DOWN;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_A)) b |= MM3_BTN_JUMP;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_B)) b |= MM3_BTN_SPIN;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_X) ||
            SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_Y)) b |= MM3_BTN_RUN;
        if (SDL_GameControllerGetButton(p, SDL_CONTROLLER_BUTTON_START)) b |= MM3_BTN_START;
    }
    return b;
}

static void update_camera(app_t *a)
{
    const mm3_game_state *s = &a->state;
    int px = mm3_fx_to_px(s->player.x) + 6, py = mm3_fx_to_px(s->player.y) + 13;
    int map_w = s->map.width * MM3_TILE_PX, map_h = s->map.height * MM3_TILE_PX;
    int tx = px - VIEW_W / 2, ty = py - VIEW_H / 2 - 16;
    /* horizontal: follow tightly; vertical: ease */
    a->cam_x = tx;
    a->cam_y += (ty - a->cam_y) / 6;
    if (a->cam_x > map_w - VIEW_W) a->cam_x = map_w - VIEW_W;
    if (a->cam_x < 0) a->cam_x = 0;
    if (a->cam_y > map_h - VIEW_H) a->cam_y = map_h - VIEW_H;
    if (a->cam_y < 0) a->cam_y = 0;
}

static void draw_hud(app_t *a)
{
    SDL_Renderer *r = a->renderer;
    const mm3_game_state *s = &a->state;
    int i, x = 6;
    SDL_Rect bar;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    bar.x = 0; bar.y = 0; bar.w = VIEW_W; bar.h = 22;
    SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
    SDL_RenderFillRect(r, &bar);
    {
        char label[48];
        int w;
        SDL_Rect hl;
        snprintf(label, sizeof(label), "%d %s", s->style + 1, mm3_style_name((mm3_style)s->style));
        w = art_text_width(label);
        hl.x = x - 2; hl.y = 2; hl.w = w + 3; hl.h = 10;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 230);
        SDL_RenderFillRect(r, &hl);
        art_text(r, label, x, 3, 20, 20, 30);
        art_text(r, "PRESS 1-9 TO CHANGE STYLE  R RESTART", x + w + 10, 3, 200, 200, 215);
    }
    art_text(r, k_hints[s->style], 6, 13, 255, 240, 170);

    /* P-meter (Island) or dash meter (styles with a dash) */
    if (s->style == MM3_STYLE_ISLAND_CLASSIC ||
        ((s->profile.moves & (MM3_MOVE_DASH | MM3_MOVE_PMETER)) && s->style != MM3_STYLE_RETRO_CLASSIC)) {
        int filled, total = 6;
        int pm = s->style == MM3_STYLE_ISLAND_CLASSIC || (s->profile.moves & MM3_MOVE_PMETER);
        if (s->style == MM3_STYLE_ISLAND_CLASSIC) filled = s->player.pmeter * total / 0x70;
        else if (pm) filled = s->profile.pmeter_frames ? s->player.pmeter * total / s->profile.pmeter_frames : 0;
        else filled = s->profile.dash_frames ? s->player.run_timer * total / s->profile.dash_frames : 0;
        for (i = 0; i < total; i++) {
            SDL_Rect seg;
            seg.x = 6 + i * 7; seg.y = VIEW_H - 12; seg.w = 5; seg.h = 6;
            if (i < filled) SDL_SetRenderDrawColor(r, 255, 210, 60, 255);
            else SDL_SetRenderDrawColor(r, 60, 60, 70, 200);
            SDL_RenderFillRect(r, &seg);
        }
        art_text(r, pm ? "P" : "DASH", 6 + total * 7 + 2, VIEW_H - 13,
                 filled >= total ? 255 : 150, filled >= total ? 220 : 150, filled >= total ? 60 : 160);
    }
}

static void draw(app_t *a)
{
    const mm3_game_state *s = &a->state;
    const mm3_player *pl = &s->player;
    SDL_Renderer *r = a->renderer;
    int style = s->style;
    int tx0, tx1, ty0, ty1, x, y, i;

    update_camera(a);
    art_draw_background(r, style, a->cam_x, a->cam_y);

    tx0 = a->cam_x / MM3_TILE_PX;
    tx1 = (a->cam_x + VIEW_W) / MM3_TILE_PX + 1;
    ty0 = a->cam_y / MM3_TILE_PX;
    ty1 = (a->cam_y + VIEW_H) / MM3_TILE_PX + 1;
    if (tx1 > s->map.width) tx1 = s->map.width;
    if (ty1 > s->map.height) ty1 = s->map.height;

    /* back layer: everything except water (water is drawn over the player) */
    for (y = ty0; y < ty1; y++)
        for (x = tx0; x < tx1; x++) {
            int t = s->map.tiles[y][x];
            int above = y > 0 ? s->map.tiles[y - 1][x] : MM3_TILE_EMPTY;
            if (t == MM3_TILE_EMPTY || t == MM3_TILE_WATER) continue;
            /* grass only where the top is exposed (not under a slope) */
            art_draw_tile(r, style, t, above != MM3_TILE_SOLID &&
                          !(above >= MM3_TILE_SLOPE_UP && above <= MM3_TILE_SLOPE_DOWN_LO),
                          x * MM3_TILE_PX - a->cam_x, y * MM3_TILE_PX - a->cam_y);
        }

    for (i = 0; i < MM3_MAX_OBJS; i++) {
        const mm3_object *o = &s->objs[i];
        if (o->type != MM3_OBJ_CRATE || o->state == MM3_OBJS_CARRIED) continue;
        art_draw_crate(r, style, mm3_fx_to_px(o->x) - a->cam_x, mm3_fx_to_px(o->y) - a->cam_y);
    }

    art_draw_player(r, style, pl->pose, pl->anim, pl->carry >= 0,
                    (pl->flags & MM3_PF_FACING_LEFT) != 0,
                    mm3_fx_to_px(pl->x) - 4 - a->cam_x,
                    mm3_fx_to_px(pl->y + pl->h) - ART_SPRITE_H - a->cam_y);

    if (pl->carry >= 0) {
        const mm3_object *o = &s->objs[pl->carry];
        art_draw_crate(r, style, mm3_fx_to_px(o->x) - a->cam_x, mm3_fx_to_px(o->y) - a->cam_y);
    }

    for (y = ty0; y < ty1; y++)
        for (x = tx0; x < tx1; x++) {
            int above = y > 0 ? s->map.tiles[y - 1][x] : MM3_TILE_EMPTY;
            if (s->map.tiles[y][x] != MM3_TILE_WATER) continue;
            art_draw_tile(r, style, MM3_TILE_WATER, above != MM3_TILE_WATER,
                          x * MM3_TILE_PX - a->cam_x, y * MM3_TILE_PX - a->cam_y);
        }

    draw_hud(a);
    SDL_RenderPresent(r);
}

static void open_pad(app_t *a)
{
    int i;
    if (a->pad) return;
    for (i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) { a->pad = SDL_GameControllerOpen(i); if (a->pad) return; }
    }
}

static void frame(void)
{
    app_t *a = &g_app;
    SDL_Event e;
    Uint64 now = SDL_GetPerformanceCounter();
    int64_t elapsed_us = (int64_t)((now - a->last_counter) * 1000000u / SDL_GetPerformanceFrequency());
    int ticks;

    a->last_counter = now;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) a->running = 0;
        if (e.type == SDL_CONTROLLERDEVICEADDED) open_pad(a);
        if (e.type == SDL_CONTROLLERDEVICEREMOVED && a->pad) {
            SDL_GameControllerClose(a->pad);
            a->pad = NULL;
            open_pad(a);
        }
        if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k >= SDLK_1 && k < SDLK_1 + MM3_STYLE_COUNT) restart(a, (mm3_style)(k - SDLK_1));
            if (k == SDLK_r) restart(a, (mm3_style)a->state.style);
            if (k == SDLK_ESCAPE) a->running = 0;
        }
        if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK)
            restart(a, (mm3_style)((a->state.style + 1) % MM3_STYLE_COUNT));
    }

    ticks = mm3_pacer_advance(&a->pacer, elapsed_us);
    while (ticks-- > 0) mm3_tick(&a->state, read_buttons(a));

    draw(a);

#ifdef __EMSCRIPTEN__
    if (!a->running) emscripten_cancel_main_loop();
#endif
}

int main(int argc, char **argv)
{
    app_t *a = &g_app;
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
#ifdef __EMSCRIPTEN__
    /* On the web the page's CSS scales the canvas; a resizable SDL window
       would instead track the canvas' CSS size. */
    a->window = SDL_CreateWindow("MM3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 VIEW_W * 3, VIEW_H * 3, 0);
#else
    a->window = SDL_CreateWindow("MM3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 VIEW_W * 3, VIEW_H * 3, SDL_WINDOW_RESIZABLE);
#endif
    a->renderer = SDL_CreateRenderer(a->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!a->renderer) a->renderer = SDL_CreateRenderer(a->window, -1, 0);
    if (!a->window || !a->renderer) {
        fprintf(stderr, "SDL window/renderer failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RenderSetLogicalSize(a->renderer, VIEW_W, VIEW_H);
    SDL_RenderSetIntegerScale(a->renderer, SDL_TRUE);
    art_init(a->renderer);
    open_pad(a);

    restart(a, MM3_STYLE_RETRO);
    a->last_counter = SDL_GetPerformanceCounter();
    a->running = 1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (a->running) frame();
#endif

    art_free();
    if (a->pad) SDL_GameControllerClose(a->pad);
    SDL_DestroyRenderer(a->renderer);
    SDL_DestroyWindow(a->window);
    SDL_Quit();
    return 0;
}
