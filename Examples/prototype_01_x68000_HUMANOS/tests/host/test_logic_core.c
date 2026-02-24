#include <assert.h>
#include <stdio.h>

#include "../../src/engine.h"
#include "../../src/logic.h"

static int find_adjacent_floor(unsigned char room,
                               unsigned char tx,
                               unsigned char ty,
                               unsigned char *px,
                               unsigned char *py,
                               signed char *dx,
                               signed char *dy)
{
    static const signed char ox[4] = {0, 0, -1, 1};
    static const signed char oy[4] = {-1, 1, 0, 0};
    int i;

    for (i = 0; i < 4; i++) {
        int nx = (int)tx + ox[i];
        int ny = (int)ty + oy[i];
        if (nx < 0 || nx >= ROOM_W || ny < 0 || ny >= ROOM_H) continue;
        if (logic_get_tile(room, (unsigned char)nx, (unsigned char)ny) == '.') {
            *px = (unsigned char)nx;
            *py = (unsigned char)ny;
            *dx = (signed char)(tx - *px);
            *dy = (signed char)(ty - *py);
            return 1;
        }
    }
    return 0;
}

static void test_basic_move_bounds(void)
{
    GameState g;
    logic_init(&g);

    g.room = 0;
    g.x = 0;
    g.y = 0;
    assert(logic_try_move(&g, -1, 0) == MOVE_BLOCKED);
    assert(logic_try_move(&g, 0, -1) == MOVE_BLOCKED);
}

static void test_box_pending(void)
{
    GameState g;
    unsigned char px, py;
    signed char dx, dy;
    unsigned char i;

    logic_init(&g);
    g.room = 0;

    for (i = 0; i < g_box_count[g.room]; i++) {
        const BoxX68 *b = &g_boxes[g.room][i];
        if (find_adjacent_floor(g.room, b->x, b->y, &px, &py, &dx, &dy)) {
            g.x = px;
            g.y = py;
            assert(logic_find_box_at(g.room, b->x, b->y) >= 0);
            assert(logic_try_move(&g, dx, dy) == MOVE_BLOCKED_BOX);
            return;
        }
    }

    assert(!"no testable box found");
}

static void test_stair_pending_and_transition(void)
{
    GameState g;
    unsigned char px, py;
    signed char dx, dy;
    unsigned char i;

    logic_init(&g);
    g.room = 0;

    for (i = 0; i < g_stair_count[g.room]; i++) {
        const StairX68 *s = &g_stairs[g.room][i];
        if (find_adjacent_floor(g.room, s->x, s->y, &px, &py, &dx, &dy)) {
            int idx;
            g.x = px;
            g.y = py;
            idx = logic_find_stair_at(g.room, s->x, s->y);
            assert(idx >= 0);
            assert(logic_try_move(&g, dx, dy) == MOVE_STAIR_PENDING);
            assert(logic_stair_transition(&g, idx) == 1);
            assert(g.room < ROOM_COUNT);
            return;
        }
    }

    assert(!"no testable stair found");
}

static void test_door_pending_and_transition(void)
{
    GameState g;
    unsigned char px, py;
    signed char dx, dy;
    unsigned char i;

    logic_init(&g);
    g.room = 0;

    for (i = 0; i < g_door_count[g.room]; i++) {
        const DoorX68 *d = &g_doors[g.room][i];
        if (find_adjacent_floor(g.room, d->x, d->y, &px, &py, &dx, &dy)) {
            int idx;
            g.x = px;
            g.y = py;
            idx = logic_find_door_at(g.room, d->x, d->y);
            assert(idx >= 0);
            assert(logic_try_move(&g, dx, dy) == MOVE_DOOR_PENDING);
            assert(logic_door_transition(&g, idx) == 1);
            assert(g.room < ROOM_COUNT);
            return;
        }
    }

    assert(!"no testable door found");
}

static void test_monster_lookup(void)
{
    unsigned char room;
    for (room = 0; room < ROOM_COUNT; room++) {
        unsigned char i;
        for (i = 0; i < g_monster_count[room]; i++) {
            const MonsterX68 *m = &g_monsters[room][i];
            int idx = logic_find_monster_at(room, m->x, m->y);
            assert(idx >= 0);
            assert(g_monsters[room][idx].name_id > 0);
        }
    }
}

