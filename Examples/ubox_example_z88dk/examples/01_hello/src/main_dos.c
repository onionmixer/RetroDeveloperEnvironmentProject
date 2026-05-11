/*
 * 01_hello — MSX-DOS2 variant.
 * Same screen output as the ROM build (../src/main.c), with two
 * differences:
 *   1. ubox_init_isr() is skipped — under MSX-DOS2 the HTIMI hook is
 *      called with BIOS ROM paged in on page 0, making ISR code at
 *      $0100+ (TPA) unreachable.
 *   2. ubox_wait() is overridden below to use halt-based polling
 *      instead of the library version (which depends on the ISR).
 *
 * Built with z88dk (+msx -subtype=msxdos2 -DMSXDOS). See
 * ../compile_dos.sh.
 */

#include <ubox.h>

#define LOCAL
#include "tiles.h"

#define WHITESPACE_TILE 129

void put_text(uint8_t x, uint8_t y, const uint8_t *text)
{
    while (*text)
        ubox_put_tile(x++, y, *text++ + 128 - 31);
}

int main(void)
{
    /* DOS: skip ubox_init_isr() — ISR unreachable from BIOS context */

    ubox_set_mode(2);
    ubox_set_colors(1, 1, 1);
    ubox_disable_screen();

    ubox_set_tiles(tiles);
    ubox_set_tiles_colors(tiles_colors);

    ubox_fill_screen(WHITESPACE_TILE);
    put_text(11, 11, "HELLO WORLD!!");

    ubox_enable_screen();

    while (1) {
        ubox_wait();   /* uses the override below */
    }
    return 0;          /* unreachable */
}

/*
 * DOS-compatible replacement for ubox_wait().
 * The library version (ubox_wait.asm) relies on the HTIMI ($FD9F)
 * hook which fails under MSX-DOS2 because the hook fires with BIOS
 * ROM paged on page 0, hiding TPA. Two halts approximate the
 * library's "wait 2 ticks" default.
 */
void ubox_wait(void)
{
    __asm
    halt
    halt
    __endasm;
}
