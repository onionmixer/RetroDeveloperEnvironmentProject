#include "logic.h"
#include "room_data.h"
#include <string.h>

/*
 * Decompressed room grid buffer.
 * Placed at $0900 — free RAM between STARTUP ($0803-$0833) and HGR page ($2000).
 * Size: 100 rows * 50 packed bytes = 5000 bytes ($0900-$1C87).
 */
#define GRID_BUFFER ((unsigned char *)0x0900)

/*
 * File read buffer for RLE data.
 * Placed at $1CD0 — between GRID_BUFFER end ($1C88) and HGR page ($2000).
 * Size: 816 bytes ($1CD0-$1FFF).
 */
#define FILE_BUFFER ((unsigned char *)0x1CD0)

/* Direct ProDOS MLI file loader (in prodos_prefix.s) */
unsigned int __fastcall__ prodos_load_room(unsigned char room);

static unsigned char g_loaded_room = 0xFF;

static void rle_decompress(const unsigned char *src, unsigned int src_len,
                           unsigned char *dst)
{
    const unsigned char *end = src + src_len;
    unsigned char cmd, count, val;
    while (src < end) {
        cmd = *src++;
        if (cmd & 0x80) {
            count = (cmd & 0x7F) + 2;
            val = *src++;
            while (count--) *dst++ = val;
        } else {
            count = cmd + 1;
            while (count--) *dst++ = *src++;
        }
    }
}

void logic_decompress_room(unsigned char room)
{
    unsigned int n;

    if (room == g_loaded_room) return;

    n = prodos_load_room(room);
    if (n > 0) {
        rle_decompress(FILE_BUFFER, n, GRID_BUFFER);
        g_loaded_room = room;
        return;
    }

    /* Fallback: fill with walls */
    memset(GRID_BUFFER, 0x11, 5000);
    g_loaded_room = room;
}

void logic_init(GameState *g)
{
    g->room = 0;
    g->x = g_rooms[0].player_start_x;
    g->y = g_rooms[0].player_start_y;
    g->cam_x = 0;
    g->cam_y = 0;
    g->running = 1;
    g_loaded_room = 0xFF;
    logic_decompress_room(0);
}

TileCode logic_get_tile_code(unsigned char room,
                             unsigned char x, unsigned char y)
{
    unsigned char b, n;
    logic_decompress_room(room);
    b = GRID_BUFFER[(unsigned int)y * (ROOM_W / 2) + (x >> 1)];
    n = (unsigned char)((x & 1) ? (b & 0x0F) : (b >> 4));
    if (n > TILE_BOX) return TILE_WALL;
    return (TileCode)n;
}

void logic_update_camera(GameState *g)
{
    int cx = (int)g->x - (VIEW_W / 2);
    int cy = (int)g->y - (VIEW_H / 2);

    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx > (ROOM_W - VIEW_W)) cx = ROOM_W - VIEW_W;
    if (cy > (ROOM_H - VIEW_H)) cy = ROOM_H - VIEW_H;

    g->cam_x = (unsigned char)cx;
    g->cam_y = (unsigned char)cy;
}

MoveResult logic_try_move(const GameState *g, signed char dx, signed char dy,
                          unsigned char *tx, unsigned char *ty)
{
    int nx = (int)g->x + dx;
    int ny = (int)g->y + dy;
    TileCode tile;

    if (nx < 0 || ny < 0 || nx >= ROOM_W || ny >= ROOM_H) return MOVE_BLOCKED;

    *tx = (unsigned char)nx;
    *ty = (unsigned char)ny;
    tile = logic_get_tile_code(g->room, *tx, *ty);

    if (tile == TILE_WALL) return MOVE_BLOCKED;
    if (tile == TILE_DOOR) return MOVE_DOOR;
    if (tile == TILE_STAIR_DOWN || tile == TILE_STAIR_UP) return MOVE_STAIR;
    if (tile == TILE_BOX) return MOVE_BOX;

    return MOVE_OK;
}

const DoorDef *logic_find_door(unsigned char room,
                               unsigned char x, unsigned char y)
{
    unsigned char i;
    const RoomDef *r = &g_rooms[room];
    for (i = 0; i < r->door_count; i++) {
        const DoorDef *d = &r->doors[i];
        if (d->orientation == ORIENT_HORIZONTAL) {
            if ((d->x == x || (unsigned char)(d->x + 1) == x) && d->y == y)
                return d;
        } else {
            if (d->x == x && (d->y == y || (unsigned char)(d->y + 1) == y))
                return d;
        }
    }
    return 0;
}

const StairDef *logic_find_stair(unsigned char room,
                                 unsigned char x, unsigned char y)
{
    unsigned char i;
    const RoomDef *r = &g_rooms[room];
    for (i = 0; i < r->stair_count; i++) {
        const StairDef *s = &r->stairs[i];
        if (s->x == x && s->y == y)
            return s;
    }
    return 0;
}

const BoxDef *logic_find_box(unsigned char room,
                             unsigned char x, unsigned char y)
{
    unsigned char i;
    const RoomDef *r = &g_rooms[room];
    for (i = 0; i < r->box_count; i++) {
        const BoxDef *b = &r->boxes[i];
        if (b->orientation == ORIENT_HORIZONTAL) {
            if (b->y == y && (b->x == x || (unsigned char)(b->x + 1) == x))
                return b;
        } else {
            if (b->x == x && (b->y == y || (unsigned char)(b->y + 1) == y))
                return b;
        }
    }
    return 0;
}

unsigned char logic_do_door(GameState *g, const DoorDef *door)
{
    int sx, sy;
    int dx = 0, dy = 0;
    unsigned char tr = door->target_room;
    unsigned char td = door->target_door;
    const DoorDef *target;

    if (tr >= ROOM_COUNT || td >= g_rooms[tr].door_count) return 0;

    target = &g_rooms[tr].doors[td];
    if (target->wall_side == SIDE_NORTH) {
        sx = target->x; sy = target->y + 1; dy = 1;
    } else if (target->wall_side == SIDE_SOUTH) {
        sx = target->x; sy = target->y - 1; dy = -1;
    } else if (target->wall_side == SIDE_WEST) {
        sx = target->x + 1; sy = target->y; dx = 1;
    } else {
        sx = target->x - 1; sy = target->y; dx = -1;
    }

    while (sx >= 0 && sy >= 0 && sx < ROOM_W && sy < ROOM_H) {
        if (logic_get_tile_code(tr, (unsigned char)sx, (unsigned char)sy) == TILE_FLOOR) {
            g->room = tr;
            g->x = (unsigned char)sx;
            g->y = (unsigned char)sy;
            return 1;
        }
        sx += dx;
        sy += dy;
    }
    return 0;
}

unsigned char logic_do_stair(GameState *g, const StairDef *stair)
{
    unsigned char tr = stair->target_room;
    unsigned char ts = stair->target_stair;

    if (tr >= ROOM_COUNT || ts >= g_rooms[tr].stair_count) return 0;

    g->room = tr;
    g->x = g_rooms[tr].stairs[ts].x;
    g->y = g_rooms[tr].stairs[ts].y;
    return 1;
}
