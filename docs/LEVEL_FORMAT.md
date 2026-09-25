# Level and replay file format (draft v1)

Not implemented yet — this is the design the code will follow. The core owns
parsing and writing, operating on byte buffers, so every platform reads and
writes identical files. Frontends only move bytes to and from disk/network.

## General rules

- All integers **little-endian**, fixed width. No raw struct dumps.
- Every file starts with a magic number and a format version.
- Unknown chunks are skipped (forward compatibility).
- A CRC32 over the payload detects corruption.

## Level file (`.mm3l`)

```
Header (16 bytes)
  u32  magic        'M','M','3','L'
  u16  version      1
  u16  flags
  u32  payload_size
  u32  crc32        of payload

Payload = sequence of chunks:
  u32  chunk_id (4 ASCII chars)
  u32  chunk_size
  ...  chunk data
```

| Chunk | Contents |
|---|---|
| `META` | title (UTF-8, length-prefixed), author id, created time (informational only; never used by the sim), theme id |
| `PHYS` | style id (u8) + the **full physics profile by value** (every field as i32/u32 in declaration order) + profile version |
| `TMAP` | width, height (u16 each), then tiles (u8 each, `mm3_tile` values: empty, solid, one-way, six slope shapes, water, vine), row-major; RLE-compressed |
| `ENTS` | entity count, then per entity: type (u16), x, y (i32 subpixels), params |
| `RULE` | clear condition, timer, start seed |

Storing the profile **by value** means a level always plays exactly the way it
did when it was uploaded, even after built-in styles are retuned.

## Replay file (`.mm3r`)

```
Header: magic 'M','M','3','R', version, flags, payload_size, crc32
  u32  level_crc32     which level (and exact version) this replay is for
  u32  seed
  u32  frame_count
  u32  final_hash      mm3_state_hash() at the end — for verification
  ...  inputs: run-length encoded (u16 buttons, u16 repeat_count) pairs
```

A server (or another player) can verify a clear or a speedrun time by
re-simulating the replay and comparing `final_hash`.
