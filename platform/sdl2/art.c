/*
 * art.c - Procedural placeholder art: one generic hero and one tileset per
 * physics style, each drawn with a different treatment (resolution,
 * palette, outline, shading, squash). All original; no image files.
 *
 * The frontend may use floats freely: nothing here touches the simulation.
 */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "art.h"

#define PI_F 3.14159265f
#define N_STYLES MM3_STYLE_COUNT
#define MAX_FRAMES 4

typedef struct { float x, y; } V2;

typedef struct {
    V2 head; float head_r;
    V2 shoulder, hip;
    V2 hand_b, hand_f, foot_b, foot_f;
    float rot;          /* radians around the body pivot */
    float sx, sy;       /* squash and stretch around the feet */
    int back_view;      /* seen from behind: no face */
    int eyes_up;
} Rig;

typedef struct { Uint8 r, g, b; } RGB;

typedef struct {
    int res;            /* texture pixels per logical pixel */
    int outline;        /* outline thickness in texture pixels */
    int shade;          /* 0 flat, 1 two-tone, 2 smooth */
    int big_eyes;
    int squash;
    RGB skin, hair, shirt, pants, shoes, band, eye, line, eye_white;
    /* tiles */
    RGB ground, ground2, grass, grass2, plank, plank2, water, vine, crate, crate2;
    RGB sky_top, sky_bottom, hill_far, hill_near;
} Style;

static const Style k_styles[N_STYLES] = {
    /* RETRO: 1x resolution, 4-colour look, no outline, flat */
    { 1, 0, 0, 0, 0,
      {248, 184, 120}, {80, 48, 0}, {0, 168, 160}, {80, 48, 0}, {80, 48, 0}, {248, 120, 0},
      {24, 16, 0}, {0, 0, 0}, {255, 255, 255},
      {140, 90, 43}, {176, 122, 60}, {88, 168, 56}, {44, 120, 24}, {216, 160, 96}, {150, 96, 40},
      {48, 80, 192}, {48, 160, 48}, {160, 96, 48}, {96, 48, 16},
      {107, 164, 248}, {107, 164, 248}, {60, 150, 60}, {40, 120, 40} },
    /* ARCADE: 1x resolution, crisp outline, flat saturated colours, checkered ground */
    { 1, 1, 0, 0, 0,
      {252, 188, 148}, {48, 28, 16}, {240, 150, 30}, {50, 60, 72}, {30, 20, 20}, {40, 170, 230},
      {10, 10, 10}, {16, 16, 16}, {255, 255, 255},
      {228, 180, 110}, {196, 140, 80}, {100, 200, 90}, {40, 120, 40}, {220, 150, 90}, {140, 80, 40},
      {60, 120, 230}, {40, 160, 60}, {200, 130, 70}, {110, 60, 30},
      {160, 224, 248}, {200, 240, 255}, {90, 190, 110}, {60, 160, 90} },
    /* ISLAND: 1x resolution, dark outline, two-tone shading */
    { 1, 1, 1, 0, 0,
      {248, 200, 160}, {107, 58, 30}, {42, 179, 166}, {46, 58, 120}, {90, 46, 20}, {255, 140, 26},
      {16, 16, 24}, {26, 16, 32}, {255, 255, 255},
      {168, 104, 58}, {140, 84, 36}, {96, 200, 80}, {30, 90, 24}, {232, 176, 104}, {150, 96, 50},
      {40, 104, 208}, {40, 150, 60}, {176, 112, 58}, {106, 62, 30},
      {120, 192, 248}, {200, 232, 255}, {96, 176, 120}, {64, 152, 88} },
    /* MODERN: 2x resolution, thin outline, smooth shading */
    { 2, 1, 2, 0, 0,
      {255, 210, 176}, {110, 64, 36}, {51, 196, 181}, {52, 66, 140}, {84, 44, 24}, {255, 150, 40},
      {20, 20, 32}, {40, 30, 50}, {255, 255, 255},
      {154, 106, 63}, {123, 82, 44}, {108, 203, 74}, {168, 240, 122}, {236, 190, 120}, {170, 116, 64},
      {46, 111, 216}, {60, 170, 70}, {186, 122, 64}, {112, 66, 32},
      {130, 200, 255}, {216, 240, 255}, {150, 205, 170}, {110, 185, 130} },
    /* ATHLETIC: 2x resolution, thick outline, bold two-tone */
    { 2, 2, 1, 0, 0,
      {255, 204, 160}, {90, 40, 20}, {16, 208, 192}, {48, 64, 192}, {60, 30, 20}, {255, 106, 0},
      {16, 16, 24}, {16, 16, 24}, {255, 255, 255},
      {242, 177, 52}, {183, 122, 16}, {59, 209, 111}, {255, 224, 138}, {255, 255, 255}, {200, 200, 210},
      {30, 120, 230}, {40, 180, 80}, {200, 120, 50}, {110, 60, 20},
      {126, 200, 255}, {230, 246, 255}, {255, 255, 255}, {220, 236, 250} },
    /* BLOOM: 2x resolution, bright, big eyes, squash and stretch */
    { 2, 1, 2, 1, 1,
      {255, 210, 176}, {120, 70, 40}, {62, 224, 200}, {91, 91, 214}, {100, 50, 30}, {255, 159, 28},
      {27, 27, 58}, {60, 40, 90}, {255, 255, 255},
      {155, 110, 208}, {184, 146, 230}, {126, 224, 106}, {255, 111, 181}, {255, 214, 140}, {220, 150, 90},
      {60, 140, 240}, {70, 200, 110}, {230, 140, 80}, {140, 70, 40},
      {255, 199, 230}, {184, 224, 255}, {200, 150, 230}, {160, 120, 210} },
    /* RETRO CLASSIC: Retro's treatment, dusk palette */
    { 1, 0, 0, 0, 0,
      {248, 184, 120}, {80, 48, 0}, {128, 88, 208}, {40, 24, 72}, {40, 24, 72}, {248, 200, 0},
      {24, 16, 0}, {0, 0, 0}, {255, 255, 255},
      {120, 72, 40}, {160, 104, 56}, {72, 144, 72}, {32, 96, 40}, {200, 144, 88}, {136, 80, 32},
      {40, 64, 176}, {48, 144, 64}, {144, 88, 40}, {88, 40, 16},
      {72, 96, 200}, {72, 96, 200}, {48, 112, 96}, {32, 88, 72} },
    /* ISLAND CLASSIC: Island's treatment, sunset palette */
    { 1, 1, 1, 0, 0,
      {248, 200, 160}, {107, 58, 30}, {128, 88, 208}, {40, 40, 96}, {90, 46, 20}, {255, 200, 40},
      {16, 16, 24}, {26, 16, 32}, {255, 255, 255},
      {176, 96, 64}, {144, 72, 44}, {120, 192, 72}, {40, 88, 24}, {232, 176, 104}, {150, 96, 50},
      {48, 96, 200}, {40, 150, 60}, {176, 112, 58}, {106, 62, 30},
      {248, 168, 120}, {255, 224, 176}, {176, 120, 120}, {140, 96, 104} },
    /* CUSTOM: Modern treatment, magenta and gold, stone tiles */
    { 2, 1, 2, 0, 0,
      {255, 210, 176}, {40, 30, 40}, {224, 80, 154}, {58, 46, 106}, {30, 24, 40}, {245, 197, 24},
      {20, 20, 32}, {30, 24, 40}, {255, 255, 255},
      {124, 132, 148}, {92, 100, 116}, {160, 168, 184}, {210, 214, 226}, {190, 196, 210}, {120, 126, 140},
      {60, 120, 220}, {90, 170, 110}, {170, 120, 80}, {90, 60, 40},
      {32, 40, 56}, {70, 84, 110}, {50, 60, 84}, {40, 48, 70} }
};

