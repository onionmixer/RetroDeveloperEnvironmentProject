#include "engine.h"
#include "render.h"
#include "logic.h"
#include "monster.h"
#include "input.h"
#include "help.h"
#include "room_data.h"

void main(void)
{
    GameState st;
    unsigned char move_counter = 0;

    monster_init_all();
    logic_init(&st);
    render_load_tileset(g_rooms[st.room].tileset_id);

    render_init();
    render_set_status("Ready.");
    logic_update_camera(&st);
    render_redraw_all(&st);

    while (st.running) {
        unsigned char map_dirty = 0;
        unsigned char hud_dirty = 0;
        unsigned char full_dirty = 0;
        unsigned char tx = 0, ty = 0;
        MoveResult mr;
        unsigned char inp;

        inp = input_read_blocking();

        if (inp == INPUT_QUIT) {
            if (render_prompt_yes_no("Quit? 1=yes 0=no"))
                st.running = 0;
            else {
                render_set_status("Continue.");
                hud_dirty = 1;
            }
        }
        else if (inp == INPUT_HELP) {
            if (help_show(&st))
                st.running = 0;
            else {
                render_set_status("Ready.");
                full_dirty = 1;
            }
        }
        else if (inp >= INPUT_UP && inp <= INPUT_RIGHT) {
            signed char dx = 0, dy = 0;
            if (inp == INPUT_UP)    dy = -1;
            if (inp == INPUT_DOWN)  dy = 1;
            if (inp == INPUT_LEFT)  dx = -1;
            if (inp == INPUT_RIGHT) dx = 1;

            mr = logic_try_move(&st, dx, dy, &tx, &ty);

            if (mr == MOVE_OK) {
                unsigned char collided = 0;
                int hit;
                char msg[MSG_BUF];

                st.x = tx;
                st.y = ty;
                render_set_status("Moved.");
                map_dirty = 1;

                hit = monster_check_collision(&st);
                if (hit >= 0) {
                    monster_format_encounter_msg(msg, sizeof(msg),
                                                 st.room, (unsigned char)hit);
                    render_set_status(msg);
                    collided = 1;
                }

                move_counter++;
                if (move_counter >= 2) {
                    move_counter = 0;
                    monster_update_all(st.room, st.x, st.y);

                    if (!collided) {
                        hit = monster_check_collision(&st);
                        if (hit >= 0) {
                            monster_format_encounter_msg(msg, sizeof(msg),
                                                         st.room, (unsigned char)hit);
                            render_set_status(msg);
                        }
                    }
                }
            } else if (mr == MOVE_BLOCKED) {
                render_set_status("Blocked.");
                hud_dirty = 1;
            } else if (mr == MOVE_DOOR) {
                const DoorDef *door = logic_find_door(st.room, tx, ty);
                if (door && render_prompt_yes_no("Enter room? 1=yes 0=no")) {
                    if (logic_do_door(&st, door)) {
                        render_load_tileset(g_rooms[st.room].tileset_id);
                        render_set_status("Room changed.");
                        move_counter = 0;
                    } else {
                        render_set_status("No connected room.");
                    }
                    full_dirty = 1;
                } else {
                    render_set_status("Canceled.");
                    hud_dirty = 1;
                }
            } else if (mr == MOVE_STAIR) {
                const StairDef *stair = logic_find_stair(st.room, tx, ty);
                if (stair && render_prompt_yes_no("Use stairs? 1=yes 0=no")) {
                    if (logic_do_stair(&st, stair)) {
                        render_load_tileset(g_rooms[st.room].tileset_id);
                        render_set_status("Floor changed.");
                        move_counter = 0;
                    } else {
                        render_set_status("No connected stair.");
                    }
                    full_dirty = 1;
                } else {
                    render_set_status("Canceled.");
                    hud_dirty = 1;
                }
            } else if (mr == MOVE_BOX) {
                const BoxDef *box = logic_find_box(st.room, tx, ty);
                if (box && render_prompt_yes_no("Open box? 1=yes 0=no")) {
                    char items[MSG_BUF];
                    unsigned char p = 0, bi;

                    render_wait_any_key(box->effect);

                    items[p++] = 'I'; items[p++] = 't'; items[p++] = 'e';
                    items[p++] = 'm'; items[p++] = 's'; items[p++] = ':';
                    items[p++] = ' ';
                    for (bi = 0; bi < box->item_count; bi++) {
                        const char *name = box->items[bi];
                        unsigned char j;
                        for (j = 0; name[j] && p < (MSG_BUF - 2); j++)
                            items[p++] = name[j];
                        if (bi + 1 < box->item_count && p < (MSG_BUF - 2)) {
                            items[p++] = ',';
                            items[p++] = ' ';
                        }
                    }
                    items[p] = '\0';
                    render_wait_any_key(items);

                    render_set_status("Box closed.");
                    full_dirty = 1;
                } else {
                    render_set_status("Canceled.");
                    hud_dirty = 1;
                }
            }
            hud_dirty = 1;
        }

        if (!st.running) break;

        logic_update_camera(&st);

        if (full_dirty) {
            render_redraw_all(&st);
            continue;
        }
        if (map_dirty) {
            render_draw_map(&st);
            render_draw_monsters(&st);
            render_update_player(&st);
        }
        if (hud_dirty || map_dirty) {
            render_update_status(&st);
        }
    }

    render_cleanup();
}
