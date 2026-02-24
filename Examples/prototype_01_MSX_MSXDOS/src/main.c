#include "engine.h"
#include "help.h"
#include "logic.h"
#include "render.h"

#include <ctype.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

static void set_box_items_message(char *out, unsigned int out_sz, const BoxMsx *box)
{
    unsigned int pos = 0;
    unsigned char i;

    if (out_sz == 0) return;
    pos += (unsigned int)snprintf(out + pos, out_sz - pos, "Items: ");
    for (i = 0; i < box->item_count && i < MAX_ITEMS_PER_BOX; i++) {
        const char *name = g_item_names[box->item_ids[i]];
        if (i > 0 && pos < out_sz) pos += (unsigned int)snprintf(out + pos, out_sz - pos, ", ");
        if (pos < out_sz) pos += (unsigned int)snprintf(out + pos, out_sz - pos, "%s", name);
        if (pos >= out_sz - 1) break;
    }
    out[out_sz - 1] = '\0';
}

int main(void)
{
    GameState g;
    HelpScreen help;
    unsigned char in_help = 0;

    logic_init(&g);
    help_init(&help);
    render_set_status(&g, "Prototype 01 MSX (12x7)");

    while (g.running) {
        char key;
        signed char dx = 0;
        signed char dy = 0;

        if (in_help) {
            help_render(&help);
            key = (char)tolower(cgetc());

            if (key == ' ') {
                in_help = 0;
            } else if (key == 'w') {
                help_scroll(&help, -1);
            } else if (key == 's') {
                help_scroll(&help, 1);
            } else if (key == 'q') {
                if (render_prompt_yes_no(&g, "Quit?")) {
                    g.running = 0;
                }
            }
            continue;
        }

        logic_update_camera(&g);
        render_draw(&g);

        key = (char)tolower(cgetc());

        if (key == 'h') {
            in_help = 1;
            continue;
        }

        if (key == 'q') {
            if (render_prompt_yes_no(&g, "Quit?")) {
                g.running = 0;
            } else {
                render_set_status(&g, "Continue.");
            }
            continue;
        }

        if (key == 'w') dy = -1;
        else if (key == 's') dy = 1;
        else if (key == 'a') dx = -1;
        else if (key == 'd') dx = 1;
        else {
            render_set_status(&g, "Unknown key.");
            continue;
        }

        {
            MoveResult mv = logic_try_move(&g, dx, dy);
            unsigned char tx = (unsigned char)((int)g.x + dx);
            unsigned char ty = (unsigned char)((int)g.y + dy);

            if (mv == MOVE_OK) {
                render_set_status(&g, "Moved.");
            } else if (mv == MOVE_BLOCKED) {
                render_set_status(&g, "Blocked.");
            } else if (mv == MOVE_DOOR_PENDING) {
                int idx = logic_find_door_at(g.room, tx, ty);
                if (idx >= 0) {
                    const DoorMsx *d = &g_doors[g.room][idx];
                    char msg[80];
                    if (d->target_room < ROOM_COUNT)
                        sprintf(msg, "Enter %s?", g_room_names[d->target_room]);
                    else
                        sprintf(msg, "Enter door?");
                    if (render_prompt_yes_no(&g, msg)) {
                        if (logic_door_transition(&g, idx)) render_set_status(&g, "Door transition ok.");
                        else render_set_status(&g, "Door transition failed.");
                    } else {
                        render_set_status(&g, "Door cancelled.");
                    }
                }
            } else if (mv == MOVE_STAIR_PENDING) {
                int idx = logic_find_stair_at(g.room, tx, ty);
                if (idx >= 0) {
                    const StairMsx *s = &g_stairs[g.room][idx];
                    const char *q = s->type == 0 ? "Descend stair?" : "Ascend stair?";
                    if (render_prompt_yes_no(&g, q)) {
                        if (logic_stair_transition(&g, idx)) render_set_status(&g, "Stair transition ok.");
                        else render_set_status(&g, "Stair transition failed.");
                    } else {
                        render_set_status(&g, "Stair cancelled.");
                    }
                }
            } else if (mv == MOVE_BLOCKED_BOX) {
                int idx = logic_find_box_at(g.room, tx, ty);
                if (idx >= 0) {
                    const BoxMsx *b = &g_boxes[g.room][idx];
                    char msg[80];
                    if (b->placed_by_id > 0) sprintf(msg, "Open box by %s?", g_placed_by[b->placed_by_id]);
                    else sprintf(msg, "Open box?");
                    if (render_prompt_yes_no(&g, msg)) {
                        render_wait_any_key(&g, g_box_effects[b->effect_id]);
                        set_box_items_message(msg, sizeof(msg), b);
                        render_wait_any_key(&g, msg);
                        render_set_status(&g, "Box closed.");
                    } else {
                        render_set_status(&g, "Box cancelled.");
                    }
                }
            }
        }
    }

    clrscr();
    cputs("Bye.\r\n");
    return 0;
}