/* ------------------------------------------------------------------ poses */

static const int k_frames[MM3_POSE_COUNT] = {
    /* IDLE WALK RUN SPRINT SKID CROUCH LOOK JUMP FALL SPIN TWIRL TRIPLE LONG BACK SIDE WALL POUND SLIDE SWIM CLIMB THROW */
    2, 4, 4, 4, 1, 1, 1, 1, 1, 4, 4, 4, 1, 4, 4, 1, 2, 1, 4, 2, 1
};
static const int k_period[MM3_POSE_COUNT] = {
    30, 7, 6, 6, 1, 1, 1, 1, 1, 3, 3, 4, 1, 5, 5, 1, 16, 1, 9, 10, 1
};

static V2 v2(float x, float y) { V2 v; v.x = x; v.y = y; return v; }

static Rig base_rig(void)
{
    Rig g;
    memset(&g, 0, sizeof(g));
    g.head = v2(10.5f, 8.0f); g.head_r = 5.2f;
    g.shoulder = v2(10.5f, 14.5f); g.hip = v2(10.5f, 20.5f);
    g.hand_b = v2(8.0f, 20.5f); g.hand_f = v2(13.0f, 20.5f);
    g.foot_b = v2(8.8f, 29.0f); g.foot_f = v2(12.2f, 29.0f);
    g.sx = g.sy = 1.0f;
    return g;
}

static void tuck(Rig *g)
{
    g->head = v2(10.5f, 12.0f);
    g->shoulder = v2(10.5f, 16.5f); g->hip = v2(10.5f, 20.5f);
    g->hand_f = v2(13.0f, 19.5f); g->hand_b = v2(8.5f, 19.5f);
    g->foot_f = v2(12.5f, 23.5f); g->foot_b = v2(9.0f, 23.5f);
}

