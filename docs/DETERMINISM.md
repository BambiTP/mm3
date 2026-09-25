# Determinism contract

"Deterministic" means: **the same level + the same seed + the same button
presses produce bit-for-bit the same result on every device, compiler and
build.** That is what makes replays, speedrun verification, level
"clear checks", ghost races and netplay possible.

## Rules for everything in `core/`

| Do | Don't |
|---|---|
| Use integers; positions/speeds in subpixels (256 per pixel) | Use `float` or `double` — results differ between CPUs, compilers and optimization levels |
| Use `mm3_floor_div`, `mm3_fx_to_px` | Right-shift negative numbers (`>>` on negatives is implementation-defined) |
| Keep values well inside `int32_t` | Rely on signed overflow (undefined behavior) |
| Draw random numbers from `state->rng` | Call `rand()`, `time()`, or read any clock |
| Iterate entities in ascending id order | Iterate by pointer address or hash-table order |
| Keep all state inside `mm3_game_state` | Use `static` mutable variables in core |
| Hash fields explicitly in little-endian order | Hash/serialize raw structs (padding and endianness differ) |
| Use fixed-size arrays | `malloc` during a tick |
| Take input only via the `mm3_buttons` argument | Read keyboard/touch/gamepad or screen size in core |

Frontends may use floats freely (rendering, audio, UI) — as long as nothing
they compute flows back into the simulation other than the buttons.

## How it is enforced

- `tests/test_determinism.c` plays one minute of scripted input for every
  physics style and checks that:
  1. re-running gives the same hash,
  2. a save-state round-trip mid-run gives the same hash,
  3. the hash equals a **golden value** committed in the test.
- CI runs that test on **Linux (GCC), Windows (MSVC), macOS (Clang) and
  WebAssembly (Node)**. If any platform disagrees, the build fails.
- Core compiles with `-Wall -Wextra -Wpedantic -Werror` (MSVC `/W4 /WX`).

## When you intentionally change gameplay

Changing physics numbers, tick order, or collision changes the golden hashes.
That is expected:

1. Run `test_determinism --print` and paste the new values into `k_golden`.
2. Once the game has shipped, **don't** change an existing style in place.
   Add a new profile version so old levels, replays and leaderboards remain
   valid (levels store the profile by value — see `LEVEL_FORMAT.md`).

## Known future work

- Replay recorder/player (level + seed + input stream) and a replay-based
  regression test suite.
- Render interpolation (frontend only; must not touch the state).
- 32-bit ARM / big-endian CI job once a cross toolchain is set up.
