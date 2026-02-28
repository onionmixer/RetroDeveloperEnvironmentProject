/* 01_hello example ported to z88dk ROM build
 * Original: resource/MSX/ubox_example/examples/01_hello
 * Tests: set_tiles, set_tiles_colors, fill_screen, put_tile, init_isr, wait
 */
#include "ubox.h"

#define LOCAL
#include "tiles.h"

#define WHITESPACE_TILE 129

void put_text(uint8_t x, uint8_t y, const uint8_t *text)
{
    while (*text)
        ubox_put_tile(x++, y, *text++ + 128 - 31);
}

void main()
{
    ubox_init_isr(2);

    ubox_set_mode(2);

    ubox_set_colors(1, 1, 1);

    ubox_disable_screen();

    ubox_set_tiles(tiles);
    ubox_set_tiles_colors(tiles_colors);

    ubox_fill_screen(WHITESPACE_TILE);
    put_text(11, 11, "HELLO WORLD!!");

    ubox_enable_screen();

    while (1)
    {
        ubox_wait();
    }
}