static Rig pose_rig(int pose, int frame, int carrying, int squash)
{
    Rig g = base_rig();
    int n = k_frames[pose];
    float t = (float)frame / (float)n;
    float s = sinf(t * 2.0f * PI_F), c = cosf(t * 2.0f * PI_F);
    switch (pose) {
    case MM3_POSE_IDLE:
        g.head.y += frame ? 0.4f : 0.0f;
        g.shoulder.y += frame ? 0.3f : 0.0f;
        break;
    case MM3_POSE_WALK:
        g.foot_f = v2(10.5f + 2.8f * s, 29.0f - 1.2f * (c < 0 ? -c : 0));
        g.foot_b = v2(10.5f - 2.8f * s, 29.0f - 1.2f * (c > 0 ? c : 0));
        g.hand_f = v2(10.5f - 2.6f * s, 20.0f);
        g.hand_b = v2(10.5f + 2.6f * s, 20.0f);
        g.head.y += (c > 0.7f || c < -0.7f) ? 0.4f : 0.0f;
        break;
    case MM3_POSE_RUN:
    case MM3_POSE_SPRINT:
        g.rot = pose == MM3_POSE_SPRINT ? 0.22f : 0.12f;
        g.foot_f = v2(10.5f + 4.0f * s, 29.0f - 2.0f * (c < 0 ? -c : 0));
        g.foot_b = v2(10.5f - 4.0f * s, 29.0f - 2.0f * (c > 0 ? c : 0));
        if (pose == MM3_POSE_SPRINT) { g.hand_f = v2(5.0f, 16.5f); g.hand_b = v2(4.5f, 18.0f); }
        else { g.hand_f = v2(10.5f - 3.5f * s, 18.0f); g.hand_b = v2(10.5f + 3.5f * s, 18.0f); }
        break;
    case MM3_POSE_SKID:
        g.rot = -0.22f;
        g.foot_f = v2(15.5f, 29.0f); g.foot_b = v2(9.0f, 29.0f);
        g.hand_f = v2(7.0f, 13.0f); g.hand_b = v2(8.0f, 14.5f);
        break;
    case MM3_POSE_CROUCH:
        g.head = v2(10.5f, 17.5f);
        g.shoulder = v2(10.5f, 22.5f); g.hip = v2(10.5f, 26.0f);
        g.hand_f = v2(13.5f, 26.5f); g.hand_b = v2(8.0f, 26.5f);
        g.foot_f = v2(13.5f, 29.0f); g.foot_b = v2(7.5f, 29.0f);
        if (squash) { g.sx = 1.12f; g.sy = 0.95f; }
        break;
    case MM3_POSE_LOOK_UP:
        g.eyes_up = 1;
        g.head = v2(10.2f, 7.6f);
        break;
    case MM3_POSE_JUMP:
        g.hand_f = v2(15.0f, 6.5f); g.hand_b = v2(6.5f, 19.0f);
        g.foot_f = v2(13.5f, 25.0f); g.foot_b = v2(9.0f, 29.0f);
        if (squash) { g.sx = 0.88f; g.sy = 1.1f; }
        break;
    case MM3_POSE_FALL:
        g.hand_f = v2(17.0f, 14.5f); g.hand_b = v2(4.0f, 14.5f);
        g.foot_f = v2(13.0f, 28.5f); g.foot_b = v2(8.0f, 28.0f);
        break;
    case MM3_POSE_SPIN:
    case MM3_POSE_TWIRL: {
        float hy = pose == MM3_POSE_TWIRL ? 12.0f : 17.0f;
        g.hand_f = v2(17.5f, hy); g.hand_b = v2(3.5f, hy);
        g.back_view = frame & 1;
        if (frame >= 2) { V2 tmp = g.hand_f; g.hand_f = g.hand_b; g.hand_b = tmp; }
        g.foot_f = v2(11.5f, 28.0f); g.foot_b = v2(9.5f, 28.5f);
        break;
    }
    case MM3_POSE_TRIPLE:
        tuck(&g);
        g.rot = t * 2.0f * PI_F;
        break;
    case MM3_POSE_BACKFLIP:
        tuck(&g);
        g.rot = -t * 2.0f * PI_F;
        break;
    case MM3_POSE_SIDEFLIP:
        tuck(&g);
        g.hand_f = v2(16.5f, 15.0f); g.hand_b = v2(4.5f, 15.0f);
        g.rot = t * 2.0f * PI_F;
        break;
    case MM3_POSE_LONG_JUMP:
        g.rot = 1.15f;
        g.hand_f = v2(16.0f, 12.0f); g.hand_b = v2(15.0f, 13.5f);
        g.foot_f = v2(11.0f, 29.5f); g.foot_b = v2(9.0f, 29.0f);
        break;
    case MM3_POSE_WALL_SLIDE:
        g.rot = -0.08f;
        g.hand_b = v2(3.5f, 12.0f); g.hand_f = v2(4.5f, 16.0f);
        g.foot_b = v2(4.5f, 28.0f); g.foot_f = v2(9.5f, 29.0f);
        break;
    case MM3_POSE_POUND:
        if (frame == 0) { tuck(&g); g.rot = 0.4f; }
        else {
            g.head = v2(10.5f, 12.0f);
            g.shoulder = v2(10.5f, 16.5f); g.hip = v2(10.5f, 23.0f);
            g.hand_f = v2(15.5f, 13.0f); g.hand_b = v2(5.5f, 13.0f);
            g.foot_f = v2(14.0f, 21.0f); g.foot_b = v2(7.0f, 21.0f);
            if (squash) { g.sx = 0.9f; g.sy = 1.08f; }
        }
        break;
    case MM3_POSE_SLIDE:
        g.rot = -1.0f;
        g.hand_f = v2(14.0f, 21.0f); g.hand_b = v2(6.0f, 22.0f);
        g.foot_f = v2(12.5f, 29.0f); g.foot_b = v2(9.5f, 29.0f);
        break;
    case MM3_POSE_SWIM:
        g.rot = 0.9f;
        g.hand_f = v2(10.5f + 5.0f * c, 13.0f + 4.0f * s);
        g.hand_b = v2(10.5f - 5.0f * c, 14.0f - 4.0f * s);
        g.foot_f = v2(11.5f + 1.5f * s, 29.0f); g.foot_b = v2(9.5f - 1.5f * s, 29.0f);
        break;
    case MM3_POSE_CLIMB:
        g.back_view = 1;
        g.hand_f = v2(14.0f, frame ? 10.0f : 15.0f); g.hand_b = v2(7.0f, frame ? 15.0f : 10.0f);
        g.foot_f = v2(12.5f, frame ? 29.0f : 27.0f); g.foot_b = v2(8.5f, frame ? 27.0f : 29.0f);
        break;
    case MM3_POSE_THROW:
        g.hand_f = v2(18.0f, 15.0f); g.hand_b = v2(7.0f, 19.0f);
        g.foot_f = v2(14.0f, 29.0f); g.foot_b = v2(8.0f, 29.0f);
        break;
    default:
        break;
    }
    if (carrying && pose != MM3_POSE_THROW && pose != MM3_POSE_CLIMB && pose != MM3_POSE_SWIM &&
        pose != MM3_POSE_SPIN && pose != MM3_POSE_TWIRL) {
        g.hand_f = v2(16.0f, 17.0f); g.hand_b = v2(15.0f, 18.5f);
    }
    return g;
}

