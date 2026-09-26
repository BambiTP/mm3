# Physics styles

Nine styles. The first five follow the **level-maker versions** of their
source games (Super Mario Maker 2), where the four 2D styles share one physics
engine and differ only in moves and rules. Two **Classic** styles keep the
exact original-game engines as bonus options.

| Key | Public name | Source (internal only) | Engine | Accuracy |
|---|---|---|---|---|
| 1 | **Retro** | SMM2 SMB1 style | `engine_modern.c`, shared maker profile | maker rules documented; numbers partly exact |
| 2 | **Arcade** | SMM2 SMB3 style | same shared maker profile | same |
| 3 | **Island** | SMM2 SMW style | same shared maker profile | same |
| 4 | **Modern** | SMM2 NSMBU style | same shared maker profile | same |
| 5 | **Athletic** | SMM2 3D World style | `engine_modern.c`, own profile | **Estimated** |
| 6 | **Bloom** | Super Mario Bros. Wonder (extra) | `engine_modern.c`, own profile | **Estimated** |
| 7 | **Retro Classic** | original Super Mario Bros. (NES) | `engine_retro.c` | **Exact** |
| 8 | **Island Classic** | original Super Mario World (SNES) | `engine_island.c` | **Exact** |
| 9 | **Custom** | — | `engine_modern.c` | maker numbers, every move enabled |

## Maker styles (1-4)

In the maker game, the SMB1, SMB3, SMW and NSMBU styles all use the same
NSMBU-derived physics; only the move set changes per style. MM3 does the same:
Retro, Arcade, Island and Modern share one profile (`MAKER_2D` in
`physics_profiles.c`).

Shared numbers:

| Value | Number | Confidence |
|---|---|---|
| Jump launch, speed bonus, gravity bands, max fall | as NSMB Wii (below) | Exact (from the NSMBW decomp; SMM2 is NSMBU-based) |
| Walk / run / P-speed | 1.5 / 2.5 / 3.0 px/f | Estimated |
| Ground / air acceleration | 0.06 / 0.035 px/f² | Estimated |
| P-meter | 56 frames at run speed to fill | Estimated |
| Slowfall gravity (jump held while falling) | 0.25 px/f² | Estimated |

Shared rules, from fan documentation of the maker game (Kaizo Mario Maker
wiki, "Mario Maker 2 momentum physics"):

- **Speed locks in the air**: you can't go faster in the air than at takeoff
  (you can still brake and turn).
- **P-meter** fills only on the ground while running at top speed; it drains
  in the air unless already full. Full meter = P-speed.
- Releasing run on the ground slows you back to walking speed.
- Air acceleration is lower than on the ground.
- **Slowfall**: holding jump while falling accelerates downward more slowly.

Moves per maker style:

| Move | Retro | Arcade | Island | Modern |
|---|---|---|---|---|
| Walk, run, P-speed, variable jump, slowfall | ✓ | ✓ | ✓ | ✓ |
| Wall slide + wall jump | ✓ | ✓ | ✓ | ✓ |
| Crouch, crouch jump, slide down slopes | ✓ | ✓ | ✓ | ✓ |
| Swim, climb vines (hold ↑) | ✓ | ✓ | ✓ | ✓ |
| Kick a crate by touching it | ✓ | ✓ | ✓ | ✓ |
| Carry and throw (hold run) | – | ✓ | ✓ | ✓ |
| Toss up (↑ while releasing run) | – | – | ✓ | – |
| Spin jump (also off vines) | – | – | ✓ | ✓ |
| Mid-air twirl, ground pound, triple jump | – | – | – | ✓ |

