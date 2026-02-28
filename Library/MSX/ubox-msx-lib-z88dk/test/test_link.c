#include <stdint.h>
#include "../include/ubox.h"

static const uint8_t dummy_tiles[8] = {0};
static const uint8_t dummy_colors[8] = {0};
static const uint8_t dummy_sprite[8] = {0};
static struct sprite_attr dummy_attr = {0, 0, 0, 0};
static uint8_t buf[32];

void my_isr(void) {}

void main(void) {
    // 0-param functions
    ubox_enable_screen();
    ubox_disable_screen();
    ubox_get_vsync_freq();
    ubox_reset_tick();
    ubox_wait();
    ubox_select_ctl();

    // pointer fastcall
    ubox_set_tiles(dummy_tiles);
    ubox_set_tiles_colors(dummy_colors);
    ubox_set_user_isr(my_isr);

    // uint8_t fastcall
    ubox_set_mode(2);
    ubox_read_keys(0);
    ubox_wait_for(1);
    ubox_fill_screen(0);
    ubox_read_ctl(0);

    // ISR
    ubox_init_isr(2);

    // 2-param callee
    ubox_wvdp(1, 0xe2);
    ubox_get_tile(0, 0);
    ubox_write_vm((uint8_t *)0x1800, 32, buf);
    ubox_set_sprite_pat8(dummy_sprite, 0);
    ubox_set_sprite_pat16(dummy_sprite, 0);
    ubox_set_sprite_attr(&dummy_attr, 0);
    ubox_set_sprite_pat8_flip(dummy_sprite, 0);
    ubox_set_sprite_pat16_flip(dummy_sprite, 0);

    // 3-param callee
    ubox_set_colors(1, 1, 1);
    ubox_put_tile(0, 0, 1);
    ubox_read_vm(buf, 32, (const uint8_t *)0x1800);
}