/* ------------------------------------------------------------- raster */

typedef struct {
    int w, h;
    Uint32 *px;       /* ARGB8888 */
} Canvas;

static Uint32 argb(RGB c, float k, Uint8 a)
{
    int r = (int)(c.r * k), g = (int)(c.g * k), b = (int)(c.b * k);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return ((Uint32)a << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
}

static float dist_seg(V2 p, V2 a, V2 b, V2 *nearest)
{
    float abx = b.x - a.x, aby = b.y - a.y;
    float apx = p.x - a.x, apy = p.y - a.y;
    float l2 = abx * abx + aby * aby, t = l2 > 0 ? (apx * abx + apy * aby) / l2 : 0;
    float dx, dy;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    nearest->x = a.x + abx * t; nearest->y = a.y + aby * t;
    dx = p.x - nearest->x; dy = p.y - nearest->y;
    return sqrtf(dx * dx + dy * dy);
}

/* Light from the upper left: returns brightness multiplier for a surface
   point offset (ox, oy) from a shape's center with radius rad. */
static float light(const Style *st, float ox, float oy, float rad)
{
    float d = (ox * 0.6f + oy * 0.8f) / (rad > 0.01f ? rad : 0.01f);
    if (st->shade == 1) return d > 0.3f ? 0.78f : 1.0f;
    if (st->shade == 2) {
        float k = 1.08f - 0.25f * (d + 1.0f) * 0.5f;
        return k;
    }
    return 1.0f;
}

/* Sample the hero at art-space point p. Returns 1 and sets *out if hit. */
static int sample_hero(const Style *st, const Rig *g, V2 p, Uint32 *out)
{
    V2 n;
    float d, k;
    V2 hc = g->head;
    float hr = g->head_r;
    V2 sh_f = v2(g->shoulder.x + 1.2f, g->shoulder.y + 0.5f), sh_b = v2(g->shoulder.x - 1.2f, g->shoulder.y + 0.5f);
    V2 hip_f = v2(g->hip.x + 1.4f, g->hip.y), hip_b = v2(g->hip.x - 1.4f, g->hip.y);
    float eye_r = st->big_eyes ? 1.15f : 0.8f;

    /* front hand and arm */
    d = sqrtf((p.x - g->hand_f.x) * (p.x - g->hand_f.x) + (p.y - g->hand_f.y) * (p.y - g->hand_f.y));
    if (d < 1.5f) { *out = argb(st->skin, light(st, p.x - g->hand_f.x, p.y - g->hand_f.y, 1.5f), 255); return 1; }
    d = dist_seg(p, sh_f, g->hand_f, &n);
    if (d < 1.25f) { *out = argb(st->shirt, light(st, p.x - n.x, p.y - n.y, 1.25f), 255); return 1; }

    /* head */
    d = sqrtf((p.x - hc.x) * (p.x - hc.x) + (p.y - hc.y) * (p.y - hc.y));
    if (d < hr) {
        float lx = p.x - hc.x, ly = p.y - hc.y;
        k = light(st, lx, ly, hr);
        if (!g->back_view) {
            float ey = hc.y + 0.3f - (g->eyes_up ? 1.0f : 0.0f);
            float e1 = sqrtf((p.x - (hc.x + 2.1f)) * (p.x - (hc.x + 2.1f)) + (p.y - ey) * (p.y - ey));
            float e2 = sqrtf((p.x - (hc.x + 4.2f)) * (p.x - (hc.x + 4.2f)) + (p.y - ey) * (p.y - ey));
            if (e1 < eye_r || e2 < eye_r) { *out = argb(st->eye, 1.0f, 255); return 1; }
            if (st->big_eyes && (e1 < eye_r + 0.55f || e2 < eye_r + 0.55f)) { *out = argb(st->eye_white, 1.0f, 255); return 1; }
        }
        if (ly > -3.1f && ly < -1.5f) { *out = argb(st->band, k, 255); return 1; }
        if (g->back_view || ly < -1.5f || lx < -2.8f) { *out = argb(st->hair, k, 255); return 1; }
        *out = argb(st->skin, k, 255);
        return 1;
    }
    /* headband knot tails behind the head */
    d = dist_seg(p, v2(hc.x - hr + 0.3f, hc.y - 2.3f), v2(hc.x - hr - 2.2f, hc.y - 0.6f), &n);
    if (d < 0.75f) { *out = argb(st->band, 0.9f, 255); return 1; }

    /* front leg */
    d = sqrtf((p.x - g->foot_f.x - 0.7f) * (p.x - g->foot_f.x - 0.7f) + (p.y - g->foot_f.y + 0.9f) * (p.y - g->foot_f.y + 0.9f) * 1.6f);
    if (d < 1.9f) { *out = argb(st->shoes, light(st, p.x - g->foot_f.x, p.y - g->foot_f.y, 1.9f), 255); return 1; }
    d = dist_seg(p, hip_f, v2(g->foot_f.x, g->foot_f.y - 1.0f), &n);
    if (d < 1.45f) { *out = argb(st->pants, light(st, p.x - n.x, p.y - n.y, 1.45f), 255); return 1; }

    /* torso */
    d = dist_seg(p, g->shoulder, g->hip, &n);
    if (d < 3.3f) {
        float kk = light(st, p.x - n.x, p.y - n.y, 3.3f);
        if (p.y > g->hip.y - 1.2f && (g->hip.y - g->shoulder.y) > 3.0f) { *out = argb(st->pants, kk, 255); return 1; }
        *out = argb(st->shirt, kk, 255);
        return 1;
    }

    /* back leg and arm, slightly darker */
    d = sqrtf((p.x - g->foot_b.x - 0.7f) * (p.x - g->foot_b.x - 0.7f) + (p.y - g->foot_b.y + 0.9f) * (p.y - g->foot_b.y + 0.9f) * 1.6f);
    if (d < 1.9f) { *out = argb(st->shoes, 0.8f, 255); return 1; }
    d = dist_seg(p, hip_b, v2(g->foot_b.x, g->foot_b.y - 1.0f), &n);
    if (d < 1.45f) { *out = argb(st->pants, 0.8f, 255); return 1; }
    d = sqrtf((p.x - g->hand_b.x) * (p.x - g->hand_b.x) + (p.y - g->hand_b.y) * (p.y - g->hand_b.y));
    if (d < 1.5f) { *out = argb(st->skin, 0.8f, 255); return 1; }
    d = dist_seg(p, sh_b, g->hand_b, &n);
    if (d < 1.25f) { *out = argb(st->shirt, 0.8f, 255); return 1; }
    return 0;
}

static void outline(Canvas *cv, Uint32 color, int passes)
{
    int pass, x, y;
    Uint32 *tmp = (Uint32 *)malloc(sizeof(Uint32) * (size_t)(cv->w * cv->h));
    if (!tmp) return;
    for (pass = 0; pass < passes; pass++) {
        memcpy(tmp, cv->px, sizeof(Uint32) * (size_t)(cv->w * cv->h));
        for (y = 0; y < cv->h; y++) {
            for (x = 0; x < cv->w; x++) {
                int dx, dy, hit = 0;
                if (tmp[y * cv->w + x] >> 24) continue;
                for (dy = -1; dy <= 1 && !hit; dy++)
                    for (dx = -1; dx <= 1 && !hit; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= cv->w || ny >= cv->h) continue;
                        if (tmp[ny * cv->w + nx] >> 24) hit = 1;
                    }
                if (hit) cv->px[y * cv->w + x] = color;
            }
        }
    }
    free(tmp);
}

