# ap-z88dk

aPLib raw-stream decompressor, ported to **z88dk z80asm** for use with
sccz80 + the rest of `Library/MSX/*-z88dk` libraries.

## Source lineage

The Z80 decompressor itself is unchanged from the original:

- Original Z80 ApLib decompressor: **Dan Weiss (Dwedit)**
- Adapted: **utopian**
- Optimized: **Metalbrain**

The SDCC asasm version lives in
`Examples/kingsvalley_z88dk/src/ap/ap.z80` (and upstream
`Library/MSX/ubox-msx-lib-1.2.0/src/ap/ap.z80`). This port translates only
the SDCC-asasm syntax wrapping (`.globl`, `_label::`, `#imm`, `.ez80`)
into z80asm equivalents (`PUBLIC`, `_label:`, plain immediates,
no directive needed) and adjusts the C entry's stack extraction for
sccz80 caller-cleanup. The actual decompression body is byte-identical.

## Build

```bash
cd Library/MSX/ap-z88dk
make
# → lib/ap.lib
```

## Use

```c
#include <ap.h>

extern const uint8_t compressed_data[];
uint8_t out[1024];

ap_uncompress(out, compressed_data);
```

z88dk link: `-Lpath/to/ap-z88dk/lib -lap`.

## Calling conventions

Two entry points, identical body:

| Entry | Convention | Inputs |
|-------|------------|--------|
| `_ap_uncompress` | sccz80 caller-clean | `dst` (sp+4..5), `src` (sp+2..3) |
| `ap_uncompress` | register | HL = src, DE = dst |

Caller-side push order verified empirically with sccz80 on 2026-05-10
(see `PLAN_MIGRATION_KINGSVALLY.md` Iter 1).

## Notes

- Body uses `IXL/IXH/IYL/IYH` (undocumented Z80 opcodes); z88dk z80asm
  accepts these by default — no `--undocumented` flag required.
- aPLib raw format is *headerless* — caller must know decompressed
  length out of band (kingsvalley stores it as a 2-byte little-endian
  prefix at `src - 3` and passes `src + 3` here).
- Tested against existing kingsvalley `apultra`-compressed assets.
