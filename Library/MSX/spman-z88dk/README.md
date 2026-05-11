# spman-z88dk

z88dk-compatible source distribution of Juan J. Martinez's `spman` sprite
manager (originally part of `ubox-msx-lib`). Built atop the z88dk port at
`Library/MSX/ubox-msx-lib-z88dk/`.

## Layout

```
spman-z88dk/
├── include/spman.h     # Public API (sprite manager on top of ubox)
├── src/spman/spman.c   # Single-file C implementation (118 lines)
└── Makefile            # placeholder (see "Build strategy" below)
```

## Build strategy — direct source inclusion

Unlike `ubox-msx-lib-z88dk` (which is z80asm and packs into a `.lib`),
spman is a single C file that uses the `ubox_wait_vsync()` macro from
`ubox.h`. The macro expands to inline `__asm halt __endasm;` which
**sccz80 fails to parse when compiling spman.c standalone with `zcc -c`**
("Unknown symbol: __asm"). The same construct works fine when the file
is part of a full app compile.

So examples that need spman just **add `src/spman/spman.c` to their
SRCS list** and pass `-I include` (this dir's headers) + ubox-msx-lib-z88dk
paths. There is no separate `.lib` to link.

## Example usage

In `compile.sh` / `compile_dos.sh` of an example that needs spman
(currently `Examples/ubox_example_z88dk/examples/10_breakout`):

```bash
SPMAN_DIR="$PROJECT_ROOT/Library/MSX/spman-z88dk"
SPMAN_INC="$SPMAN_DIR/include"
SPMAN_SRC="$SPMAN_DIR/src/spman/spman.c"

ZCCFLAGS="$ZCCFLAGS -I$SPMAN_INC"
SRCS="$SPMAN_SRC src/main.c ..."
```

## API surface (from `include/spman.h`)

`spman_init`, `spman_alloc_pat`, `spman_alloc_sprite`,
`spman_alloc_fixed_sprite`, `spman_sprite_flush`, `spman_update`,
`spman_hide_all_sprites`.

## License

MIT (Juan J. Martinez, 2020-2021).
