/*
 * input.h - Abstract controller buttons.
 *
 * A tick receives exactly one uint16_t per player. Frontends map keyboards,
 * gamepads and touch controls onto these bits; the core never sees devices.
 * A replay is just the level + seed + this bitfield for every frame.
 */
#ifndef MM3_INPUT_H
#define MM3_INPUT_H

#include <stdint.h>

typedef uint16_t mm3_buttons;

enum {
    MM3_BTN_LEFT  = 1u << 0,
    MM3_BTN_RIGHT = 1u << 1,
    MM3_BTN_UP    = 1u << 2,
    MM3_BTN_DOWN  = 1u << 3,
    MM3_BTN_JUMP  = 1u << 4,
    MM3_BTN_RUN   = 1u << 5,  /* run / dash / action */
    MM3_BTN_SPIN  = 1u << 6,  /* spin jump (styles that support it) */
    MM3_BTN_START = 1u << 7
};

#endif
