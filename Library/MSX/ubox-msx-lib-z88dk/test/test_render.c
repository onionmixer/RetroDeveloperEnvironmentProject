#include "ubox.h"

/* Simple 8x8 checkerboard pattern for tile 1 */
static const uint8_t patterns[2048] = {
    /* tile 0: empty */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* tile 1: checkerboard */
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    /* tile 2: solid */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    /* rest: zeros */
};

static const uint8_t colors[2048] = {
    /* tile 0: black on blue */
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    /* tile 1: white on black */
    0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1, 0xF1,
    /* tile 2: green on black */
    0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
    /* rest: zeros */
};

void main(void)
{
    uint8_t x, y;

    ubox_set_mode(2);
    ubox_set_colors(15, 1, 1);
    ubox_disable_screen();
    ubox_set_tiles(patterns);
    ubox_set_tiles_colors(colors);
    ubox_fill_screen(0);

    /* Draw a border of tile 2 and fill center with tile 1 */
    for (x = 0; x < 32; x++) {
        ubox_put_tile(x, 0, 2);
        ubox_put_tile(x, 23, 2);
    }
    for (y = 1; y < 23; y++) {
        ubox_put_tile(0, y, 2);
        ubox_put_tile(31, y, 2);
        for (x = 1; x < 31; x++) {
            ubox_put_tile(x, y, 1);
        }
    }

    ubox_enable_screen();
    ubox_init_isr(2);

    /* Stay here forever */
    for (;;) {
        ubox_wait();
    }
}
