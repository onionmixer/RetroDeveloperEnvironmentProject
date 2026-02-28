#include "monster.h"
#include "room_data.h"
#include "logic.h"

RoomRuntime g_runtime[ROOM_COUNT];

static int abs_i(int v)
{
    if (v < 0) return -v;
    return v;
}

int monster_index_at(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char i;
    unsigned char count = g_rooms[room].monster_count;
    for (i = 0; i < count; i++) {
        if (g_runtime[room].monsters[i].x == x &&
            g_runtime[room].monsters[i].y == y)
            return (int)i;
    }
    return -1;
}

static int has_line_of_sight(unsigned char room,
                             int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int adx = abs_i(dx);
    int ady = abs_i(dy);
    int sx = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
    int sy = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
    int x = x0;
    int y = y0;

    if (adx == 0 && ady == 0) return 1;

    if (adx >= ady) {
        int err = adx / 2;
        int i;
        for (i = 0; i < adx; i++) {
            x += sx;
            err -= ady;
            if (err < 0) {
                y += sy;
                err += adx;
            }
            if (x == x1 && y == y1) return 1;
            if (x < 0 || x >= ROOM_W || y < 0 || y >= ROOM_H) return 0;
            if (logic_get_tile_code(room, (unsigned char)x, (unsigned char)y) == TILE_WALL) return 0;
        }
    } else {
        int err = ady / 2;
        int i;
        for (i = 0; i < ady; i++) {
            y += sy;
            err -= adx;
            if (err < 0) {
                x += sx;
                err += ady;
            }
            if (x == x1 && y == y1) return 1;
            if (x < 0 || x >= ROOM_W || y < 0 || y >= ROOM_H) return 0;
            if (logic_get_tile_code(room, (unsigned char)x, (unsigned char)y) == TILE_WALL) return 0;
        }
    }
    return 1;
}

static void get_patrol_waypoint(const MonsterDef *md, unsigned char idx, int *ox, int *oy)
{
    switch (idx % 4) {
    case 0:
        *ox = (int)md->home_x + 3;
        *oy = (int)md->home_y;
        break;
    case 1:
        *ox = (int)md->home_x;
        *oy = (int)md->home_y + 3;
        break;
    case 2:
        *ox = (int)md->home_x - 3;
        *oy = (int)md->home_y;
        break;
    default:
        *ox = (int)md->home_x;
        *oy = (int)md->home_y - 3;
        break;
    }

    if (*ox < 1) *ox = 1;
    if (*ox > (ROOM_W - 2)) *ox = ROOM_W - 2;
    if (*oy < 1) *oy = 1;
    if (*oy > (ROOM_H - 2)) *oy = ROOM_H - 2;
}

static int monster_occupied(unsigned char room, int nx, int ny, unsigned char self_idx)
{
    unsigned char i;
    unsigned char count = g_rooms[room].monster_count;
    for (i = 0; i < count; i++) {
        if (i == self_idx) continue;
        if ((int)g_runtime[room].monsters[i].x == nx &&
            (int)g_runtime[room].monsters[i].y == ny)
            return 1;
    }
    return 0;
}

static int monster_step_toward(unsigned char room, unsigned char idx, int tx, int ty)
{
    MonsterRuntime *m = &g_runtime[room].monsters[idx];
    int dx = tx - (int)m->x;
    int dy = ty - (int)m->y;
    int adx = abs_i(dx);
    int ady = abs_i(dy);
    int step_x = dx == 0 ? 0 : (dx > 0 ? 1 : -1);
    int step_y = dy == 0 ? 0 : (dy > 0 ? 1 : -1);
    int try_x0, try_y0, try_x1, try_y1;
    int k;

    if (dx == 0 && dy == 0) return 0;

    if (adx >= ady) {
        try_x0 = (int)m->x + step_x;
        try_y0 = (int)m->y;
        try_x1 = (int)m->x;
        try_y1 = (int)m->y + step_y;
    } else {
        try_x0 = (int)m->x;
        try_y0 = (int)m->y + step_y;
        try_x1 = (int)m->x + step_x;
        try_y1 = (int)m->y;
    }

    for (k = 0; k < 2; k++) {
        int nx = (k == 0) ? try_x0 : try_x1;
        int ny = (k == 0) ? try_y0 : try_y1;
        if (nx < 0 || ny < 0 || nx >= ROOM_W || ny >= ROOM_H) continue;
        if (logic_get_tile_code(room, (unsigned char)nx, (unsigned char)ny) != TILE_FLOOR) continue;
        if (monster_occupied(room, nx, ny, idx)) continue;
        m->x = (unsigned char)nx;
        m->y = (unsigned char)ny;
        return 1;
    }

    return 0;
}