**Arcade (SMB3 style):** in the maker game SMB3 style moves exactly like the
others; its differences are the move set (carry and kick) and items that
aren't in MM3 yet (power-ups). The original SMB3's own numbers, for a possible
future "Arcade Classic": walk speed `0x18`, run `0x28`, P-speed `0x38`
(1/16 px per frame), from Data Crystal's SMB3 notes; the full disassembly is
[captainsouthbird/smb3](https://github.com/captainsouthbird/smb3).

## Shared jump height

**Every style, including the Classics, reaches the same jump heights in real
play.** The height depends on what you're doing, not on each style's raw
speed:

| Jump | Height (feet rise) |
|---|---|
| Standing | 67.6 px (4.2 tiles) |
| Walking | 78.6 px |
| Running | 82.4 px |
| Full speed (P-speed or dash) | 86.3 px |
| Tapped (standing) | 20.5 px |

Bloom and Retro Classic have no full-speed state, so their top jump is the
running jump.

The modern engine picks a jump tier from the movement state (standing, walking
up to walk speed, running faster than that, full P-meter or dash). The
Classic engines keep their own speed tiers and gravity, but their launch
speeds were re-tuned (see their sections). Only the arc changes per style:

| Style | Gravity vs maker | Airtime of a standing jump |
|---|---|---|
| Retro / Arcade / Island / Modern / Custom | 1.0x | 54 frames |
| Athletic | 1.3x (snappy) | 47 frames |
| Bloom | 0.65x (floaty) | 64 frames |
| Retro Classic / Island Classic | original gravity | original feel |

All launch numbers come from `tools/calibrate_jump.c` (build with
`-DMM3_BUILD_TOOLS=ON`). `tests/test_moves.c` plays standing, walking,
running, full-speed and tapped jumps with real inputs in all 9 styles and
checks each is within 1 px of the maker height (today: within 0.9 px; the
Classic engines move in whole pixels or 1/16 px so they can't be finer).

## Athletic (maker 3D World style, 5)

Its own engine in the maker game, with the biggest move set: dash (after
about 1.5 s of running), **long jump** (run, then run + action button, or
run + ↓ + jump), **backflip** (crouch about 1 s, then jump), **crawl**, **roll**
(action button while crouched), ground pound, slide, carry, wall jump. All
numbers Estimated.

Confidence labels used below:

- **Exact**: copied from a community disassembly or decompilation of the
  source game, and verified by `tests/test_moves.c`.
- **Estimated**: no public data exists; tuned to match the feel and move set.

Collision (tiles, slopes, water, vines) is shared by all engines and is *not*
a copy of any original: the originals used different collision systems, and
levels here have slopes and features some originals never had.

## Units

All engines use **1/4096 px** (Q20.12) for positions and speeds, at 60 ticks
per second. That unit stores all the source values without rounding:

| Source | Native unit | In our units |
|---|---|---|
| 8-bit (SMB1) | speed byte = 1/16 px/frame + 1/256 fraction | ×256 per speed unit |
| 16-bit (SMW) | "4.12" speed word; high byte = 1/16 px/frame | identical (1 : 1) |
| HD (NSMBW) | float px/frame | ×4096, rounded |

## Sources

- **SMB1:** doppelganger's comprehensive disassembly, as maintained at
  [Xkeeper0/smb1](https://github.com/Xkeeper0/smb1) (`src/prg.asm`:
  `PlayerPhysicsSub`, `X_Physics`, `ImposeFriction`, `MovePlayerHorizontally`,
  `ImposeGravity`, `ClimbingSub`, and the tables `JumpMForceData`,
  `FallMForceData`, `PlayerYSpdData`, `InitMForceData`, `MaxRightXSpdData`,
  `FrictionData`, `Climb_Y_SpeedData`, `Climb_Y_MForceData`, `ClimbAdderLow`).
- **SMW:** [IsoFrieze/SMWDisX](https://github.com/IsoFrieze/SMWDisX)
  (`bank_00.asm`, NTSC tables: `DATA_00D2BD` jump speeds, `MarioAccel_`,
  `DATA_00D535` max speeds, `DATA_00D2CD` friction, `DATA_00D7A5`/`DATA_00D7AF`
  gravity and fall cap, `DATA_00D5EB` P-meter steps, `DATA_00D984` swim rise
  caps, `CODE_00D9EB` swimming, `CODE_00CD24` frame order), plus measurements
  of the original game provided by the project owner (see test targets below).
- **NSMBW:** [NSMBW-Community/NSMBW-Decomp](https://github.com/NSMBW-Community/NSMBW-Decomp)
  (`d_a_player_base.cpp`: `sc_JumpSpeed`, `sc_MaxFallSpeed`, water constants,
  `setButtonJumpGravity`/`setNormalJumpGravity`; `d_a_player.cpp`:
  `smc_POWER_CHANGE_DATA` gravity bands and jump-speed bonuses, `getJumpSpeed`,
  `setJumpSpeed` (third jump ×1.05)). The walk/run speed table lives in code
  that isn't decompiled yet, so those values are estimates.
- **3D World, Wonder:** no decompilation or datamine is public. Move sets come
  from the games' manuals and wikis; numbers are estimates.

## 7 · Retro Classic (original 8-bit engine)

Exact original, **except jump launch speeds and tier-0 fall force**, which
were re-tuned so its jumps match every other style. Original launch:
−4 px/f (tiers 0-2), −5 px/f (tiers 3-4); tier-0 fall force `0x70`.


Movement is a line-by-line port of the original's player routine:

- Horizontal speed is a signed byte in 1/16 px/frame plus an 8-bit fraction.
  The fraction byte is **shared** between acceleration and sub-pixel position,
  exactly like the original. Speed is capped by snapping, not easing.
- Rows: **run** max `0x28` (2.5 px/f), accel `0xE4`; **walk** max `0x18`
  (1.5 px/f), accel `0x98` (or `0xD0` above speed `0x21`); **water** max
  `0x10`. Acceleration doubles when facing ≠ moving direction (turning).
- B-button run keeps working for 10 frames after release (`RunningTimer`).
- Below speed `0x0B`, pressing the other way turns you around instantly.
- **Jump:** 5 tiers by |speed| (`<0x09`, `<0x10`, `<0x19`, `<0x1C`, else).
  Launch Y speed −4 or −5 px/f. Rising gravity `0x20/0x20/0x1E/0x28/0x28`
  while A is held; fall gravity `0x70/0x70/0x60/0x90/0x90` after release or at
  the apex. Max fall 4 px/f. No jump buffering, no coyote time.
- **No mid-air turning**: facing is locked in the air; air speed row depends
  on speed at the time (≥ `0x19` keeps the run cap).
- **Crouch** (big form), **crouch jump** keeps the crouch. Down + left/right
  on the ground cancels both (original quirk).
- **Swim:** strokes use tier 5 (`−2` px/f, gravity `0x0D`, fall `0x0A`).
  Near the surface gravity becomes `0x18`. *MM3 addition:* jumping at the
  surface leaves the water (the original's water levels had no surface).
- **Vines:** up −7/8 px/f, down ≈2 px/f (exact). Changed from the original
  because it felt bad: you grab a vine by holding ↑ instead of on touch, you
  slide sideways smoothly instead of the original's 24-frame side hops, and
  you can jump off.

Verified: walk top speed `0x18`, run `0x28`, standing full jump ≈4 tiles,
running full jump ≈5 tiles, no air turning.

## 8 · Island Classic (original 16-bit engine)

Exact original, **except the jump table and release gravity**, which were
re-tuned so its jumps match every other style (new normal launch
−82/−88/−88/−90/−90/−93/−93/−93, spin −73, release gravity 11). The
original numbers and the measurements below are kept for reference.


- Frame order matches the original: move with last frame's speed and collide,
  **then** read input and compute new speeds.
- Acceleration **1.5** units/frame (`0x180`); skid **2.5** walking / **5**
  running; friction **1** unit/frame on the ground; **no friction in the air**
  (releasing the direction in mid-air freezes your speed).
- Caps: walk **20**, run **36**, sprint **48**. Because speed below the cap
  accelerates by 1.5 and speed at/over it brakes by 1, speed oscillates just
  above the cap: walk peaks at **21**, sprint cycles **47-48-49**.
- **P-meter**: +2 per frame while running at ≥ 35 units on the ground, −1
  otherwise, full at 112. Full meter unlocks the sprint cap.
- **Jump** launch (high byte) from `DATA_00D2BD`, indexed by `(|speed| >> 2) & ~1`;
  spin jump uses the odd column. Gravity **3** while B or A is held, **6**
  otherwise, applied to the high byte; fall speed is capped at **64** *before*
  gravity is added (so the true max is 67 / 70).
- **Spin jump** (A / C key), can't spin while carrying.
- **Duck**, duck jump (keeps the duck), **look up**, **duck-slide on slopes**.
- **Carry**: hold Y (X key) to grab a crate; release to kick (`0x2E`), hold ↑
  to toss up (`0x70`), hold ↓ to set it down.
- **Swim**: stroke −`0x20`, sinking +2 every 4th frame, rise caps −24 / −8
  (↓) / −48 (↑), sink cap 64; water speed caps 8 on the floor, 16 swimming.
- **Climb**: 8 units/frame, 16 holding run; jump off with −`0x50`.

Verified against measurements of the original (project owner's data):

| Measurement | Original | MM3 |
|---|---|---|
| Walk / sprint peak speed | 21 / 49 | 21 / 49 (47-49 cycle) |
| P-meter full from standstill | ≈ 80 frames | 80 |
| Reach speed 49 | ≈ 90 frames | 91 |
| Launch speed at 0/21/37/49 | 77 / 82 / 87 / 92 | 77 / 82 / 87 / 92 |
| Spin launch at 0/21/37/49 | 71 / 75 / 79 / 84 | 71 / 75 / 79 / 84 |
| Running jump height | 5 tiles | 5.1 |
| Sprint jump height | 6 tiles (barely) | 5.7 feet-rise (+ landing tolerance) |
| Spin jump height (run / sprint) | 4 / 5 | 4.2 / 4.5 |
| Minimum jump | 2 tiles | 2.1 |
| Sprint jump length | ≈ 12 tiles | 12.3 |
| Sprint spin jump length | 11 | 10.8 |
| Walking jump length | 5 (barely) | 4.5 |

## Modern engine details

One engine, driven by `mm3_modern_profile` (see `physics_profiles.c`).
Gravity uses the HD-era scheme from the NSMBW decomp: the acceleration depends
on the current vertical-speed band and whether jump is held.

| Band (upward speed >) | 2.5 | 1.5 | 0.3 | −0.12 | −3.0 | else |
|---|---|---|---|---|---|---|
| Jump held | 0.06 | 0.25 | 0.34 | **0.08** (floaty apex) | 0.31 | 0.34 |
| Released | 0.34 | 0.34 | 0.34 | 0.25 | 0.34 | 0.34 |

### NSMB Wii source values (used by the maker profile)

| Value | Number | Confidence |
|---|---|---|
| Jump launch | 3.628 px/f | Exact |
| Jump bonus by speed (<0.7, <1.5, <2.8, faster) | +0, +0.18, +0.24, +0.30 | Exact |
| Third consecutive jump | ×1.05 | Exact |
| Gravity bands | table above | Exact |
| Max fall | 4.0 px/f | Exact |
| Water: swim speed / stroke / max fall | 1.125 / 1.25 / 3.0 | Exact (constants) |
| Turn deceleration | 0.12 | Exact (value), usage inferred |
| Walk / run top speed | 1.5 / 3.0 px/f | Estimated |
| Accel, friction, air control | 0.06 / 0.10 / 0.04 | Estimated |
| Wall slide, wall kick, pound, twirl, spin jump | see profile | Estimated |

Moves: walk/run, **double & triple jump** (chain within 10 frames of landing),
**spin jump** (C on ground), **twirl** (C in air, once per jump), **wall
slide + wall jump**, **ground pound** (↓ in air: windup, fall, landing lag),
crouch, crouch jump, **crouch slide**, swim, climb, carry/throw.

### Bloom (Wonder) — Estimated

Lower top speed, strong friction (stops quickly), floatier jump, slower max
fall, generous coyote time (6 frames) and jump buffer (8 frames). Moves: spin
jump, twirl, wall jump, ground pound, crouch jump, crouch slide, swim, climb,
carry. Planned: badge-style modifiers (profile overlays).

### Custom

Starts from Modern's numbers with **every** move flag enabled (including
long jump, backflip, side flip, dash). Every field of `mm3_modern_profile`
will be editable in the level editor and saved in the level.

## Move matrix

R = Retro, Ar = Arcade, I = Island, M = Modern, At = Athletic, B = Bloom,
RC = Retro Classic, IC = Island Classic, C = Custom.

| Move | R | Ar | I | M | At | B | RC | IC | C |
|---|---|---|---|---|---|---|---|---|---|
| Walk / run / skid / variable jump | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| P-speed / dash | P | P | P | P | dash | – | – | P | both |
| Air speed lock, slowfall | ✓ | ✓ | ✓ | ✓ | – | – | lock (orig.) | – | slowfall |
| Wall slide + wall jump | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | – | – | ✓ |
| Crouch, crouch jump | ✓ | ✓ | ✓ | ✓ | crouch | ✓ | ✓ | ✓ | ✓ |
| Slide down slopes | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | – | ✓ | ✓ |
| Kick crate on touch | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | – | – | ✓ |
| Carry / throw | – | ✓ | ✓ | ✓ | ✓ | ✓ | – | ✓ | ✓ |
| Toss up | – | – | ✓ | – | – | – | – | ✓ | ✓ |
| Spin jump | – | – | ✓ | ✓ | – | ✓ | – | ✓ | ✓ |
| Twirl, ground pound | – | – | – | ✓ | pound | ✓ | – | – | ✓ |
| Triple jump | – | – | – | ✓ | – | – | – | – | ✓ |
| Long jump, backflip, crawl, roll | – | – | – | – | ✓ | – | – | – | ✓ |
| Side flip | – | – | – | – | – | – | – | – | ✓ |
| Swim, climb vines (hold ↑) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Look up | – | – | – | – | – | – | – | ✓ | – |

Not yet included: power-up forms and their moves (e.g. flight), enemies and
stomping, spin-bouncing off enemies, Yoshi-style mounts, and the Wonder
badges. These need entities that the engine doesn't have yet.

## Adding or changing a style

1. Modern-engine style: add a profile in `physics_profiles.c` and an enum
   value in `physics.h`. New move → new `MM3_MOVE_*` flag, implemented in
   `engine_modern.c` behind that flag.
2. Run `test_determinism --print` and paste the new golden hashes.
3. Add checks to `tests/test_moves.c` and update this document.
4. After release, never edit a shipped profile in place: add a new version so
   old levels and replays keep playing identically.
