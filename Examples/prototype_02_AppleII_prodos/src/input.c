#include "input.h"
#include <conio.h>

unsigned char input_read_blocking(void)
{
    char key = cgetc();

    switch (key) {
        case 'w': case 'W': case 'i': case 'I': return INPUT_UP;
        case 's': case 'S': case 'k': case 'K': return INPUT_DOWN;
        case 'a': case 'A': case 'j': case 'J': return INPUT_LEFT;
        case 'd': case 'D': case 'l': case 'L': return INPUT_RIGHT;
        case 'h': case 'H': return INPUT_HELP;
        case 'q': case 'Q': return INPUT_QUIT;
        case '1': return INPUT_YES;
        case '0': return INPUT_NO;
        default: return INPUT_NONE;
    }
}

unsigned char input_wait_yes_no(void)
{
    for (;;) {
        char c = cgetc();
        if (c == '1') return 1;
        if (c == '0') return 0;
    }
}

void input_wait_any_key(void)
{
    (void)cgetc();
}
