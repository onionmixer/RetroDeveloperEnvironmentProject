#include "logic.h"

#include <string.h>

static unsigned char s_monster_x[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_y[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_home_x[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_home_y[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_range[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_state[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_detect_enabled[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_monster_patrol_wp_idx[ROOM_COUNT][MAX_MONSTERS];
static unsigned char s_room_phase[ROOM_COUNT];

static int monster_can_enter_tile(char tile)
{
    /* Keep patrol/chase safe: monsters only walk on floor. */
    return tile == '.';
}

static unsigned char clamp_u8(int v, int lo, int hi)
{
    if (v < lo) return (unsigned char)lo;
    if (v > hi) return (unsigned char)hi;
    return (unsigned char)v;
}

static int iabs(int v)
{
    return v < 0 ? -v : v;
}

static int manhattan_dist(int x1, int y1, int x2, int y2)
{
    return iabs(x1 - x2) + iabs(y1 - y2);
}

static int has_line_of_sight(unsigned char room, int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int adx = iabs(dx);
    int ady = iabs(dy);
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
            if (logic_get_tile(room, (unsigned char)x, (unsigned char)y) == '#') return 0;
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
            if (logic_get_tile(room, (unsigned char)x, (unsigned char)y) == '#') return 0;
        }
    }

    return 1;
}

static int monster_occupied(unsigned char room, int nx, int ny, int self_idx)
{
    unsigned char i;
    for (i = 0; i < g_monster_count[room]; i++) {
        if ((int)i == self_idx) continue;
        if (s_monster_x[room][i] == (unsigned char)nx &&
            s_monster_y[room][i] == (unsigned char)ny) {
            return 1;
        }
    }
    return 0;
}

static int monster_step_toward(unsigned char room, int idx, int tx, int ty)
{
    int mx = (int)s_monster_x[room][idx];
    int my = (int)s_monster_y[room][idx];
    int dx = tx - mx;
    int dy = ty - my;
    int adx = iabs(dx);
    int ady = iabs(dy);
    int step_x = dx == 0 ? 0 : (dx > 0 ? 1 : -1);
    int step_y = dy == 0 ? 0 : (dy > 0 ? 1 : -1);
    int try_x[2];
    int try_y[2];
    int k;

    if (dx == 0 && dy == 0) return 0;

    if (adx >= ady) {
        try_x[0] = mx + step_x; try_y[0] = my;
        try_x[1] = mx;          try_y[1] = my + step_y;
    } else {
        try_x[0] = mx;          try_y[0] = my + step_y;
        try_x[1] = mx + step_x; try_y[1] = my;
    }

    for (k = 0; k < 2; k++) {
        int nx = try_x[k];
        int ny = try_y[k];
        if (nx == mx && ny == my) continue;
        if (nx < 0 || nx >= ROOM_W || ny < 0 || ny >= ROOM_H) continue;
        if (!monster_can_enter_tile(logic_get_tile(room, (unsigned char)nx, (unsigned char)ny))) continue;
        if (monster_occupied(room, nx, ny, idx)) continue;

        s_monster_x[room][idx] = (unsigned char)nx;
        s_monster_y[room][idx] = (unsigned char)ny;
        return 1;
    }

    return 0;
}

static void get_patrol_waypoint(unsigned char room, int idx, int *wx, int *wy)
{
    int hx = (int)s_monster_home_x[room][idx];
    int hy = (int)s_monster_home_y[room][idx];
    int wp = (int)(s_monster_patrol_wp_idx[room][idx] & 3u);

    switch (wp) {
    case 0: *wx = hx + 3; *wy = hy; break;   /* east */
    case 1: *wx = hx; *wy = hy + 3; break;   /* south */
    case 2: *wx = hx - 3; *wy = hy; break;   /* west */
    default:*wx = hx; *wy = hy - 3; break;   /* north */
    }

    if (*wx < 1) *wx = 1;
    if (*wx > ROOM_W - 2) *wx = ROOM_W - 2;
    if (*wy < 1) *wy = 1;
    if (*wy > ROOM_H - 2) *wy = ROOM_H - 2;
}

void logic_init(GameState *g)
{
    unsigned char r, i;

    memset(g, 0, sizeof(*g));
    g->room = 0;
    g->x = g_player_start_x[0];
    g->y = g_player_start_y[0];
    g->monster_tick = 0;
    g->monster_move_enabled = 1;
    g->running = 1;
    g->status[0] = '\0';

    for (r = 0; r < ROOM_COUNT; r++) {
        s_room_phase[r] = 0;
        for (i = 0; i < g_monster_count[r]; i++) {
            unsigned char range = g_monsters[r][i].range;
            if (range < 4) range = 4;

            s_monster_x[r][i] = g_monsters[r][i].x;
            s_monster_y[r][i] = g_monsters[r][i].y;
            s_monster_home_x[r][i] = g_monsters[r][i].x;
            s_monster_home_y[r][i] = g_monsters[r][i].y;
            s_monster_range[r][i] = range;
            s_monster_state[r][i] = (unsigned char)MONSTER_PATROL;
            s_monster_detect_enabled[r][i] = 1;
            s_monster_patrol_wp_idx[r][i] = 0;
        }
    }
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
        const DoorX68 *d = &g_doors[room][i];
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
        const StairX68 *s = &g_stairs[room][i];
        if (s->x == x && s->y == y) return i;
    }
    return -1;
}

int logic_find_box_at(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char i;
    if (room >= ROOM_COUNT) return -1;
    for (i = 0; i < g_box_count[room]; i++) {
        const BoxX68 *b = &g_boxes[room][i];
        if (b->orientation == 0) {
            if (b->y == y && (b->x == x || (unsigned char)(b->x + 1) == x)) return i;
        } else {
            if (b->x == x && (b->y == y || (unsigned char)(b->y + 1) == y)) return i;
        }
    }
    return -1;
}

int logic_find_monster_at(unsigned char room, unsigned char x, unsigned char y)
{
    unsigned char i;
    if (room >= ROOM_COUNT) return -1;
    for (i = 0; i < g_monster_count[room]; i++) {
        if (s_monster_x[room][i] == x && s_monster_y[room][i] == y) return i;
    }
    return -1;
}

void logic_monster_force_return(unsigned char room, int monster_idx)
{
    if (room >= ROOM_COUNT) return;
    if (monster_idx < 0 || (unsigned char)monster_idx >= g_monster_count[room]) return;
    s_monster_state[room][monster_idx] = (unsigned char)MONSTER_RETURNING;
    s_monster_detect_enabled[room][monster_idx] = 0;
}

MonsterState logic_get_monster_state(unsigned char room, int monster_idx)
{
    if (room >= ROOM_COUNT) return MONSTER_PATROL;
    if (monster_idx < 0 || (unsigned char)monster_idx >= g_monster_count[room]) return MONSTER_PATROL;
    return (MonsterState)s_monster_state[room][monster_idx];
}

unsigned char logic_get_monster_detect_enabled(unsigned char room, int monster_idx)
{
    if (room >= ROOM_COUNT) return 0;
    if (monster_idx < 0 || (unsigned char)monster_idx >= g_monster_count[room]) return 0;
    return s_monster_detect_enabled[room][monster_idx];
}

void logic_get_monster_pos(unsigned char room, int monster_idx, unsigned char *x, unsigned char *y)
{
    if (x) *x = 0;
    if (y) *y = 0;
    if (room >= ROOM_COUNT) return;
    if (monster_idx < 0 || (unsigned char)monster_idx >= g_monster_count[room]) return;
    if (x) *x = s_monster_x[room][monster_idx];
    if (y) *y = s_monster_y[room][monster_idx];
}

void logic_update_monsters(const GameState *g)
{
    unsigned char room = g->room;
    unsigned char phase = s_room_phase[room];
    unsigned char i;

    for (i = 0; i < g_monster_count[room]; i++) {
        int mx = (int)s_monster_x[room][i];
        int my = (int)s_monster_y[room][i];
        int px = (int)g->x;
        int py = (int)g->y;
        int dist = manhattan_dist(mx, my, px, py);

        (void)phase;

        if (s_monster_state[room][i] == (unsigned char)MONSTER_PATROL) {
            if (s_monster_detect_enabled[room][i] &&
                dist <= (int)s_monster_range[room][i] &&
                has_line_of_sight(room, mx, my, px, py)) {
                s_monster_state[room][i] = (unsigned char)MONSTER_CHASE;
            } else {
                int wx, wy;
                get_patrol_waypoint(room, i, &wx, &wy);

                if (!monster_can_enter_tile(logic_get_tile(room, (unsigned char)wx, (unsigned char)wy))) {
                    s_monster_patrol_wp_idx[room][i] = (unsigned char)((s_monster_patrol_wp_idx[room][i] + 1u) & 3u);
                    get_patrol_waypoint(room, i, &wx, &wy);
                }

                (void)monster_step_toward(room, i, wx, wy);

                if (s_monster_x[room][i] == (unsigned char)wx &&
                    s_monster_y[room][i] == (unsigned char)wy) {
                    s_monster_patrol_wp_idx[room][i] = (unsigned char)((s_monster_patrol_wp_idx[room][i] + 1u) & 3u);
                }
                continue;
            }
        }

        if (s_monster_state[room][i] == (unsigned char)MONSTER_CHASE) {
            mx = (int)s_monster_x[room][i];
            my = (int)s_monster_y[room][i];
            dist = manhattan_dist(mx, my, px, py);

            if (dist > (int)s_monster_range[room][i] * 2) {
                s_monster_state[room][i] = (unsigned char)MONSTER_RETURNING;
                s_monster_detect_enabled[room][i] = 0;
            } else if (dist > (int)s_monster_range[room][i] &&
                       !has_line_of_sight(room, mx, my, px, py)) {
                s_monster_state[room][i] = (unsigned char)MONSTER_RETURNING;
                s_monster_detect_enabled[room][i] = 0;
            } else {
                (void)monster_step_toward(room, i, px, py);
                continue;
            }
        }

        if (s_monster_state[room][i] == (unsigned char)MONSTER_RETURNING) {
            if (s_monster_x[room][i] == s_monster_home_x[room][i] &&
                s_monster_y[room][i] == s_monster_home_y[room][i]) {
                s_monster_state[room][i] = (unsigned char)MONSTER_PATROL;
                s_monster_detect_enabled[room][i] = 1;
                s_monster_patrol_wp_idx[room][i] = 0;
            } else {
                (void)monster_step_toward(room, i,
                                          (int)s_monster_home_x[room][i],
                                          (int)s_monster_home_y[room][i]);
            }
        }
    }

    s_room_phase[room] = (unsigned char)((phase + 1) & 3);
}

unsigned char logic_get_monster_phase(unsigned char room)
{
    if (room >= ROOM_COUNT) return 0;
    return s_room_phase[room];
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
    const DoorX68 *src;
    const DoorX68 *dst;
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
    if (dst->wall_side == 0) {
        sx = dst->x;
        sy = dst->y + 1;
        dy = 1;
    } else if (dst->wall_side == 1) {
        sx = dst->x;
        sy = dst->y - 1;
        dy = -1;
    } else if (dst->wall_side == 2) {
        sx = dst->x + 1;
        sy = dst->y;
        dx = 1;
    } else {
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
    const StairX68 *src;
    const StairX68 *dst;

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
