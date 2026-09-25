# Porting guide

The core needs only a C99 compiler and `<stdint.h>`/`<string.h>`. A port is a
frontend that implements the responsibilities in `platform/platform.h`:
input → buttons, 60 Hz fixed-timestep ticking, drawing, audio, file I/O.

## Target matrix

| Target | Frontend | Status | Notes |
|---|---|---|---|
| Windows | SDL2 | ✅ builds in CI | MSVC, SDL2 statically linked |
| macOS | SDL2 | ✅ builds in CI | Apple Silicon + Intel; notarization needed to distribute |
| Linux | SDL2 | ✅ builds in CI | Also covers Steam Deck |
| Web browsers | SDL2 via Emscripten | ✅ builds in CI | WebAssembly + WebGL; hosted on GitHub Pages |
| Android | SDL2 | planned | SDL2 Android project template + touch controls overlay |
| iOS / iPadOS | SDL2 | planned | Needs a Mac + Apple developer account to build/sign |
| Nintendo Switch / PlayStation / Xbox | custom frontend on console SDK | future | Requires becoming a licensed developer (NDA SDKs; code can't be public) |
| Homebrew / retro handhelds | custom or SDL2 | possible | Core has no heap and no floats |

## Steps to add a platform

1. Create `platform/<name>/`.
2. Map the device's controls to `mm3_buttons` bits (`core/include/mm3/input.h`).
   Touch devices: an on-screen D-pad + buttons overlay that sets the same bits.
3. Drive the simulation with `mm3_pacer_advance()` so it ticks at exactly
   60 Hz regardless of the display's refresh rate.
4. Draw from the read-only `mm3_game_state`.
5. Add the build to CI and make sure `test_determinism` passes on the device
   (or an emulator of it) — that proves the port simulates identically.

## Portability rules for frontends

- Keep platform `#ifdef`s inside `platform/`; never in `core/`.
- Scale the fixed 448×256 view with integer scaling where possible; letterbox
  otherwise.
- Never change simulation speed to match a display; always use the pacer.
