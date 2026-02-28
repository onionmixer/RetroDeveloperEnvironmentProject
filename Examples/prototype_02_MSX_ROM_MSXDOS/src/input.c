#include "input.h"
#include <ubox.h>

#define KEY_REPEAT_DELAY 8
#define KEY_REPEAT_RATE  3

static uint8_t last_input;
static uint8_t key_timer;

uint8_t input_read(void)
{
    uint8_t result = 0;
    uint8_t row;

    /* Row 5: W(bit4) S(bit0) */
    row = ubox_read_keys(5);
    if (row & UBOX_MSX_KEY_W) result |= INPUT_UP;
    if (row & UBOX_MSX_KEY_S) result |= INPUT_DOWN;

    /* Row 2: A(bit6) */
    row = ubox_read_keys(2);
    if (row & UBOX_MSX_KEY_A) result |= INPUT_LEFT;

    /* Row 3: D(bit2) */
    row = ubox_read_keys(3);
    if (row & UBOX_MSX_KEY_D) result |= INPUT_RIGHT;

    /* Row 3: H(bit5) */
    if (row & UBOX_MSX_KEY_H) result |= INPUT_HELP;

    /* Row 4: Q(bit6) */
    row = ubox_read_keys(4);
    if (row & UBOX_MSX_KEY_Q) result |= INPUT_QUIT;

    /* Row 8: SPACE(bit0) */
    row = ubox_read_keys(8);
    if (row & UBOX_MSX_KEY_SPACE) result |= INPUT_FIRE1;

    /* Row 0: 1(bit1) for Yes */
    row = ubox_read_keys(0);
    if (row & UBOX_MSX_KEY_1) result |= INPUT_YES;

    /* Key repeat logic */
    if (result == 0) {
        key_timer = 0;
        last_input = 0;
        return 0;
    }

    if (result != last_input) {
        last_input = result;
        key_timer = 0;
        return result;
    }

    key_timer++;
    if (key_timer < KEY_REPEAT_DELAY)
        return 0;

    if (((key_timer - KEY_REPEAT_DELAY) % KEY_REPEAT_RATE) == 0)
        return result;

    return 0;
}

void input_reset_timer(void)
{
    key_timer = 0;
    last_input = 0;
}
