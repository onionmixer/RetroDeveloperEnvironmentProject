#ifndef INPUT_H
#define INPUT_H

#define INPUT_NONE   0
#define INPUT_UP     1
#define INPUT_DOWN   2
#define INPUT_LEFT   3
#define INPUT_RIGHT  4
#define INPUT_HELP   5
#define INPUT_QUIT   6
#define INPUT_YES    7
#define INPUT_NO     8

unsigned char input_read_blocking(void);
unsigned char input_wait_yes_no(void);
void input_wait_any_key(void);

#endif
