# Physics styles

Every style is a `mm3_physics_profile` (`core/include/mm3/physics.h`): a list
of numbers plus ability flags. Player code only reads the profile — it never
asks "which style is this?" — so any mix of settings is possible, which is
what the **Custom** style exposes to players.

All numbers are **original placeholder values** tuned by feel, not data taken
from any existing game. Expect heavy retuning during playtesting.

## Units

| Quantity | Unit | Example |
|---|---|---|
| Position | subpixel (1/256 px) | 16 px tile = 4096 |
| Speed | subpixels per frame | 256 = 1 px/frame = 60 px/s |
| Acceleration | subpixels per frame² | 32 = 0.125 px/frame² |
| Time | frames at 60 Hz | 8 frames ≈ 133 ms |

Jump height ≈ `jump_vel² / (2 × gravity_hold)` subpixels.
Example: 1024² / 64 = 16384 subpx = 64 px = 4 tiles.

## The styles

### 1 · Classic — "commit to your jump"
- Heavy momentum: slow acceleration, long skids.
- **Air control locked**: in the air you can't exceed the speed tier you
  jumped with, and turning around is slow.
- Holding jump gives low gravity while rising; releasing switches to heavy
  gravity (variable jump height).
- Running faster before takeoff gives a higher jump (3 speed tiers).
- No coyote time, no jump buffer, no wall jump.

### 2 · New — "modern and responsive"
- Snappier acceleration, free air turning.
- **Wall slide + wall jump**, **ground pound** (Down in the air),
  **spin jump** (C: lower jump, slower fall).
- Small coyote time (4 f) and jump buffer (4 f).

### 3 · World — "tight and athletic"
- Fastest acceleration and top speed, quick stops.
- **Long jump** (run at speed + hold Down + Jump), wall jump, ground pound.
- Crouch jump and short wall climb are flagged but **not implemented yet**.
- Coyote 5 f, buffer 6 f.

### 4 · Wonder — "friendly and floaty"
- Lower top speed and momentum, strong friction (stops quickly).
- Floatier jumps and slow max fall.
- Very generous coyote time (8 f) and jump buffer (8 f).
- Wall jump, ground pound.
- Planned: **modifier overlays** ("badges"): small sets of deltas applied on
  top of a profile, e.g. +floaty, +wall-climb, +higher-jump.

### 5 · Custom — "your rules"
- Starts as a copy of New with long jump enabled.
- Every field in the table below will be editable in the level editor and
  saved inside the level.

## Profile fields

| Field | Meaning | Classic | New | World | Wonder |
|---|---|---|---|---|---|
| walk_max | top speed, RUN not held | 384 | 400 | 440 | 380 |
| run_max | top speed, RUN held | 640 | 704 | 736 | 600 |
| walk_accel / run_accel | ground acceleration | 14 / 22 | 16 / 24 | 24 / 30 | 20 / 24 |
| air_accel | air acceleration | 14 | 16 | 24 | 22 |
| friction | slowdown, no direction held | 13 | 16 | 24 | 30 |
| skid_decel | slowdown, pushing against motion | 26 | 32 | 40 | 40 |
| gravity_hold | rising, jump held | 32 | 28 | 30 | 26 |
| gravity_release | rising, jump released | 96 | 80 | 90 | 80 |
| gravity_fall | falling | 96 | 64 | 72 | 60 |
| max_fall | terminal velocity | 1152 | 1024 | 1100 | 900 |
| jump_vel[3] | launch speed per speed tier | 1024/1056/1120 | 1000/1040/1100 | 1000/1030/1080 | 980/1000/1040 |
| jump_tier_speed[2] | speed thresholds for tiers | 256/512 | 256/560 | 256/600 | 256/480 |
| coyote_frames | late-jump grace | 0 | 4 | 5 | 8 |
| jump_buffer_frames | early-press grace | 0 | 4 | 6 | 8 |
| abilities | move set | — | wall, pound, spin, air-turn | wall, pound, long, (crouch), (climb), air-turn | wall, pound, air-turn |

## Adding a style

1. Add an enum value before `MM3_STYLE_COUNT` in `physics.h`.
2. Add a row to `k_profiles` and a name to `k_names` in `physics_profiles.c`.
3. If it needs a new move, add an `MM3_ABIL_*` flag and implement it in
   `player.c`, gated on that flag.
4. Update golden hashes (`test_determinism --print`) and this document.
