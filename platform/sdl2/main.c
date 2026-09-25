/*
 * main.c - SDL2 frontend (Windows, macOS, Linux, Web via Emscripten,
 * and later Android/iOS). Placeholder rectangles only: no art yet.
 *
 * Controls: Arrows/WASD move, Z/Space/K jump, X/Shift/J run, C/L spin,
 *           1-5 switch physics style, R restart.
 */
#include <stdio.h>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "mm3/sim.h"
#include "../platform.h"

#define VIEW_W 448
#define VIEW_H 256

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    mm3_game_state state;
    mm3_pacer pacer;
    Uint64 last_counter;
    int running;
} app_t;

static app_t g_app;

static const Uint8 k_style_colors[MM3_STYLE_COUNT][3] = {
    {220, 60, 50},  /* Classic */
    {40, 170, 70},  /* New     */
    {50, 110, 220}, /* World   */
    {200, 80, 200}, /* Wonder  */
    {240, 170, 30}  /* Custom  */
};

static void update_title(app_t *a)
{
    char title[96];
    snprintf(title, sizeof(title), "MM3 prototype - physics: %s (1-5 to switch)",
             mm3_style_name(a->state.style));
    SDL_SetWindowTitle(a->window, title);
}

static void restart(app_t *a, mm3_style style)
{
    mm3_init(&a->state, 1u, style);
    mm3_load_demo_level(&a->state);
    update_title(a);
}

static mm3_buttons read_buttons(void)
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
    return b;
}

static void draw(app_t *a)
{
    const mm3_game_state *s = &a->state;
    const mm3_player *pl = &s->player;
    const Uint8 *col = k_style_colors[s->style];
    SDL_Renderer *r = a->renderer;
    int map_w_px = s->map.width * MM3_TILE_PX;
    int px = mm3_fx_to_px(pl->x), py = mm3_fx_to_px(pl->y);
    int pw = mm3_fx_to_px(pl->w), ph = mm3_fx_to_px(pl->h);
    int cam_x = px + pw / 2 - VIEW_W / 2;
    int tx0, tx1, x, y, i;
    SDL_Rect rc;

    if (cam_x > map_w_px - VIEW_W) cam_x = map_w_px - VIEW_W;
    if (cam_x < 0) cam_x = 0;

    SDL_SetRenderDrawColor(r, 120, 190, 240, 255);
    SDL_RenderClear(r);

    tx0 = cam_x / MM3_TILE_PX;
    tx1 = (cam_x + VIEW_W) / MM3_TILE_PX + 1;
    if (tx1 > s->map.width) tx1 = s->map.width;
    for (y = 0; y < s->map.height; y++) {
        for (x = tx0; x < tx1; x++) {
            if (s->map.tiles[y][x] != MM3_TILE_SOLID) continue;
            rc.x = x * MM3_TILE_PX - cam_x; rc.y = y * MM3_TILE_PX;
            rc.w = rc.h = MM3_TILE_PX;
            SDL_SetRenderDrawColor(r, 150, 95, 55, 255);
            SDL_RenderFillRect(r, &rc);
            SDL_SetRenderDrawColor(r, 110, 65, 35, 255);
            SDL_RenderDrawRect(r, &rc);
        }
    }

    /* Player: body in style color, "eye" shows facing. */
    rc.x = px - cam_x; rc.y = py; rc.w = pw; rc.h = ph;
    SDL_SetRenderDrawColor(r, col[0], col[1], col[2], 255);
    SDL_RenderFillRect(r, &rc);
    rc.w = 3; rc.h = 3; rc.y = py + 3;
    rc.x = (pl->flags & MM3_PF_FACING_LEFT) ? px - cam_x + 2 : px - cam_x + pw - 5;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &rc);

    /* HUD: one swatch per style, active one is larger. */
    for (i = 0; i < MM3_STYLE_COUNT; i++) {
        int active = (i == (int)s->style);
        rc.x = 6 + i * 14; rc.y = active ? 4 : 6;
        rc.w = rc.h = active ? 12 : 8;
        SDL_SetRenderDrawColor(r, k_style_colors[i][0], k_style_colors[i][1],
                               k_style_colors[i][2], 255);
        SDL_RenderFillRect(r, &rc);
    }

    SDL_RenderPresent(r);
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
        if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k >= SDLK_1 && k < SDLK_1 + MM3_STYLE_COUNT) restart(a, (mm3_style)(k - SDLK_1));
            if (k == SDLK_r) restart(a, a->state.style);
            if (k == SDLK_ESCAPE) a->running = 0;
        }
    }

    ticks = mm3_pacer_advance(&a->pacer, elapsed_us);
    while (ticks-- > 0) mm3_tick(&a->state, read_buttons());

    draw(a);

#ifdef __EMSCRIPTEN__
    if (!a->running) emscripten_cancel_main_loop();
#endif
}

int main(int argc, char **argv)
{
    app_t *a = &g_app;
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
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
    SDL_RenderSetLogicalSize(a->renderer, VIEW_W, VIEW_H);
    SDL_RenderSetIntegerScale(a->renderer, SDL_TRUE);

    restart(a, MM3_STYLE_CLASSIC);
    a->last_counter = SDL_GetPerformanceCounter();
    a->running = 1;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 0, 1);
#else
    while (a->running) frame();
#endif

    SDL_DestroyRenderer(a->renderer);
    SDL_DestroyWindow(a->window);
    SDL_Quit();
    return 0;
}
