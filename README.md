# MM3 (internal codename)

An original 2D platformer **level maker** with multiple selectable physics
styles, a fully **deterministic** simulation, and a core designed to run on
**any device** — desktop, browser, phones, consoles.

> "MM3" is a codename only. The shipping product will have its own original
> name, characters, art and audio. See [docs/ART_AND_IP.md](docs/ART_AND_IP.md).

**Status:** Milestone 1 — architecture, docs and a compilable skeleton with a
placeholder-rectangle demo of all five physics styles.

## Physics styles

| Key | Style | Feel |
|---|---|---|
| 1 | Classic | Heavy momentum, committed jumps, no air turning |
| 2 | New | Snappier, wall jump, ground pound, spin jump |
| 3 | World | Tight control, long jump, dash |
| 4 | Wonder | Forgiving, floaty, generous timing windows |
| 5 | Custom | Your own tunable style |

Details: [docs/PHYSICS.md](docs/PHYSICS.md).

## Testing without a dev setup

You don't need to install anything. Every push is built by GitHub Actions for
Windows, macOS, Linux and the browser. Follow [docs/TESTING.md](docs/TESTING.md)
to get a link you can open in your browser
(for example **https://bambitp.github.io/mm3/** for `main`, once Pages is enabled).

Controls: arrows/WASD move · Z/Space jump · X/Shift run · Down = ground pound
(in air) or long jump (while running, World style) · C spin · 1–5 style · R restart.

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
