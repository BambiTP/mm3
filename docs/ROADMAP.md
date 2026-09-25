# Roadmap

## Milestone 1 — foundation ✅ (this commit)
- Architecture, determinism contract, physics design, file formats, porting,
  IP and business docs.
- Deterministic C99 core: fixed-point math, RNG, tilemap collision, player
  movement with 5 data-driven physics styles.
- Golden-hash determinism tests on Linux/Windows/macOS/WebAssembly in CI.
- SDL2 frontend (desktop + browser) with placeholder rectangles.
- Auto-deploy of the browser build to Cloudflare Pages.

## Milestone 2 — feels good to play
- Playtest and retune each style; implement crouch jump and wall climb.
- Render interpolation, camera smoothing, basic sound effects.
- Replay recorder/player + replay-based regression tests.
- Touch controls overlay (phones/tablets in the browser).
- Gamepad support.

## Milestone 3 — the maker
- Level editor: place/erase tiles, spawn/goal, undo/redo, play-test toggle.
- Level save/load (`LEVEL_FORMAT.md`), physics style picker per level,
  Custom-physics sliders.
- First original tileset + player character.

## Milestone 4 — a real game
- Entities: enemies, coins/collectibles, springs, moving platforms, hazards,
  doors/pipes-equivalent warps, checkpoints, goal.
- Power-ups (original designs), multiple themes.
- Title screen, menus, settings, level clear conditions.

## Milestone 5 — sharing
- Online level upload/browse/play with accounts, likes, reports, moderation.
- Server-side replay verification of clears ("clear check") and speedruns.
- Android and iOS builds.

## Later
- Multiplayer (rollback netcode, enabled by determinism).
- Console ports via publisher / developer programs.
