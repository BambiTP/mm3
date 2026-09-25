# MM3 (internal codename)

An original 2D platformer **level maker** with multiple selectable physics
styles, a fully **deterministic** simulation, and a core designed to run on
**any device** — desktop, browser, phones, consoles.

> "MM3" is a codename only. The shipping product will have its own original
> name, characters, art and audio. See [docs/ART_AND_IP.md](docs/ART_AND_IP.md).

**Status:** Milestone 2 in progress — six physics styles with their full
move sets (two reproduced exactly from their source games), swimming,
climbing, carrying, slopes, and procedural placeholder art per style.

## Physics styles

| Key | Style | What it plays like |
|---|---|---|
| 1 | Retro | 8-bit rules, exact: heavy momentum, no mid-air turning, swim, vines |
| 2 | Island | 16-bit rules, exact: P-meter sprint, spin jump, carry, look up |
| 3 | Modern | HD 2D: triple jump, wall jump, ground pound, twirl |
| 4 | Athletic | 3D-era moves in 2D: long jump, backflip, side flip, dash |
| 5 | Bloom | Floaty and forgiving: spin, twirl, wall jump |
| 6 | Custom | Every move from every style, tunable |

Details, sources and accuracy of every number: [docs/PHYSICS.md](docs/PHYSICS.md).

## Testing without a dev setup

You don't need to install anything. Every push is built by GitHub Actions for
Windows, macOS, Linux and the browser. Follow [docs/TESTING.md](docs/TESTING.md)
to get a link you can open in your browser
(for example **https://bambitp.github.io/mm3/** for `main`, once Pages is enabled).

Controls: arrows/WASD move · Z/Space jump · X/Shift run, pick up and throw ·
C spin / twirl · Down crouch, slide, ground pound · Up vine, look up ·
1–6 style · R restart. Gamepads work too (A jump, B spin, X/Y run).

## Building locally (optional)

```sh
# Desktop (needs CMake + a C compiler + SDL2 dev package)
cmake -B build -DCMAKE_BUILD_TYPE=Release     # add -DMM3_FETCH_SDL2=ON to auto-download SDL2
cmake --build build --config Release
ctest --test-dir build -C Release             # determinism tests
./build/mm3

# Browser (needs Emscripten)
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web                       # open build-web/index.html via a local web server
```

## Layout

```
core/       deterministic simulation (pure C99, no floats, no OS calls)
platform/   frontends: sdl2/ (desktop + web + mobile); platform.h = porting contract
tests/      determinism tests (golden hashes checked on every platform in CI)
web/        HTML shell for the browser build
assets/     original art/audio (empty for now)
docs/       architecture, determinism, physics, formats, porting, business
```

## Docs

- [ARCHITECTURE](docs/ARCHITECTURE.md) — layers and data flow
- [DETERMINISM](docs/DETERMINISM.md) — the rules that keep every device in sync
- [PHYSICS](docs/PHYSICS.md) — physics styles and parameters
- [LEVEL_FORMAT](docs/LEVEL_FORMAT.md) — level and replay files
- [PORTING](docs/PORTING.md) — adding a new device
- [ART_AND_IP](docs/ART_AND_IP.md) — staying clear of copyright/trademark trouble
- [BUSINESS](docs/BUSINESS.md) — closed-source, licensing, where to sell
- [TESTING](docs/TESTING.md) — play every build in your browser
- [ROADMAP](docs/ROADMAP.md) — what comes next

Proprietary — all rights reserved. See [LICENSE](LICENSE).