static void test_monster_update_bounds(void)
{
    GameState g;
    unsigned char room;

    logic_init(&g);
    for (room = 0; room < ROOM_COUNT; room++) {
        int step;
        g.room = room;
        for (step = 0; step < 8; step++) {
            unsigned char i;
            logic_update_monsters(&g);
            for (i = 0; i < g_monster_count[room]; i++) {
                int found = 0;
                unsigned char y, x;
                for (y = 0; y < ROOM_H && !found; y++) {
                    for (x = 0; x < ROOM_W; x++) {
                        int idx = logic_find_monster_at(room, x, y);
                        if (idx == i) {
                            assert(logic_get_tile(room, x, y) == '.');
                            found = 1;
                            break;
                        }
                    }
                }
                assert(found);
            }
        }
    }
}

static void collect_monster_positions(unsigned char room,
                                      unsigned char out_x[MAX_MONSTERS],
                                      unsigned char out_y[MAX_MONSTERS])
{
    unsigned char y, x;
    unsigned char i;

    for (i = 0; i < MAX_MONSTERS; i++) {
        out_x[i] = 255;
        out_y[i] = 255;
    }

    for (y = 0; y < ROOM_H; y++) {
        for (x = 0; x < ROOM_W; x++) {
            int idx = logic_find_monster_at(room, x, y);
            if (idx >= 0 && idx < MAX_MONSTERS) {
                out_x[idx] = x;
                out_y[idx] = y;
            }
        }
    }
}

static void test_monster_update_deterministic(void)
{
    GameState g1, g2;
    unsigned char room = 0;
    unsigned char ax[MAX_MONSTERS], ay[MAX_MONSTERS];
    unsigned char bx[MAX_MONSTERS], by[MAX_MONSTERS];
    unsigned char i;

    logic_init(&g1);
    g1.room = room;
    logic_update_monsters(&g1);
    collect_monster_positions(room, ax, ay);

    logic_init(&g2);
    g2.room = room;
    logic_update_monsters(&g2);
    collect_monster_positions(room, bx, by);

    for (i = 0; i < g_monster_count[room]; i++) {
        assert(ax[i] == bx[i]);
        assert(ay[i] == by[i]);
    }
}

static int find_adjacent_floor_to_monster(unsigned char room,
                                          int monster_idx,
                                          unsigned char *px,
                                          unsigned char *py)
{
    static const signed char ox[4] = {1, -1, 0, 0};
    static const signed char oy[4] = {0, 0, 1, -1};
    unsigned char mx, my;
    int k;

    logic_get_monster_pos(room, monster_idx, &mx, &my);
    for (k = 0; k < 4; k++) {
        int nx = (int)mx + ox[k];
        int ny = (int)my + oy[k];
        if (nx < 0 || nx >= ROOM_W || ny < 0 || ny >= ROOM_H) continue;
        if (logic_get_tile(room, (unsigned char)nx, (unsigned char)ny) == '.') {
            *px = (unsigned char)nx;
            *py = (unsigned char)ny;
            return 1;
        }
    }
    return 0;
}

static void test_monster_state_detection_and_return(void)
{
    GameState g;
    unsigned char room = 0;
    unsigned char px = 0, py = 0;

    logic_init(&g);
    g.room = room;
    if (g_monster_count[room] == 0) return;

    assert(logic_get_monster_state(room, 0) == MONSTER_PATROL);
    assert(logic_get_monster_detect_enabled(room, 0) == 1);

    if (find_adjacent_floor_to_monster(room, 0, &px, &py)) {
        g.x = px;
        g.y = py;
        logic_update_monsters(&g);
        assert(logic_get_monster_state(room, 0) == MONSTER_CHASE);
    }

    logic_monster_force_return(room, 0);
    assert(logic_get_monster_state(room, 0) == MONSTER_RETURNING);
    assert(logic_get_monster_detect_enabled(room, 0) == 0);

    {
        int i;
        for (i = 0; i < 256; i++) {
            logic_update_monsters(&g);
            if (logic_get_monster_state(room, 0) == MONSTER_PATROL) break;
        }
    }
    assert(logic_get_monster_state(room, 0) == MONSTER_PATROL);
    assert(logic_get_monster_detect_enabled(room, 0) == 1);
}

int main(void)
{
    test_basic_move_bounds();
    test_box_pending();
    test_stair_pending_and_transition();
    test_door_pending_and_transition();
    test_monster_lookup();
    test_monster_update_bounds();
    test_monster_update_deterministic();
    test_monster_state_detection_and_return();
    puts("test_logic_core: ok");
    return 0;
}
