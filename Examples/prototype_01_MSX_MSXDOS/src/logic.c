#include "logic.h"

#include <string.h>

static unsigned char clamp_u8(int v, int lo, int hi)
{
    if (v < lo) return (unsigned char)lo;
    if (v > hi) return (unsigned char)hi;
    return (unsigned char)v;
}

void logic_init(GameState *g)
{
    memset(g, 0, sizeof(*g));
    g->room = 0;
    g->x = g_player_start_x[0];
    g->y = g_player_start_y[0];
    g->running = 1;
    g->status[0] = '\0';
}

void logic_update_camera(GameState *g)
{
    int cx = (int)g->x - (VIEW_W / 2);
    int cy = (int)g->y - (VIEW_H / 2);
    g->cam_x = clamp_u8(cx, 0, ROOM_W - VIEW_W);
    g->cam_y = clamp_u8(cy, 0, ROOM_H - VIEW_H);
}

char logic_get_tile(unsigned char room, unsigned char x, unsigned char y)
{
    if (room >= ROOM_COUNT || x >= ROOM_W || y >= ROOM_H) return '#';
    return g_room_grids[room][y][x];
}

int logic_find_door_at(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char i;
    if (room >= ROOM_COUNT) return -1;
    for (i = 0; i < g_door_count[room]; i++) {
        const DoorMsx *d = &g_doors[room][i];
        if (d->orientation == 0) {
            if (d->y == y && (d->x == x || (unsigned char)(d->x + 1) == x)) return i;
        } else {
            if (d->x == x && (d->y == y || (unsigned char)(d->y + 1) == y)) return i;
        }
    }
    return -1;
}

int logic_find_stair_at(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char i;
    if (room >= ROOM_COUNT) return -1;
    for (i = 0; i < g_stair_count[room]; i++) {
        const StairMsx *s = &g_stairs[room][i];
        if (s->x == x && s->y == y) return i;
    }
    return -1;
}

int logic_find_box_at(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char i;
    if (room >= ROOM_COUNT) return -1;
    for (i = 0; i < g_box_count[room]; i++) {
        const BoxMsx *b = &g_boxes[room][i];
        if (b->orientation == 0) {
            if (b->y == y && (b->x == x || (unsigned char)(b->x + 1) == x)) return i;
        } else {
            if (b->x == x && (b->y == y || (unsigned char)(b->y + 1) == y)) return i;
        }
    }
    return -1;
}

MoveResult logic_try_move(GameState *g, signed char dx, signed char dy)
{
    int nx = (int)g->x + dx;
    int ny = (int)g->y + dy;
    char tile;

    if (nx < 0 || nx >= ROOM_W || ny < 0 || ny >= ROOM_H) return MOVE_BLOCKED;

    tile = logic_get_tile(g->room, (unsigned char)nx, (unsigned char)ny);
    if (tile == '#') return MOVE_BLOCKED;
    if (tile == '%') return MOVE_BLOCKED_BOX;
    if (tile == '@') return MOVE_DOOR_PENDING;
    if (tile == '<' || tile == '>') return MOVE_STAIR_PENDING;

    g->x = (unsigned char)nx;
    g->y = (unsigned char)ny;
    return MOVE_OK;
}

int logic_door_transition(GameState *g, int door_idx)
{
    const DoorMsx *src;
    const DoorMsx *dst;
    int sx, sy, dx, dy;
    unsigned char room;

    if (door_idx < 0) return 0;
    if ((unsigned char)door_idx >= g_door_count[g->room]) return 0;

    src = &g_doors[g->room][door_idx];
    if (src->target_room >= ROOM_COUNT || src->target_index >= MAX_DOORS) return 0;

    room = src->target_room;
    if (src->target_index >= g_door_count[room]) return 0;

    dst = &g_doors[room][src->target_index];

    dx = 0;
    dy = 0;
    if (dst->wall_side == 0) { /* north */
        sx = dst->x;
        sy = dst->y + 1;
        dy = 1;
    } else if (dst->wall_side == 1) { /* south */
        sx = dst->x;
        sy = dst->y - 1;
        dy = -1;
    } else if (dst->wall_side == 2) { /* west */
        sx = dst->x + 1;
        sy = dst->y;
        dx = 1;
    } else { /* east */
        sx = dst->x - 1;
        sy = dst->y;
        dx = -1;
    }

    while (sx >= 0 && sx < ROOM_W && sy >= 0 && sy < ROOM_H) {
        if (logic_get_tile(room, (unsigned char)sx, (unsigned char)sy) == '.') break;
        sx += dx;
        sy += dy;
    }

    if (sx < 0 || sx >= ROOM_W || sy < 0 || sy >= ROOM_H) return 0;

    g->room = room;
    g->x = (unsigned char)sx;
    g->y = (unsigned char)sy;
    return 1;
}

int logic_stair_transition(GameState *g, int stair_idx)
{
    const StairMsx *src;
    const StairMsx *dst;

    if (stair_idx < 0) return 0;
    if ((unsigned char)stair_idx >= g_stair_count[g->room]) return 0;

    src = &g_stairs[g->room][stair_idx];
    if (src->target_room >= ROOM_COUNT || src->target_index >= MAX_STAIRS) return 0;
    if (src->target_index >= g_stair_count[src->target_room]) return 0;

    dst = &g_stairs[src->target_room][src->target_index];

    g->room = src->target_room;
    g->x = dst->x;
    g->y = dst->y;
    return 1;
}