static SDL_Texture *to_texture(SDL_Renderer *r, const Canvas *cv)
{
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, cv->w, cv->h);
    if (!t) return NULL;
    SDL_UpdateTexture(t, NULL, cv->px, cv->w * 4);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    return t;
}

static SDL_Texture *render_hero(SDL_Renderer *r, const Style *st, const Rig *g)
{
    Canvas cv;
    int x, y;
    SDL_Texture *t;
    float cr = cosf(-g->rot), sr = sinf(-g->rot);
    const float pivot_x = 10.5f, pivot_y = 19.0f;
    cv.w = ART_SPRITE_W * st->res;
    cv.h = ART_SPRITE_H * st->res;
    cv.px = (Uint32 *)calloc((size_t)(cv.w * cv.h), sizeof(Uint32));
    if (!cv.px) return NULL;
    for (y = 0; y < cv.h; y++) {
        for (x = 0; x < cv.w; x++) {
            V2 p = v2((x + 0.5f) / st->res, (y + 0.5f) / st->res);
            V2 q;
            Uint32 c;
            /* undo squash (around the feet) then rotation (around the body) */
            p.x = 10.5f + (p.x - 10.5f) / g->sx;
            p.y = 29.5f + (p.y - 29.5f) / g->sy;
            q.x = pivot_x + (p.x - pivot_x) * cr - (p.y - pivot_y) * sr;
            q.y = pivot_y + (p.x - pivot_x) * sr + (p.y - pivot_y) * cr;
            if (sample_hero(st, g, q, &c)) cv.px[y * cv.w + x] = c;
        }
    }
    if (st->outline) outline(&cv, argb(st->line, 1.0f, 255), st->outline);
    t = to_texture(r, &cv);
    free(cv.px);
    return t;
}

/* ------------------------------------------------------------------ tiles */

enum { TT_GROUND_TOP, TT_GROUND, TT_SEMI, TT_WATER_TOP, TT_WATER, TT_VINE,
       TT_SLOPE_UP, TT_SLOPE_DOWN, TT_UP_LO, TT_UP_HI, TT_DOWN_HI, TT_DOWN_LO, TT_COUNT };

