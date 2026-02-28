#include "ubox.h"

void main(void) {
    ubox_set_mode(2);
    ubox_set_colors(15, 1, 1);
    ubox_disable_screen();
    ubox_fill_screen(0);
    ubox_init_isr(2);
    ubox_enable_screen();
    ubox_wait_for(90);
    __asm
    di
    ld a, $C9
    ld ($FD9F), a
    ei
    call $006C
    __endasm;
}
