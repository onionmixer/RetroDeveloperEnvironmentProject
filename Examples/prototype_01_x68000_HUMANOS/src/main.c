#include "engine.h"
#include "logic.h"
#include "render.h"

#include <stdio.h>
#include <string.h>

static int find_safe_spawn(unsigned char room, unsigned char *out_x, unsigned char *out_y)
{
    int y, x;
    unsigned char sx = g_player_start_x[room];
    unsigned char sy = g_player_start_y[room];

    if (logic_get_tile(room, sx, sy) == '.' && logic_find_monster_at(room, sx, sy) < 0) {
        *out_x = sx;
        *out_y = sy;
        return 1;
    }

    for (y = 0; y < ROOM_H; y++) {
        for (x = 0; x < ROOM_W; x++) {
            unsigned char ux = (unsigned char)x;
            unsigned char uy = (unsigned char)y;
            if (logic_get_tile(room, ux, uy) == '.' &&
                logic_find_monster_at(room, ux, uy) < 0) {
                *out_x = ux;
                *out_y = uy;
                return 1;
            }
        }
    }
    return 0;
}

static void set_box_items_message(char *out, unsigned int out_sz, const BoxX68 *box)
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

    logic_init(&g);
    render_set_status(&g, "Prototype 01 X68000 (20x20)");

    while (g.running) {
        int key;
        signed char dx = 0;
        signed char dy = 0;
        unsigned char prev_room = g.room;
        unsigned char prev_x = g.x;
        unsigned char prev_y = g.y;
        unsigned char did_player_move = 0;

        logic_update_camera(&g);
        render_draw(&g);

        key = render_read_key();

        if (key == 'q') {
            if (render_prompt_yes_no(&g, "Quit?")) {
                g.running = 0;
            } else {
                render_set_status(&g, "Continue.");
            }
            continue;
        }
        if (key == 'h') {
            if (render_show_help()) {
                g.running = 0;
            } else {
                render_set_status(&g, "Back from help.");
            }
            continue;
        }
        if (key == 'm') {
            g.monster_move_enabled = (unsigned char)!g.monster_move_enabled;
            if (g.monster_move_enabled) render_set_status(&g, "Monster move: ON");
            else render_set_status(&g, "Monster move: OFF");
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
            int tx = (int)g.x + dx;
            int ty = (int)g.y + dy;

            if (mv == MOVE_OK) {
                did_player_move = 1;
                int midx = logic_find_monster_at(g.room, g.x, g.y);
                if (midx >= 0) {
                    const MonsterX68 *m = &g_monsters[g.room][midx];
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Encountered %s! Step back.",
                             g_monster_names[m->name_id]);
                    logic_monster_force_return(g.room, midx);
                    render_set_status(&g, msg);
                    g.room = prev_room;
                    g.x = prev_x;
                    g.y = prev_y;
                } else {
                    render_set_status(&g, "Moved.");
                }
            } else if (mv == MOVE_BLOCKED) {
                render_set_status(&g, "Blocked.");
            } else if (mv == MOVE_DOOR_PENDING) {
                if (tx >= 0 && tx < ROOM_W && ty >= 0 && ty < ROOM_H) {
                    int idx = logic_find_door_at(g.room, (unsigned char)tx, (unsigned char)ty);
                    if (idx >= 0) {
                        const DoorX68 *d = &g_doors[g.room][idx];
                        char msg[128];
                        if (d->target_room < ROOM_COUNT)
                            snprintf(msg, sizeof(msg), "Enter %s?", g_room_names[d->target_room]);
                        else
                            snprintf(msg, sizeof(msg), "Enter door?");
                        if (render_prompt_yes_no(&g, msg)) {
                            if (logic_door_transition(&g, idx)) {
                                int midx = logic_find_monster_at(g.room, g.x, g.y);
                                if (midx >= 0) {
                                    unsigned char sx, sy;
                                    logic_monster_force_return(g.room, midx);
                                    if (find_safe_spawn(g.room, &sx, &sy)) {
                                        g.x = sx;
                                        g.y = sy;
                                        render_set_status(&g, "Door transition ok. Monster nearby, respawned.");
                                    } else {
                                        render_set_status(&g, "Door transition ok. No safe spawn.");
                                    }
                                } else {
                                    render_set_status(&g, "Door transition ok.");
                                }
                            } else render_set_status(&g, "Door transition failed.");
                        } else {
                            render_set_status(&g, "Door cancelled.");
                        }
                    }
                }
            } else if (mv == MOVE_STAIR_PENDING) {
                if (tx >= 0 && tx < ROOM_W && ty >= 0 && ty < ROOM_H) {
                    int idx = logic_find_stair_at(g.room, (unsigned char)tx, (unsigned char)ty);
                    if (idx >= 0) {
                        const StairX68 *s = &g_stairs[g.room][idx];
                        const char *q = s->type == 0 ? "Descend stair?" : "Ascend stair?";
                        if (render_prompt_yes_no(&g, q)) {
                            if (logic_stair_transition(&g, idx)) {
                                int midx = logic_find_monster_at(g.room, g.x, g.y);
                                if (midx >= 0) {
                                    unsigned char sx, sy;
                                    logic_monster_force_return(g.room, midx);
                                    if (find_safe_spawn(g.room, &sx, &sy)) {
                                        g.x = sx;
                                        g.y = sy;
                                        render_set_status(&g, "Stair transition ok. Monster nearby, respawned.");
                                    } else {
                                        render_set_status(&g, "Stair transition ok. No safe spawn.");
                                    }
                                } else {
                                    render_set_status(&g, "Stair transition ok.");
                                }
                            } else render_set_status(&g, "Stair transition failed.");
                        } else {
                            render_set_status(&g, "Stair cancelled.");
                        }
                    }
                }
            } else if (mv == MOVE_BLOCKED_BOX) {
                if (tx >= 0 && tx < ROOM_W && ty >= 0 && ty < ROOM_H) {
                    int idx = logic_find_box_at(g.room, (unsigned char)tx, (unsigned char)ty);
                    if (idx >= 0) {
                        const BoxX68 *b = &g_boxes[g.room][idx];
                        char msg[128];
                        if (b->placed_by_id > 0) snprintf(msg, sizeof(msg), "Open box by %s?", g_placed_by[b->placed_by_id]);
                        else snprintf(msg, sizeof(msg), "Open box?");
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

        if (did_player_move && g.monster_move_enabled) {
            int midx_after;
            g.monster_tick++;
            if ((g.monster_tick & 1u) == 0u) {
                logic_update_monsters(&g);
            }
            midx_after = logic_find_monster_at(g.room, g.x, g.y);
            if (midx_after >= 0) {
                const MonsterX68 *m = &g_monsters[g.room][midx_after];
                char msg[128];
                snprintf(msg, sizeof(msg), "%s moved into you! Back step.",
                         g_monster_names[m->name_id]);
                logic_monster_force_return(g.room, midx_after);
                render_set_status(&g, msg);
                g.room = prev_room;
                g.x = prev_x;
                g.y = prev_y;
            }
        }
    }

    printf("\nBye.\n");
    return 0;
}