static float slope_h(int tt, float lx)
{
    switch (tt) {
    case TT_SLOPE_UP:   return lx;
    case TT_SLOPE_DOWN: return 16.0f - lx;
    case TT_UP_LO:      return lx * 0.5f;
    case TT_UP_HI:      return 8.0f + lx * 0.5f;
    case TT_DOWN_HI:    return 16.0f - lx * 0.5f;
    case TT_DOWN_LO:    return 8.0f - lx * 0.5f;
    default:            return 16.0f;
    }
}

/* Which tile pattern family a style uses. */
static int look_of(int style)
{
    if (style == MM3_STYLE_RETRO_CLASSIC) return MM3_STYLE_RETRO;
    if (style == MM3_STYLE_ISLAND_CLASSIC) return MM3_STYLE_ISLAND;
    return style;
}

/* Ground material at local (lx, ly); depth = distance below the surface. */
static Uint32 ground_px(const Style *st, int style, float lx, float ly, float depth, int ix, int iy)
{
    style = look_of(style);
    float gd = (style == MM3_STYLE_RETRO || style == MM3_STYLE_ARCADE) ? 3.0f : 4.0f;
    if (depth < gd) {
        if (style == MM3_STYLE_ISLAND && depth > gd - 1.0f) return argb(st->grass2, 1.0f, 255);
        if (style == MM3_STYLE_MODERN && depth < 1.0f) return argb(st->grass2, 1.0f, 255);
        if (style == MM3_STYLE_BLOOM && ((ix * 7 + iy * 3) % 23) == 0) return argb(st->grass2, 1.0f, 255);
        if (style == MM3_STYLE_RETRO && depth > gd - 1.0f) return argb(st->grass2, 1.0f, 255);
        return argb(st->grass, st->shade == 2 ? 1.05f - depth * 0.04f : 1.0f, 255);
    }
    switch (style) {
    case MM3_STYLE_RETRO:
        return ((ix * 7 + iy * 3) % 11 == 0) ? argb(st->ground2, 1.0f, 255) : argb(st->ground, 1.0f, 255);
    case MM3_STYLE_ARCADE: {
        int check = (((int)(lx / 8.0f)) + ((int)(ly / 8.0f))) & 1;
        int edge = ((int)lx % 8 == 7) || ((int)ly % 8 == 7);
        if (edge) return argb(st->ground2, 0.8f, 255);
        return argb(check ? st->ground2 : st->ground, 1.0f, 255);
    }
    case MM3_STYLE_ISLAND: {
        float cx = (float)((int)(lx / 8.0f)) * 8.0f + 4.0f + (((int)(ly / 8.0f)) % 2) * 2.0f;
        float cy = (float)((int)(ly / 8.0f)) * 8.0f + 4.0f;
        float d = (lx - cx) * (lx - cx) + (ly - cy) * (ly - cy);
        return d < 3.0f ? argb(st->ground2, 1.0f, 255) : argb(st->ground, 1.0f, 255);
    }
    case MM3_STYLE_MODERN:
        return argb(st->ground, 1.05f - ly * 0.018f, 255);
    case MM3_STYLE_ATHLETIC:
        if (lx < 1.5f || ly < 1.5f) return argb(st->plank, 0.95f, 255);
        if (lx > 14.5f || ly > 14.5f) return argb(st->ground2, 1.0f, 255);
        return argb(st->ground, 1.0f, 255);
    case MM3_STYLE_BLOOM: {
        float d = sinf(lx * 0.7f) + cosf(ly * 0.6f + lx * 0.2f);
        return d > 1.2f ? argb(st->ground2, 1.0f, 255) : argb(st->ground, 1.0f, 255);
    }
    default: {
        int crack = ((int)lx == 5 && ly > 3 && ly < 9) || ((int)ly == 11 && lx > 8 && lx < 14);
        return crack ? argb(st->ground2, 1.0f, 255) : argb(st->ground, 1.0f - (ly > 8 ? 0.05f : 0.0f), 255);
    }
    }
}

