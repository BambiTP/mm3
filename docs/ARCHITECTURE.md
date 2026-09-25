# Architecture

## Layers

```
┌──────────────────────────────────────────────────────────────┐
│ Frontend (one per platform family)                           │
│   platform/sdl2  → Windows, macOS, Linux, Web, Android, iOS  │
│   platform/<x>   → consoles, handhelds, anything else        │
│   input → buttons · timing · rendering · audio · file I/O    │
└───────────────┬──────────────────────────────▲───────────────┘
                │ mm3_tick(state, buttons)     │ read-only state
┌───────────────▼──────────────────────────────┴───────────────┐
│ Core (core/) — pure C99, deterministic                       │
│   sim.c        tick order, init, state hash                  │
│   player.c     movement driven by the physics profile        │
│   physics_*.c  built-in physics styles (data only)           │
│   rng.c        seeded xorshift32, lives in the game state    │
│   (future)     entities, level editor ops, level/replay I/O  │
└──────────────────────────────────────────────────────────────┘
```

**The single most important rule:** the core is a pure function
`(state, buttons) → next state`. It never reads the clock, the filesystem,
the screen size, or anything else from the outside world. Everything
device-specific lives in a frontend.

## Why this shape

| Goal | How the architecture delivers it |
|---|---|
| Runs on any device | Core is dependency-free C99 with no heap and no floats; compiles with any C compiler, including console SDKs and WebAssembly. |
| Deterministic | Integer math only, fixed 60 Hz tick, input is the only external data. Enforced by golden-hash tests in CI on every OS. |
| Multiple physics styles | Styles are data (`mm3_physics_profile`), not code branches. New style = new table row. |
| Replays / sharing / netplay | Replay = level + seed + one `uint16` per frame. Save-state = `memcpy` of `mm3_game_state`. |
| Level editor | The editor edits the same `mm3_game_state`/level data the game plays; "play test" just starts ticking. |

## Game state

`mm3_game_state` (`core/include/mm3/world.h`) is plain data: fixed-size
arrays, no pointers. This enables:

- save-states and instant restart (`memcpy`),
- rollback netcode later (keep N past states, re-simulate),
- identical behavior on devices without `malloc`.

The tilemap cap is 256×64 tiles for now; it will become a level-size setting
once the editor exists.

## Main loop (frontend)

```
every display frame:
    elapsed = real time since last frame
    n = mm3_pacer_advance(&pacer, elapsed)   // how many 1/60 s steps are due
    repeat n: mm3_tick(&state, read_buttons())
    draw(state)                               // never modifies state
```

Rendering may run at 30, 60, 144 Hz or anything else; the simulation always
advances in exact 1/60 s steps. (Render interpolation between the last two
states will be added later for high-refresh displays.)

## Tick order (keep stable — changing it changes replays)

1. Player input edges (pressed/released)
2. Horizontal acceleration
3. Jumps, wall jumps, ground pound
4. Gravity
5. Move X → collide; move Y → collide; wall contact check
6. Timers, pit respawn
7. `frame++`

Enemies, objects and editor-placed gadgets will slot in as steps between 5
and 6, always iterated in ascending entity-id order.