static void monster_update_one(unsigned char room, unsigned char idx,
                               unsigned char px, unsigned char py)
{
    const MonsterDef *md = &g_rooms[room].monsters[idx];
    MonsterRuntime *m = &g_runtime[room].monsters[idx];
    int dist = abs_i((int)m->x - (int)px) + abs_i((int)m->y - (int)py);

    if (m->state == MONSTER_PATROL) {
        int wx, wy;

        if (m->detect_enabled && dist <= (int)md->range &&
            has_line_of_sight(room, (int)m->x, (int)m->y, (int)px, (int)py)) {
            m->state = MONSTER_CHASE;
        }

        if (m->state == MONSTER_PATROL) {
            get_patrol_waypoint(md, m->patrol_wp_idx, &wx, &wy);
            if (logic_get_tile_code(room, (unsigned char)wx, (unsigned char)wy) != TILE_FLOOR) {
                m->patrol_wp_idx = (unsigned char)((m->patrol_wp_idx + 1) % 4);
                get_patrol_waypoint(md, m->patrol_wp_idx, &wx, &wy);
            }
            monster_step_toward(room, idx, wx, wy);
            if ((int)m->x == wx && (int)m->y == wy)
                m->patrol_wp_idx = (unsigned char)((m->patrol_wp_idx + 1) % 4);
            return;
        }
    }

    if (m->state == MONSTER_CHASE) {
        dist = abs_i((int)m->x - (int)px) + abs_i((int)m->y - (int)py);
        if (dist > (int)md->range * 2) {
            m->state = MONSTER_RETURNING;
            m->detect_enabled = 0;
        } else if (dist > (int)md->range &&
                   !has_line_of_sight(room, (int)m->x, (int)m->y, (int)px, (int)py)) {
            m->state = MONSTER_RETURNING;
            m->detect_enabled = 0;
        }

        if (m->state == MONSTER_CHASE) {
            monster_step_toward(room, idx, (int)px, (int)py);
            return;
        }
    }

    if (m->state == MONSTER_RETURNING) {
        if (m->x == md->home_x && m->y == md->home_y) {
            m->state = MONSTER_PATROL;
            m->detect_enabled = 1;
            m->patrol_wp_idx = 0;
            return;
        }
        monster_step_toward(room, idx, (int)md->home_x, (int)md->home_y);
    }
}

void monster_init_all(void)
{
    unsigned char r;
    for (r = 0; r < ROOM_COUNT; r++) {
        unsigned char i;
        unsigned char count = g_rooms[r].monster_count;
        for (i = 0; i < count; i++) {
            g_runtime[r].monsters[i].x = g_rooms[r].monsters[i].home_x;
            g_runtime[r].monsters[i].y = g_rooms[r].monsters[i].home_y;
            g_runtime[r].monsters[i].state = MONSTER_PATROL;
            g_runtime[r].monsters[i].detect_enabled = 1;
            g_runtime[r].monsters[i].patrol_wp_idx = 0;
        }
    }
}

void monster_update_all(unsigned char room,
                        unsigned char px, unsigned char py)
{
    unsigned char i;
    unsigned char count = g_rooms[room].monster_count;
    for (i = 0; i < count; i++) {
        monster_update_one(room, i, px, py);
    }
}

int monster_check_collision(const GameState *st)
{
    int idx = monster_index_at(st->room, st->x, st->y);
    if (idx >= 0) {
        MonsterRuntime *m = &g_runtime[st->room].monsters[(unsigned char)idx];
        m->state = MONSTER_RETURNING;
        m->detect_enabled = 0;
    }
    return idx;
}

void monster_format_encounter_msg(char *buf, unsigned char buf_size,
                                  unsigned char room, unsigned char idx)
{
    const char *name = g_rooms[room].monsters[idx].name;
    unsigned char p = 0;
    unsigned char i = 0;
    const char *prefix = "Encountered ";

    while (prefix[p] != '\0' && p < (buf_size - 2)) {
        buf[p] = prefix[p];
        p++;
    }
    while (name[i] != '\0' && p < (buf_size - 2)) {
        buf[p++] = name[i++];
    }
    buf[p++] = '!';
    buf[p] = '\0';
}