static SDL_Texture *render_tile(SDL_Renderer *r, const Style *st, int style, int tt)
{
    Canvas cv;
    int x, y;
    SDL_Texture *t;
    cv.w = cv.h = 16 * st->res;
    cv.px = (Uint32 *)calloc((size_t)(cv.w * cv.h), sizeof(Uint32));
    if (!cv.px) return NULL;
    for (y = 0; y < cv.h; y++) {
        for (x = 0; x < cv.w; x++) {
            float lx = (x + 0.5f) / st->res, ly = (y + 0.5f) / st->res;
            int ix = x / st->res, iy = y / st->res;
            Uint32 c = 0;
            switch (tt) {
            case TT_GROUND_TOP: c = ground_px(st, style, lx, ly, ly, ix, iy); break;
            case TT_GROUND:     c = ground_px(st, style, lx, ly, 99.0f, ix, iy); break;
            case TT_SEMI:
                if (ly < 5.0f) c = argb(ly < 1.0f || ly > 4.0f ? st->plank2 : st->plank,
                                        ((int)lx % 8 == 0) ? 0.85f : 1.0f, 255);
                else if (ly < 9.0f && ((int)lx == 3 || (int)lx == 12)) c = argb(st->plank2, 1.0f, 255);
                break;
            case TT_WATER_TOP:
            case TT_WATER: {
                float wave = tt == TT_WATER_TOP ? 2.0f + sinf(lx * 0.8f) : -1.0f;
                if (ly >= wave) c = argb(st->water, (tt == TT_WATER_TOP && ly < wave + 1.2f) ? 1.6f : 1.0f,
                                         (tt == TT_WATER_TOP && ly < wave + 1.2f) ? 200 : 140);
                break;
            }
            case TT_VINE: {
                float cx = 8.0f + 1.5f * sinf(ly * 0.45f);
                float leaf = 8.0f + ((iy / 4) % 2 ? 3.5f : -3.5f);
                if (fabsf(lx - cx) < 1.1f) c = argb(st->vine, 0.8f, 255);
                else if ((iy % 4) < 2 && fabsf(lx - leaf) < 2.0f) c = argb(st->vine, 1.1f, 255);
                break;
            }
            default: {
                float h = slope_h(tt, lx);
                float surf = 16.0f - h;
                if (ly >= surf) {
                    float depth = (ly - surf) * (tt == TT_SLOPE_UP || tt == TT_SLOPE_DOWN ? 0.72f : 0.9f);
                    c = ground_px(st, style, lx, ly, depth, ix, iy);
                }
                break;
            }
            }
            cv.px[y * cv.w + x] = c;
        }
    }
    if (st->outline && (tt == TT_SEMI)) outline(&cv, argb(st->line, 1.0f, 255), 1);
    t = to_texture(r, &cv);
    free(cv.px);
    return t;
}

static SDL_Texture *render_crate(SDL_Renderer *r, const Style *st)
{
    Canvas cv;
    int x, y;
    SDL_Texture *t;
    cv.w = cv.h = 14 * st->res;
    cv.px = (Uint32 *)calloc((size_t)(cv.w * cv.h), sizeof(Uint32));
    if (!cv.px) return NULL;
    for (y = 0; y < cv.h; y++)
        for (x = 0; x < cv.w; x++) {
            float lx = (x + 0.5f) / st->res, ly = (y + 0.5f) / st->res;
            int frame = lx < 2.0f || ly < 2.0f || lx > 12.0f || ly > 12.0f;
            int diag = fabsf(lx - ly) < 1.2f || fabsf(lx + ly - 14.0f) < 1.2f;
            RGB col = (frame || diag) ? st->crate2 : st->crate;
            cv.px[y * cv.w + x] = argb(col, light(st, lx - 7.0f, ly - 7.0f, 9.0f), 255);
        }
    if (st->outline) outline(&cv, argb(st->line, 1.0f, 255), 1);
    t = to_texture(r, &cv);
    free(cv.px);
    return t;
}

#define BG_W 512
#define BG_H 256

static SDL_Texture *render_background(SDL_Renderer *r, const Style *st, int style)
{
    Canvas cv;
    int x, y;
    SDL_Texture *t;
    cv.w = BG_W; cv.h = BG_H;
    cv.px = (Uint32 *)calloc((size_t)(cv.w * cv.h), sizeof(Uint32));
    if (!cv.px) return NULL;
    for (x = 0; x < BG_W; x++) {
        float u = (float)x / BG_W * 2.0f * PI_F;
        float far_h = 150.0f + 22.0f * sinf(u * 2.0f) + 10.0f * sinf(u * 5.0f + 1.0f);
        float near_h = 190.0f + 18.0f * sinf(u * 3.0f + 2.0f) + 6.0f * sinf(u * 8.0f);
        for (y = 0; y < BG_H; y++) {
            float k = (float)y / BG_H;
            RGB sky;
            sky.r = (Uint8)(st->sky_top.r + (st->sky_bottom.r - st->sky_top.r) * k);
            sky.g = (Uint8)(st->sky_top.g + (st->sky_bottom.g - st->sky_top.g) * k);
            sky.b = (Uint8)(st->sky_top.b + (st->sky_bottom.b - st->sky_top.b) * k);
            if (look_of(style) == MM3_STYLE_RETRO) sky = st->sky_top;
            if (y > near_h) cv.px[y * BG_W + x] = argb(st->hill_near, 1.0f, 255);
            else if (y > far_h) cv.px[y * BG_W + x] = argb(st->hill_far, 1.0f, 255);
            else cv.px[y * BG_W + x] = argb(sky, 1.0f, 255);
        }
    }
    t = to_texture(r, &cv);
    free(cv.px);
    return t;
}

/* ------------------------------------------------------------------ state */

static SDL_Texture *g_hero[N_STYLES][MM3_POSE_COUNT][MAX_FRAMES][2];
static SDL_Texture *g_tiles[N_STYLES][TT_COUNT];
static SDL_Texture *g_crate[N_STYLES];
static SDL_Texture *g_bg[N_STYLES];

int art_init(SDL_Renderer *r)
{
    int s, p, f, c;
    for (s = 0; s < N_STYLES; s++) {
        const Style *st = &k_styles[s];
        for (p = 0; p < MM3_POSE_COUNT; p++)
            for (f = 0; f < k_frames[p]; f++)
                for (c = 0; c < 2; c++) {
                    Rig g = pose_rig(p, f, c, st->squash);
                    g_hero[s][p][f][c] = render_hero(r, st, &g);
                }
        for (p = 0; p < TT_COUNT; p++) g_tiles[s][p] = render_tile(r, st, s, p);
        g_crate[s] = render_crate(r, st);
        g_bg[s] = render_background(r, st, s);
    }
    return 1;
}

