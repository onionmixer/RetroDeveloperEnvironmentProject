#include "help.h"
#include "render.h"
#include "input.h"
#include <ubox.h>

void help_show(const GameState *gs)
{
    ubox_disable_screen();
    ubox_fill_screen(0);

    render_print(2, 1, "DUNGEON EXPLORER");
    render_print(2, 3, "WASD : Move");
    render_print(2, 5, "Walk into doors or");
    render_print(2, 6, "stairs to interact");
    render_print(2, 8, "1=Yes 0=No for");
    render_print(2, 9, "confirmations");
    render_print(2, 11, "H : This help");
    render_print(2, 12, "Q : Quit game");
    render_print(2, 14, "[Press any key]");

    ubox_enable_screen();

    input_reset_timer();
    while (input_read() == 0)
        ubox_wait();

    input_reset_timer();
    render_redraw_all(gs);
}
