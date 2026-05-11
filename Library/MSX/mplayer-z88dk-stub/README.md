# mplayer-z88dk-stub

**Silent stub** of Juan J. Martinez's mplayer / Arkos 2 AKM player for use
under z88dk. All API entrypoints exist as no-op functions and the data
symbols (`SONG`, `EFFECTS`) are empty arrays. Lets `Examples/ubox_example_z88dk/examples/{05_music,06_sound}` (and any other AKM-using code) link and run with no audio output.

## Why a stub?

The real Arkos 2 AKM player (`Examples/ubox_example_z88dk/src/mplayer/akm/PlayerAkm.asm`,
~2000 lines, rasm-macro dependent z80asm) does not currently have a z88dk
port. Porting it requires either:

- rewriting PlayerAkm.asm + akm_ubox.asm into z88dk's `z80asm` macro
  syntax, and likewise the Disark-emitted song/effects data files,
- **or** building `rasm` + `Disark` in-tree and chaining them so that the
  resulting SDCC-asm form becomes `z80asm`-compatible (still a separate
  pipeline shim).

Either path is a multi-iteration project. Until that lands, this stub
keeps the build matrix complete (12/12) at the cost of silent music.

## What you get

| Symbol | Type | Behaviour |
|--------|------|-----------|
| `mplayer_init` | function | no-op |
| `mplayer_init_effects` | function | no-op |
| `mplayer_play` | function | no-op (would tick the player each frame) |
| `mplayer_stop` | function | no-op |
| `mplayer_play_effect` / `..._p` | function | no-op |
| `mplayer_stop_effect_channel` | function | no-op |
| `mplayer_is_sound_effect_on` | function | returns 0 (always off) |
| `SONG[]` | array | `{ 0 }` |
| `EFFECTS[]` | array | `{ 0 }` |

## Layout

```
mplayer-z88dk-stub/
├── include/mplayer.h                 # Verbatim copy of upstream header
├── src/mplayer/
│   ├── mplayer_stub.c                # 8 no-op API functions
│   └── song_data_stub.c              # Empty SONG / EFFECTS arrays
├── Makefile                          # placeholder (no .lib build)
└── README.md
```

## Use from an example

```bash
MPSTUB_DIR="$PROJECT_ROOT/Library/MSX/mplayer-z88dk-stub"
MPSTUB_INC="$MPSTUB_DIR/include"
MPSTUB_SRC="$MPSTUB_DIR/src/mplayer/mplayer_stub.c $MPSTUB_DIR/src/mplayer/song_data_stub.c"

ZCCFLAGS="$ZCCFLAGS -I$UBOX_INC -I$MPSTUB_INC -I$SCRIPT_DIR/src"
SRCS="src/main.c $MPSTUB_SRC"
```

The example does NOT need the upstream `alienall.asm` / `effects.asm`
data files — the stub provides empty `SONG` / `EFFECTS` arrays.

## Future replacement

When the real AKM port lands at `Library/MSX/mplayer-z88dk/`, examples
just swap `MPSTUB_DIR` for `MPLAYER_DIR` and link against the real
implementation; the C-side API surface is identical.

## License

MIT (matches upstream mplayer license; no Arkos 2 AKM source is
distributed in this stub).
