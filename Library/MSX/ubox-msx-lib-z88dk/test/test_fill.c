#include "ubox.h"

/* tile 1 = checkerboard */
static const uint8_t patterns[2048] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,
};
static const uint8_t colors[2048] = {
    0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,0xF1,
};

void main(void)
{
    ubox_set_mode(2);
    ubox_set_colors(15, 1, 1);
    ubox_disable_screen();
    ubox_set_tiles(patterns);
    ubox_set_tiles_colors(colors);
    ubox_fill_screen(1);         /* fill with checkerboard tile */
    ubox_enable_screen();
    ubox_init_isr(2);
    for (;;) { ubox_wait(); }   /* stay forever */
}
