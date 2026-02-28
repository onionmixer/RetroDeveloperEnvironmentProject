#include "help.h"
#include "render.h"
#include "input.h"
#include <conio.h>

#define HELP_LINES  45
#define HELP_PAGE   20

static const char *k_help_raw[] = {
    "F-SpagetyProto - Help Guide",
    "",
    "Movement Controls:",
    "  W  Move up",
    "  S  Move down",
    "  A  Move left",
    "  D  Move right",
    "",
    "Interaction:",
    "  When you approach a door, stair,",
    "  or box, a confirmation prompt",
    "  appears.",
    "  Press 1 to confirm the action, or",
    "  press 0 to cancel and stay.",
    "",
    "Map Tiles:",
    "  Floor    walkable area",
    "  Wall     impassable",
    "  Door     room transition",
    "  Stairs   level transition",
    "  Box      open for items",
    "  Monster  hostile creature",
    "",
    "Boxes contain items and may have",
    "special effects. Some boxes are",
    "placed by villains and may trigger",
    "negative effects when opened.",
    "Opening a box shows its effect",
    "first, then items inside.",
    "",
    "Monsters patrol rooms and will chase",
    "you if you get too close.",
    "Colliding with a monster causes it",
    "to retreat to its home position.",
    "",
    "Runtime keys:",
    "  W A S D (also I J K L): move",
    "  H: open this help",
    "  Q: quit prompt (1/0)",
    "",
    "The viewport shows a 10x10 section",
    "of the current 100x100 room grid.",
    "Coordinates and floor are displayed",
    "next to the map.",
    ""
};

static unsigned char help_scroll;

static void help_render(void)
{
    unsigned char i;
    unsigned char row = 0;

    render_clear_screen();

    for (i = help_scroll; i < HELP_LINES && row < HELP_PAGE; i++) {
        render_print(1, row, k_help_raw[i]);
        row++;
    }

    render_print(0, 22, "W/S scroll  SPACE return  Q quit");
}

unsigned char help_show(const GameState *gs)
{
    char c;
    (void)gs;
    help_scroll = 0;
    help_render();

    for (;;) {
        c = cgetc();
        if (c == 'w' || c == 'W') {
            if (help_scroll > 0) {
                help_scroll--;
                help_render();
            }
        } else if (c == 's' || c == 'S') {
            if (help_scroll + HELP_PAGE < HELP_LINES) {
                help_scroll++;
                help_render();
            }
        } else if (c == ' ') {
            return 0;
        } else if (c == 'q' || c == 'Q') {
            if (render_prompt_yes_no("Quit? 1=yes 0=no")) {
                return 1;
            }
            help_render();
        }
    }
}
