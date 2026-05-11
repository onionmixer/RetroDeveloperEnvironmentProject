#ifndef _AP_H
#define _AP_H

#include <stdint.h>

/*
 * ap-z88dk — aPLib decompressor (z88dk port)
 *
 * Port of `kingsvalley/src/ap/ap.z80` (SDCC asasm) to z88dk z80asm.
 * Original Z80 decompressor by Dan Weiss (Dwedit), adapted by utopian,
 * optimized by Metalbrain. License follows kingsvalley upstream.
 *
 * apultra (https://github.com/emmanuel-marty/apultra) is the optimal
 * compressor for the aPLib raw format. Compressed data is expected
 * to follow that on-wire format (no header — caller must know length
 * via convention; this routine streams until the format's terminator).
 */

/**
 * Decompress aPLib raw stream from `src` into `dst`.
 * Returns when the encoded stream's EOF marker is hit.
 */
void ap_uncompress(const uint8_t *dst, const uint8_t *src);

#endif /* _AP_H */