void art_free(void)
{
    int s, p, f, c;
    for (s = 0; s < N_STYLES; s++) {
        for (p = 0; p < MM3_POSE_COUNT; p++)
            for (f = 0; f < MAX_FRAMES; f++)
                for (c = 0; c < 2; c++)
                    if (g_hero[s][p][f][c]) SDL_DestroyTexture(g_hero[s][p][f][c]);
        for (p = 0; p < TT_COUNT; p++) if (g_tiles[s][p]) SDL_DestroyTexture(g_tiles[s][p]);
        if (g_crate[s]) SDL_DestroyTexture(g_crate[s]);
        if (g_bg[s]) SDL_DestroyTexture(g_bg[s]);
    }
}

void art_draw_player(SDL_Renderer *r, int style, int pose, int anim, int carrying,
                     int facing_left, int x, int y)
{
    int n = k_frames[pose];
    int frame = (anim / k_period[pose]) % n;
    SDL_Rect dst;
    SDL_Texture *t;
    if (pose == MM3_POSE_POUND) frame = anim < 16 ? 0 : 1;
    if (pose == MM3_POSE_WALL_SLIDE) facing_left = !facing_left;
    t = g_hero[style][pose][frame][carrying ? 1 : 0];
    if (!t) return;
    dst.x = x; dst.y = y; dst.w = ART_SPRITE_W; dst.h = ART_SPRITE_H;
    SDL_RenderCopyEx(r, t, NULL, &dst, 0.0, NULL, facing_left ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}

void art_draw_tile(SDL_Renderer *r, int style, int tile, int top, int x, int y)
{
    int tt;
    SDL_Rect dst;
    switch (tile) {
    case MM3_TILE_SOLID:         tt = top ? TT_GROUND_TOP : TT_GROUND; break;
    case MM3_TILE_SEMISOLID:     tt = TT_SEMI; break;
    case MM3_TILE_SLOPE_UP:      tt = TT_SLOPE_UP; break;
    case MM3_TILE_SLOPE_DOWN:    tt = TT_SLOPE_DOWN; break;
    case MM3_TILE_SLOPE_UP_LO:   tt = TT_UP_LO; break;
    case MM3_TILE_SLOPE_UP_HI:   tt = TT_UP_HI; break;
    case MM3_TILE_SLOPE_DOWN_HI: tt = TT_DOWN_HI; break;
    case MM3_TILE_SLOPE_DOWN_LO: tt = TT_DOWN_LO; break;
    case MM3_TILE_WATER:         tt = top ? TT_WATER_TOP : TT_WATER; break;
    case MM3_TILE_VINE:          tt = TT_VINE; break;
    default: return;
    }
    dst.x = x; dst.y = y; dst.w = dst.h = 16;
    SDL_RenderCopy(r, g_tiles[style][tt], NULL, &dst);
}

void art_draw_crate(SDL_Renderer *r, int style, int x, int y)
{
    SDL_Rect dst;
    dst.x = x; dst.y = y; dst.w = dst.h = 14;
    SDL_RenderCopy(r, g_crate[style], NULL, &dst);
}

void art_draw_background(SDL_Renderer *r, int style, int cam_x, int cam_y)
{
    int off = (cam_x / 3) % BG_W, x;
    SDL_Rect dst;
    (void)cam_y;
    for (x = -off; x < 448; x += BG_W) {
        dst.x = x; dst.y = 0; dst.w = BG_W; dst.h = BG_H;
        SDL_RenderCopy(r, g_bg[style], NULL, &dst);
    }
}

/* ------------------------------------------------------------------- font */

/* 5x7 glyphs, one byte per row, bit 4 = leftmost column. */
static const char k_font_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -+./:()!?";
static const Uint8 k_font[][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14},{7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,17,25,21,19,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2},{31,16,30,1,1,17,14},{6,8,16,30,17,17,14},{31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},{14,17,17,15,1,2,12},
    {0,0,0,0,0,0,0},{0,0,0,31,0,0,0},{0,4,4,31,4,4,0},{0,0,0,0,0,12,12},{1,2,2,4,8,8,16},
    {0,12,12,0,12,12,0},{2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{4,4,4,4,4,0,4},{14,17,1,2,4,0,4}
};

int art_text_width(const char *s)
{
    return (int)strlen(s) * 6;
}

int art_text(SDL_Renderer *r, const char *s, int x, int y, Uint8 cr, Uint8 cg, Uint8 cb)
{
    int x0 = x;
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    for (; *s; s++) {
        char ch = *s;
        const char *pos;
        int gi, row, col;
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        pos = strchr(k_font_chars, ch);
        if (pos && ch) {
            gi = (int)(pos - k_font_chars);
            for (row = 0; row < 7; row++)
                for (col = 0; col < 5; col++)
                    if (k_font[gi][row] & (16 >> col)) {
                        SDL_Rect px;
                        px.x = x + col; px.y = y + row; px.w = 1; px.h = 1;
                        SDL_RenderFillRect(r, &px);
                    }
        }
        x += 6;
    }
    return x - x0;
}
