#include "engine.h"
#include "logic.h"
#include "render.h"
#include "input.h"
#include "help.h"
#include "monster.h"
#include <ubox.h>
#include <stdio.h>

static GameState g;
static signed char last_dx, last_dy;
static uint8_t move_counter;

static uint8_t wait_yes_no(void)
{
    uint8_t row;
    input_reset_timer();
    for (;;) {
        ubox_wait();
        row = ubox_read_keys(0);
        if (row & UBOX_MSX_KEY_1) return 1;
        if (row & UBOX_MSX_KEY_0) return 0;
    }
}

static void do_door(void)
{
    int di;
    unsigned char nx, ny;

    render_set_status("Enter door? 1=Yes 0=No");
    render_update_status(&g);

    if (!wait_yes_no()) {
        render_set_status("Door cancelled.");
        input_reset_timer();
        return;
    }

    nx = (unsigned char)((int)g.x + last_dx);
    ny = (unsigned char)((int)g.y + last_dy);
    di = logic_find_door_at(g.room, nx, ny);
    if (di >= 0) {
        g.x = nx;
        g.y = ny;
        if (logic_door_transition(&g, di)) {
            logic_update_camera(&g);
            monster_init_room(g.room);
            move_counter = 0;
            render_set_status("Continue.");
            render_load_tileset(g_room_tileset_id[g.room]);
            render_redraw_all(&g);
            input_reset_timer();
            return;
        }
    }
    render_set_status("Door locked.");
    input_reset_timer();
}

static void do_stair(void)
{
    int si;
    unsigned char nx, ny;

    render_set_status("Use stairs? 1=Yes 0=No");
    render_update_status(&g);

    if (!wait_yes_no()) {
        render_set_status("Cancelled.");
        input_reset_timer();
        return;
    }

    nx = (unsigned char)((int)g.x + last_dx);
    ny = (unsigned char)((int)g.y + last_dy);
    si = logic_find_stair_at(g.room, nx, ny);
    if (si >= 0) {
        g.x = nx;
        g.y = ny;
        if (logic_stair_transition(&g, si)) {
            logic_update_camera(&g);
            monster_init_room(g.room);
            move_counter = 0;
            render_set_status("Continue.");
            render_load_tileset(g_room_tileset_id[g.room]);
            render_redraw_all(&g);
            input_reset_timer();
            return;
        }
    }
    render_set_status("Stairs blocked.");
    input_reset_timer();
}

int main(void)
{
    uint8_t inp;
    MoveResult mr;

    logic_init(&g);
    logic_update_camera(&g);
    monster_init_room(g.room);
    move_counter = 0;
    render_init();
    render_load_tileset(g_room_tileset_id[g.room]);
    render_redraw_all(&g);

    while (g.running) {
        ubox_wait();
        inp = input_read();
        if (inp == 0)
            continue;

        if (inp & INPUT_QUIT) {
            g.running = 0;
            break;
        }

        if (inp & INPUT_HELP) {
            help_show(&g);
            continue;
        }

        last_dx = 0;
        last_dy = 0;
        if (inp & INPUT_UP)         { last_dy = -1; }
        else if (inp & INPUT_DOWN)  { last_dy = 1; }
        else if (inp & INPUT_LEFT)  { last_dx = -1; }
        else if (inp & INPUT_RIGHT) { last_dx = 1; }
        else continue;

        mr = logic_try_move(&g, last_dx, last_dy);

        switch (mr) {
            case MOVE_OK:
            {
                unsigned char hit;
                char msg[40];

                hit = monster_check_collision(g.room, g.x, g.y);
                if (hit) {
                    sprintf(msg, "Encountered %s!",
                            g_monster_names[hit - 1]);
                    render_set_status(msg);
                } else {
                    render_set_status("Moved.");
                }

                move_counter++;
                if (move_counter >= 2) {
                    move_counter = 0;
                    monster_update_all(g.room, g.x, g.y);
                    if (!hit) {
                        hit = monster_check_collision(g.room,
                                                     g.x, g.y);
                        if (hit) {
                            sprintf(msg, "Encountered %s!",
                                    g_monster_names[hit - 1]);
                            render_set_status(msg);
                        }
                    }
                }
                break;
            }
            case MOVE_BLOCKED:
                render_set_status("Blocked.");
                render_update_status(&g);
                continue;
            case MOVE_BLOCKED_BOX:
                render_set_status("A box blocks the way.");
                render_update_status(&g);
                continue;
            case MOVE_DOOR_PENDING:
                do_door();
                continue;
            case MOVE_STAIR_PENDING:
                do_stair();
                continue;
        }

        logic_update_camera(&g);
        render_draw_map(&g);
        render_draw_monsters(&g);
        render_update_player(&g);
        render_update_status(&g);
    }

    render_cleanup();
    return 0;
}
