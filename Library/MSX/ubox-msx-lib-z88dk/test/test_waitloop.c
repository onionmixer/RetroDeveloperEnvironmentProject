/* Test: set_tiles (no colors) + infinite wait loop */
#include "ubox.h"

static const uint8_t patterns[2048] = {0};

void main(void)
{
    ubox_set_mode(2);
    ubox_set_colors(15, 1, 1);
    ubox_disable_screen();
    ubox_set_tiles(patterns);
    ubox_fill_screen(0);
    ubox_init_isr(2);
    ubox_enable_screen();
    /* infinite wait — should NEVER return to DOS */
    for (;;) {
        ubox_wait();
    }
}
