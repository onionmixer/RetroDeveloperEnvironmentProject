# mplayer-z88dk — real Arkos 2 AKM player port for z88dk

z88dk-buildable port of Juan J. Martinez's mplayer (Arkos 2 AKM player wrapper)
without Disark. Replaces the no-op `Library/MSX/mplayer-z88dk-stub/`.

작성: 2026-04-29 (Phase 3-C real port).

## How it works (binary-embedding + LDIR strategy)

The Arkos 2 `PlayerAkm.asm` is ~2300 lines of rasm-syntax z80 assembly with
absolute internal `call` / `jp` instructions. Porting the source to z88dk's
`z80asm` macro syntax is a multi-iteration project; instead we do this:

1. **rasm assembles the AKM player+song+effects to a binary blob at a fixed
   ORG ($A000).** The blob contains baked-in absolute addresses pointing at
   $A000-range. Per-example builds run rasm on the example's `src/akm.z80`
   (modified in-place to add `ORG #A000` at the top).
2. **z88dk INCBINs the blob** (via `BINARY "akm.bin"`) into the application's
   `code_user` section. The application places the blob wherever its
   linker decides — typically inside cart ROM ($4000+) or DOS TPA ($0100+).
3. **A small loader function** (`mplayer_engine_load`, defined in
   `src/akm/akm_bridge.asm`) runs once at startup and `LDIR`-copies the
   blob to $A000 RAM. After that, AKM internal jumps target $A000+ correctly.
4. **AKM entry-point symbols** (`_PLY_AKM_INIT`, `_PLY_AKM_PLAY`, etc.) are
   defined as `DEFC` constants at `AKM_BASE + offset` in the bridge file.
   The C-side `mplayer_init` etc. wrappers (in `src/wrappers/`) call those
   constants via z80 `call` / `jp`.
5. **Per-example song/effects symbols** (`_SONG`, `_EFFECTS`) are extracted
   from the rasm-emitted symbol table by each example's `compile.sh` and
   written into a tiny generated `song_bindings.asm` linked alongside.

## Layout

```
mplayer-z88dk/
├── include/mplayer.h              # API (verbatim from upstream)
├── src/wrappers/
│   ├── mplayer_init.asm           # 8 wrappers, sccz80 calling convention,
│   ├── mplayer_init_effects.asm   #   converted from upstream SDCC asasm
│   ├── mplayer_is_sound_effect_on.asm
│   ├── mplayer_play.asm
│   ├── mplayer_play_effect.asm
│   ├── mplayer_play_effect_p.asm
│   ├── mplayer_stop.asm
│   └── mplayer_stop_effect_channel.asm
├── src/akm/
│   └── akm_bridge.asm             # INCBIN blob + LDIR loader + AKM symbol DEFC
├── lib/                           # (placeholder — no .lib build)
└── README.md (this file)
```

## Why $A000?

MSX memory map under typical 16 KiB cartridge boot:
- $0000-$3FFF: BIOS ROM (slot 0)
- $4000-$7FFF: cartridge ROM (cart slot)
- $8000-$BFFF: **RAM** (page 2 mapped to primary slot, default RAM)
- $C000-$FFFF: RAM

$A000 lives squarely in the RAM page 2 zone, away from cart ROM and
away from PLY_AKM_ROM_Buffer ($C000) which is AKM's internal var area.
Under MSX-DOS too, page 2 is RAM (the entire address space below BIOS
is usable RAM). So $A000 works for both ROM cart and DOS .COM modes.

For 32 KiB cartridges the cart ROM extends through $8000-$BFFF — those
would clash with our $A000 placement. The current ubox_example_z88dk
ports all fit in 16 KiB so this isn't an issue; future examples that
need 32 KiB cart should pick a different `AKM_BASE` (e.g. $C200 just
above the AKM ROM buffer, leaving $C000-$C199 untouched).

## Per-example build flow (used by 05_music, 06_sound)

In each example's `compile.sh` / `compile_dos.sh`:

```bash
# 1. rasm assembles src/akm.z80 (which `include`s the example's per-song
#    asm files, akm_ubox, and via that chain PlayerAkm.asm). The .z80
#    file has been edited in-place to add `ORG #A000` at the top.
$RASM src/akm.z80 -ob $BUILD/akm.bin -os $BUILD/akm.sym -s -sl

# 2. Copy bridge.asm into $BUILD so its `BINARY "akm.bin"` finds the file
#    in its own directory.
cp $MPLAYER_DIR/src/akm/akm_bridge.asm $BUILD/akm_bridge.asm

# 3. Auto-generate song_bindings.asm — extract _SONG (and _EFFECTS if
#    present) addresses from akm.sym.
SONG_ADDR=$(awk '$1=="SONGDISARKGENERATEEXTERNALLABEL"{print $2}' $BUILD/akm.sym)
# convert "#A616" → "$A616" and emit `DEFC _SONG = $A616` etc.

# 4. zcc compiles main.c + 8 wrappers + bridge.asm + song_bindings.asm
$ZCC $ZCCFLAGS -m -o $BUILD/$TARGET \
    src/main.c \
    $WRAPPER_DIR/mplayer_*.asm \
    $BUILD/akm_bridge.asm \
    $BUILD/song_bindings.asm

# 5. C-side calls `mplayer_engine_load()` once before any mplayer_*
#    function (in main.c / main_dos.c).
```

## Verified end-to-end

| Example | ROM | DOS .COM | Notes |
|---------|-----|----------|-------|
| 05_music | 16,384 B | 8,928 B | alienall song embedded; _SONG=$A616 |
| 06_sound | 16,384 B | 7,568 B | song + 5 effects; _SONG=$A440, _EFFECTS=$A610 |

Build verified: all symbols resolve, AKM blob at ROM $4193+ (or DOS
TPA $0100+), LDIR'd to $A000 at startup, internal jumps land correctly.
**Runtime music output verification pending** (user must boot in openMSX
and listen).

## Limitations / future work

- ubox_wait_vsync() macro from `Library/MSX/ubox-msx-lib-z88dk/include/ubox.h`
  still trips sccz80 — see `Library/MSX/spman-z88dk/README.md`. Independent fix.
- AKM blob is copied into RAM at startup (~3 KiB memcpy). Slight cold-start
  cost. For DOS this is fine; for ROM mode could be eliminated by placing
  the blob directly in cart ROM at the assembled address (would require
  z88dk linker section pinning — moderately invasive).
- `PLY_AKM_Rom = 1` config (no self-modification) is set in
  `Examples/ubox_example_z88dk/src/mplayer/akm/akm_ubox.asm`. Since we LDIR
  to RAM, we COULD switch to `PLY_AKM_Rom = 0` for slightly faster (and
  smaller buffer) operation; not done in this port for safety.

## License

Wrappers + bridge: MIT (matching upstream mplayer license).
PlayerAkm.asm (assembled into the blob): see
`Examples/ubox_example_z88dk/src/mplayer/akm/LICENSE.txt` (Arkos Tracker
2 / Targhan, MIT-equivalent terms).
