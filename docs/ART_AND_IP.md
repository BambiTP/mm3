# Art and intellectual property

Goal: a game that is clearly **its own thing**, so it can be sold on any
store without takedowns. This is practical guidance, not legal advice — talk
to a lawyer before launch.

## What's generally safe

- **Game mechanics and genre.** Running, jumping, wall jumps, ground pounds,
  building levels and sharing them: gameplay ideas are not protected by
  copyright.
- **Physics numbers.** Game mechanics and plain numeric facts (a speed, a
  gravity value) are generally not protected by copyright, so the Retro,
  Island and Modern styles reproduce the source games' published physics
  constants from community disassemblies (see `docs/PHYSICS.md`). Only the
  numbers and the behavior are reproduced: no code, art, audio or text.
  This is low risk but not zero; have a lawyer review before release. Every
  such constant is isolated in a profile table or a clearly marked engine
  file, so it can be swapped for tuned values if needed.
- **Generic concepts**: blocks, coins, springs, moving platforms, enemies that
  walk back and forth, goal flags.

## What to avoid

- **Names and trademarks**: "Mario", "Maker" in a way that suggests the
  official series, "Nintendo", character/enemy/item names from those games
  (e.g. no "Goomba", "Koopa", "Super Mushroom", "Fire Flower", "?-block").
  The public name, store page and marketing must not reference them.
- **Look-alike characters**: no red-cap plumber, no mushroom people, no
  turtle-shell enemies. Design new silhouettes and color schemes.
- **Copied or traced sprites, textures, fonts, sounds or music**, including
  "recreations" of recognizable jingles (coin sound, power-up, level clear).
- **Trade dress**: don't replicate specific UI layouts, logos, title-screen
  compositions or the exact look of the four official art styles. The four
  physics styles have their own public names (Retro, Arcade, Island, Modern,
  Athletic, Bloom, Retro Classic, Island Classic, Custom); the source game titles appear only in code
  comments and docs, never in the game UI or store page.
- Marketing that says "like Mario Maker 3" or uses their screenshots.

## Asset rules for this repo

- Every file in `assets/` must be made by you or a contractor under a written
  work-for-hire/assignment agreement, or bought under a license that allows
  commercial redistribution in a game. Record the source in
  `assets/CREDITS.md` (create when the first asset arrives).
- No AI-generated asset may be trained on or prompted to imitate Nintendo
  characters or styles; check the tool's commercial terms.
- Placeholder art is drawn in code (`platform/sdl2/art.c`): a plain generic
  hero and simple tiles, rendered differently per style. Replace with
  commissioned original art later.
