#include "help.h"

#include <conio.h>
#include <stdio.h>
#include <string.h>

#define WRAP_WIDTH 40

static const char * const k_help_text[] = {
    "Prototype 01 MSX - Help",
    "",
    "Movement",
    "W: up, S: down, A: left, D: right",
    "Walls(#) are blocked.",
    "",
    "Interactions",
    "When you touch door/stair/box,",
    "a prompt appears.",
    "Press 1 to confirm, 0 to cancel.",
    "",
    "Map symbols",
    "!: player",
    ".: floor",
    "#: wall",
    "@: door",
    "<: down stair",
    ">: up stair",
    "%: box",
    "",
    "Door/Stair",
    "Move onto @,<,> then confirm",
    "to transition room/floor.",
    "",
    "Box",
    "Open box -> effect message ->",
    "item list message.",
    "",
    "Help screen controls",
    "W/S: scroll help",
    "SPACE: return to game",
    "Q: quit prompt"
};

static void add_line(HelpScreen *hs, const char *text, unsigned char len)
{
    unsigned char i;
    if (hs->line_count >= MAX_HELP_LINES) return;
    if (len > WRAP_WIDTH) len = WRAP_WIDTH;
    for (i = 0; i < len; i++) hs->lines[hs->line_count][i] = text[i];
    hs->lines[hs->line_count][len] = '\0';
    hs->line_count++;
}

static void wrap_and_add(HelpScreen *hs, const char *src)
{
    unsigned char len = (unsigned char)strlen(src);
    unsigned char pos = 0;

    if (len == 0) {
        add_line(hs, "", 0);
        return;
    }

    while (pos < len) {
        unsigned char rem = (unsigned char)(len - pos);
        unsigned char cut;

        if (rem <= WRAP_WIDTH) {
            add_line(hs, src + pos, rem);
            return;
        }

        cut = WRAP_WIDTH;
        while (cut > 0 && src[pos + cut] != ' ') cut--;
        if (cut == 0) {
            add_line(hs, src + pos, WRAP_WIDTH);
            pos = (unsigned char)(pos + WRAP_WIDTH);
        } else {
            add_line(hs, src + pos, cut);
            pos = (unsigned char)(pos + cut + 1);
        }
    }
}

void help_init(HelpScreen *hs)
{
    unsigned int i;
    hs->line_count = 0;
    hs->scroll_offset = 0;

    for (i = 0; i < sizeof(k_help_text) / sizeof(k_help_text[0]); i++) {
        wrap_and_add(hs, k_help_text[i]);
        if (hs->line_count >= MAX_HELP_LINES) break;
    }
}

void help_scroll(HelpScreen *hs, signed char direction)
{
    int next = (int)hs->scroll_offset + direction;
    int max_offset = (int)hs->line_count - HELP_DISPLAY_ROWS;

    if (max_offset < 0) max_offset = 0;
    if (next < 0) next = 0;
    if (next > max_offset) next = max_offset;
    hs->scroll_offset = (unsigned char)next;
}

void help_render(const HelpScreen *hs)
{
    unsigned char row;
    clrscr();

    for (row = 0; row < HELP_DISPLAY_ROWS; row++) {
        unsigned int idx = (unsigned int)hs->scroll_offset + row;
        if (idx < hs->line_count) {
            printf("%-40s\r\n", hs->lines[idx]);
        } else {
            cputs("                                        \r\n");
        }
    }

    cputs("SPACE:return  W/S:scroll  Q:quit        \r\n");
}
