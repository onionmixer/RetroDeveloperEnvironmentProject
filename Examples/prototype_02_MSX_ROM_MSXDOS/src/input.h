#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#define INPUT_UP     0x01
#define INPUT_DOWN   0x02
#define INPUT_LEFT   0x04
#define INPUT_RIGHT  0x08
#define INPUT_FIRE1  0x10
#define INPUT_HELP   0x20
#define INPUT_QUIT   0x40
#define INPUT_YES    0x80

uint8_t input_read(void);
void input_reset_timer(void);

#endif
