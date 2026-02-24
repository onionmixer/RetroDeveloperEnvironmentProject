#include "render.h"
#include "logic.h"

#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void print_padded_line(const char *text)
{
    char out[41];
    unsigned int i = 0;

    while (i < 40 && text[i] != '\0') {
        out[i] = text[i];
        i++;
    }
    while (i < 40) {
        out[i++] = ' ';
    }
    out[40] = '\0';
    printf("%s\r\n", out);
}

void render_set_status(GameState *g, const char *text)
{
    unsigned int i;
    for (i = 0; i < sizeof(g->status) - 1 && text[i] != '\0'; i++) {
        g->status[i] = text[i];
    }
    g->status[i] = '\0';
}

void render_draw(const GameState *g)
{
    unsigned char row;
    unsigned char col;
    char line[41];
    char info[40];

    clrscr();

    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            unsigned char wx = g->cam_x + col;
            unsigned char wy = g->cam_y + row;
            char ch = logic_get_tile(g->room, wx, wy);
            if (wx == g->x && wy == g->y) ch = '!';
            line[col] = ch;
        }
        line[VIEW_W] = '\0';
        print_padded_line(line);
    }

    sprintf(info, "R:%u X:%u Y:%u Z:%u", (unsigned)(g->room + 1), (unsigned)g->x,
            (unsigned)g->y, (unsigned)g_room_z[g->room]);
    print_padded_line(info);
    print_padded_line(g->status);
    print_padded_line("WASD(move) 1/0(confirm)");
    print_padded_line("H(HELP) Q(quit)");
}

int render_prompt_yes_no(GameState *g, const char *text)
{
    char key;
    char buf[80];

    sprintf(buf, "%s 1=yes 0=no", text);
    render_set_status(g, buf);
    render_draw(g);

    for (;;) {
        key = (char)tolower(cgetc());
        if (key == '1') return 1;
        if (key == '0') return 0;
    }
}

void render_wait_any_key(GameState *g, const char *text)
{
    render_set_status(g, text);
    render_draw(g);
    cgetc();
}
