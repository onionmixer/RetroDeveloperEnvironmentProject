#include "monster.h"
#include "logic.h"

/* --- const data --- */

const unsigned char g_monster_count[ROOM_COUNT] = { 3, 2, 3 };

const MonsterDef g_monsters[ROOM_COUNT][MAX_MONSTERS] = {
    /* Room 0: Abandoned Hall (start 49,49) */
    { { 55,43, 0, 5 }, { 42,54, 1, 4 }, { 57,40, 3, 6 }, { 0,0,0,0 } },
    /* Room 1: Crystal Cavern (start 49,49) */
    { { 55,55, 5, 5 }, { 43,43, 6, 6 }, { 0,0,0,0 }, { 0,0,0,0 } },
    /* Room 2: Shadow Labyrinth (start 49,49) */
    { { 55,48, 2, 4 }, { 54,56, 4, 5 }, { 64,56, 7, 4 }, { 0,0,0,0 } },
};

const char * const g_monster_names[] = {
    "Cave Spider", "Dark Slime", "Shadow Rat", "Goblin Scout",
    "Skeleton", "Stone Golem", "Bat Swarm", "Fire Imp",
};
const unsigned char g_monster_name_count = 8;

/* --- runtime --- */

MonsterRT g_monsters_rt[MAX_MONSTERS];

static unsigned char abs_diff(unsigned char a, unsigned char b)
{
    return (a > b) ? (a - b) : (b - a);
}

static unsigned char manhattan(unsigned char x1, unsigned char y1,
                               unsigned char x2, unsigned char y2)
{
    return abs_diff(x1, x2) + abs_diff(y1, y2);
}

void monster_init_room(unsigned char room)
{
    unsigned char i;
    unsigned char cnt = g_monster_count[room];
    for (i = 0; i < MAX_MONSTERS; i++) {
        if (i < cnt) {
            g_monsters_rt[i].x = g_monsters[room][i].home_x;
            g_monsters_rt[i].y = g_monsters[room][i].home_y;
            g_monsters_rt[i].state = MON_IDLE;
            g_monsters_rt[i].detect_enabled = 1;
        } else {
            g_monsters_rt[i].x = 0;
            g_monsters_rt[i].y = 0;
            g_monsters_rt[i].state = MON_IDLE;
            g_monsters_rt[i].detect_enabled = 0;
        }
    }
}

static unsigned char monster_step_toward(MonsterRT *m, unsigned char room,
                                         unsigned char tx, unsigned char ty,
                                         unsigned char self_idx)
{
    signed char dx, dy;
    unsigned char adx, ady;
    unsigned char nx, ny;
    unsigned char cnt, j;

    dx = (signed char)tx - (signed char)m->x;
    dy = (signed char)ty - (signed char)m->y;
    if (dx == 0 && dy == 0) return 0;

    adx = abs_diff(m->x, tx);
    ady = abs_diff(m->y, ty);

    /* Primary axis: larger delta */
    if (adx >= ady) {
        nx = (dx > 0) ? m->x + 1 : m->x - 1;
        ny = m->y;
    } else {
        nx = m->x;
        ny = (dy > 0) ? m->y + 1 : m->y - 1;
    }

    if (nx < ROOM_W && ny < ROOM_H &&
        logic_get_tile(room, nx, ny) == '.') {
        cnt = g_monster_count[room];
        for (j = 0; j < cnt; j++) {
            if (j != self_idx &&
                g_monsters_rt[j].x == nx && g_monsters_rt[j].y == ny)
                goto try_secondary;
        }
        m->x = nx;
        m->y = ny;
        return 1;
    }

try_secondary:
    /* Secondary axis */
    if (adx >= ady) {
        if (dy == 0) return 0;
        nx = m->x;
        ny = (dy > 0) ? m->y + 1 : m->y - 1;
    } else {
        if (dx == 0) return 0;
        nx = (dx > 0) ? m->x + 1 : m->x - 1;
        ny = m->y;
    }

    if (nx < ROOM_W && ny < ROOM_H &&
        logic_get_tile(room, nx, ny) == '.') {
        cnt = g_monster_count[room];
        for (j = 0; j < cnt; j++) {
            if (j != self_idx &&
                g_monsters_rt[j].x == nx && g_monsters_rt[j].y == ny)
                return 0;
        }
        m->x = nx;
        m->y = ny;
        return 1;
    }
    return 0;
}

void monster_update_all(unsigned char room,
                        unsigned char px, unsigned char py)
{
    unsigned char i, cnt, dist;
    MonsterRT *m;
    const MonsterDef *def;

    cnt = g_monster_count[room];
    for (i = 0; i < cnt; i++) {
        m = &g_monsters_rt[i];
        def = &g_monsters[room][i];
        dist = manhattan(m->x, m->y, px, py);

        switch (m->state) {
        case MON_IDLE:
            if (m->detect_enabled && dist <= def->range) {
                m->state = MON_CHASE;
                monster_step_toward(m, room, px, py, i);
            }
            break;

        case MON_CHASE:
            if (dist > (unsigned char)(def->range * 2)) {
                m->state = MON_RETURNING;
                m->detect_enabled = 0;
                monster_step_toward(m, room,
                                    def->home_x, def->home_y, i);
            } else {
                monster_step_toward(m, room, px, py, i);
            }
            break;

        case MON_RETURNING:
            if (m->x == def->home_x && m->y == def->home_y) {
                m->state = MON_IDLE;
                m->detect_enabled = 1;
            } else {
                monster_step_toward(m, room,
                                    def->home_x, def->home_y, i);
            }
            break;
        }
    }
}

unsigned char monster_check_collision(unsigned char room,
                                      unsigned char px, unsigned char py)
{
    unsigned char i, cnt;
    cnt = g_monster_count[room];
    for (i = 0; i < cnt; i++) {
        if (g_monsters_rt[i].x == px && g_monsters_rt[i].y == py) {
            g_monsters_rt[i].state = MON_RETURNING;
            g_monsters_rt[i].detect_enabled = 0;
            return g_monsters[room][i].name_id + 1;
        }
    }
    return 0;
}
