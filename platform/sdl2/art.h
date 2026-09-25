/*
 * art.h - Procedurally drawn placeholder art for each physics style.
 *
 * Everything is generated at startup from simple shapes, so the game ships
 * without any image files. Replace with hand-made sprites later; the rest
 * of the frontend only uses the functions below.
 */
#ifndef MM3_ART_H
#define MM3_ART_H

#include <SDL.h>
#include "mm3/sim.h"

#define ART_SPRITE_W 20   /* logical pixels */
#define ART_SPRITE_H 30

int  art_init(SDL_Renderer *r);
void art_free(void);

/* Player sprite for a pose; anim picks the frame. */
void art_draw_player(SDL_Renderer *r, int style, int pose, int anim, int carrying,
                     int facing_left, int x, int y);
/* One map tile (tile type from mm3_tile). top = no solid tile above it. */
void art_draw_tile(SDL_Renderer *r, int style, int tile, int top, int x, int y);
void art_draw_crate(SDL_Renderer *r, int style, int x, int y);
void art_draw_background(SDL_Renderer *r, int style, int cam_x, int cam_y);

/* 5x7 pixel font. Returns the width drawn. */
int  art_text(SDL_Renderer *r, const char *s, int x, int y, Uint8 cr, Uint8 cg, Uint8 cb);
int  art_text_width(const char *s);

#endif
