/* test_screen2 + set_tiles only — isolate the crash */
#include "ubox.h"

static const uint8_t patterns[2048] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,
};

void main(void)
{
    ubox_set_mode(2);
    ubox_set_colors(15, 1, 1);
    ubox_disable_screen();
    ubox_set_tiles(patterns);   /* <-- THE ONLY ADDITION vs test_screen2 */
    ubox_fill_screen(0);
    ubox_init_isr(2);
    ubox_enable_screen();
    ubox_wait_for(180);         /* 3 seconds */
    __asm
    di
    ld a, $C9
    ld ($FD9F), a
    ei
    call $006C
    __endasm;
}
