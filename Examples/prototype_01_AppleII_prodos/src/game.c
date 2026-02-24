#include "engine.h"
#include "room_data.h"

#include <conio.h>
#include <string.h>

typedef enum {
    MOVE_OK = 0,
    MOVE_BLOCKED,
    MOVE_DOOR,
    MOVE_STAIR,
    MOVE_BOX
} MoveResult;

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char state;
    unsigned char detect_enabled;
    unsigned char patrol_wp_idx;
} MonsterRuntime;

typedef struct {
    MonsterRuntime monsters[MAX_MONSTERS];
} RoomRuntime;

static char g_status[MSG_BUF];
static const char *k_spaces40 = "                                        ";
static const char k_tile_chars[] = ".#@<>%";
static RoomRuntime g_runtime[ROOM_COUNT];
static unsigned char g_help_scroll_offset;

static const char *k_help_raw[] = {
    "F-SpagetyProto - Help Guide",
    "",
    "Movement Controls:",
    "  W  Move up",
    "  S  Move down",
    "  A  Move left",
    "  D  Move right",
    "",
    "Interaction:",
    "  When you approach a door, stair, or box,",
    "  a confirmation prompt appears.",
    "  Press 1 to confirm the action, or",
    "  press 0 to cancel and stay in place.",
    "",
    "Map Symbols:",
    "  !  Player (you)",
    "  .  Floor (walkable)",
    "  #  Wall (impassable)",
    "  @  Door (room transition)",
    "  <  Stairs going down one level",
    "  >  Stairs going up one level",
    "  %  Box (open to see effect and items)",
    "  $  Monster (hostile creature)",
    "",
    "Boxes contain items and may have special effects.",
    "Some boxes are placed by villains and may trigger",
    "negative effects when opened.",
    "Opening a box shows its effect first, then",
    "displays the item list inside.",
    "",
    "Monsters ($) patrol rooms and will chase you",
    "if you get too close.",
    "Colliding with a monster causes it to retreat",
    "to its home position.",
    "",
    "Runtime keys:",
    "  W A S D (also I J K L): move",
    "  H: open this help",
    "  Q: quit prompt (1/0)",
    "",
    "The viewport shows a 10x10 section",
    "of the current 100x100 room grid.",
    "Coordinates and floor are displayed",
    "below the map."
};

static unsigned char prompt_quit(void);

static TileCode room_tile_code(const RoomDef *room, unsigned char x, unsigned char y)
{
    unsigned char b = room->grid_packed[y][x >> 1];
    unsigned char n = (unsigned char)((x & 1) ? (b & 0x0F) : (b >> 4));
    if (n > TILE_BOX) return TILE_WALL;
    return (TileCode)n;
}

static char room_tile_char(const RoomDef *room, unsigned char x, unsigned char y)
{
    return k_tile_chars[(unsigned char)room_tile_code(room, x, y)];
}

static void clear_line(unsigned char y)
{
    gotoxy(0, y);
    cputs(k_spaces40);
}

static unsigned char help_total_lines(void)
{
    unsigned char i;
    unsigned char total = 0;
    for (i = 0; i < (unsigned char)(sizeof(k_help_raw) / sizeof(k_help_raw[0])); i++) {
        const char *src = k_help_raw[i];
        unsigned char len = (unsigned char)strlen(src);
        unsigned char pos = 0;

        if (len == 0) {
            total++;
            continue;
        }
        while (pos < len) {
            unsigned char remaining = (unsigned char)(len - pos);
            if (remaining <= 38) {
                total++;
                break;
            } else {
                signed char cut = 38;
                while (cut > 0 && src[pos + (unsigned char)cut] != ' ')
                    cut--;
                total++;
                if (cut <= 0) pos = (unsigned char)(pos + 38);
                else pos = (unsigned char)(pos + (unsigned char)cut + 1);
            }
        }
    }
    return total;
}

static unsigned char help_line_at(unsigned char line_idx, char *out)
{
    unsigned char i;
    unsigned char logical = 0;
    for (i = 0; i < (unsigned char)(sizeof(k_help_raw) / sizeof(k_help_raw[0])); i++) {
        const char *src = k_help_raw[i];
        unsigned char len = (unsigned char)strlen(src);
        unsigned char pos = 0;

        if (len == 0) {
            if (logical == line_idx) {
                out[0] = '\0';
                return 1;
            }
            logical++;
            continue;
        }

        while (pos < len) {
            unsigned char remaining = (unsigned char)(len - pos);
            unsigned char chunk_len;
            unsigned char next_pos;
            unsigned char j;

            if (remaining <= 38) {
                chunk_len = remaining;
                next_pos = len;
            } else {
                signed char cut = 38;
                while (cut > 0 && src[pos + (unsigned char)cut] != ' ')
                    cut--;
                if (cut <= 0) {
                    chunk_len = 38;
                    next_pos = (unsigned char)(pos + 38);
                } else {
                    chunk_len = (unsigned char)cut;
                    next_pos = (unsigned char)(pos + (unsigned char)cut + 1);
                }
            }

            if (logical == line_idx) {
                for (j = 0; j < chunk_len; j++) out[j] = src[pos + j];
                out[chunk_len] = '\0';
                return 1;
            }
            logical++;
            pos = next_pos;
        }
    }
    out[0] = '\0';
    return 0;
}

static void help_scroll(signed char dir)
{
    int next = (int)g_help_scroll_offset + dir;
    int total = (int)help_total_lines();
    int max_offset = total - 23;
    if (max_offset < 0) max_offset = 0;
    if (next < 0) next = 0;
    if (next > max_offset) next = max_offset;
    g_help_scroll_offset = (unsigned char)next;
}

static void help_render(void)
{
    unsigned char row;
    char line[39];
    for (row = 0; row < 23; row++) {
        clear_line(row);
        gotoxy(0, row);
        if (help_line_at((unsigned char)(g_help_scroll_offset + row), line))
            cputs(line);
    }
    clear_line(23);
    gotoxy(0, 23);
    cputs("W/S scroll  SPACE return  Q quit");
}

static void set_status(const char *msg)
{
    unsigned char i = 0;
    while (msg[i] != '\0' && i < (MSG_BUF - 1)) {
        g_status[i] = msg[i];
        i++;
    }
    g_status[i] = '\0';
}

static void set_encounter_status(unsigned char room_idx, unsigned char monster_idx)
{
    const char *name = g_rooms[room_idx].monsters[monster_idx].name;
    unsigned char p = 0;
    unsigned char i = 0;

    const char *prefix = "Encountered ";
    const char *suffix = "!";

    while (prefix[p] != '\0' && p < (MSG_BUF - 1)) {
        g_status[p] = prefix[p];
        p++;
    }
    while (name[i] != '\0' && p < (MSG_BUF - 2)) {
        g_status[p++] = name[i++];
    }
    i = 0;
    while (suffix[i] != '\0' && p < (MSG_BUF - 1)) {
        g_status[p++] = suffix[i++];
    }
    g_status[p] = '\0';
}

static void append_u8(char *buf, unsigned char *pos, unsigned char val)
{
    char tmp[4];
    unsigned char len = 0;
    unsigned char i;

    if (val >= 100) {
        tmp[len++] = (char)('0' + (val / 100));
        val = (unsigned char)(val % 100);
        tmp[len++] = (char)('0' + (val / 10));
        tmp[len++] = (char)('0' + (val % 10));
    } else if (val >= 10) {
        tmp[len++] = (char)('0' + (val / 10));
        tmp[len++] = (char)('0' + (val % 10));
    } else {
        tmp[len++] = (char)('0' + val);
    }

    for (i = 0; i < len; i++) {
        buf[*pos] = tmp[i];
        (*pos)++;
    }
    buf[*pos] = '\0';
}

static int abs_i(int v)
{
    if (v < 0) return -v;
    return v;
}

static int monster_index_at(unsigned char room_idx, unsigned char x, unsigned char y)
{
    unsigned char i;
    unsigned char count = g_rooms[room_idx].monster_count;
    for (i = 0; i < count; i++) {
        if (g_runtime[room_idx].monsters[i].x == x &&
            g_runtime[room_idx].monsters[i].y == y)
            return (int)i;
    }
    return -1;
}

static int has_line_of_sight(unsigned char room_idx,
                             int x0, int y0, int x1, int y1)
{
    const RoomDef *room = &g_rooms[room_idx];
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
            if (room_tile_code(room, (unsigned char)x, (unsigned char)y) == TILE_WALL) return 0;
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
            if (room_tile_code(room, (unsigned char)x, (unsigned char)y) == TILE_WALL) return 0;
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

static int monster_occupied(unsigned char room_idx, int nx, int ny, unsigned char self_idx)
{
    unsigned char i;
    unsigned char count = g_rooms[room_idx].monster_count;
    for (i = 0; i < count; i++) {
        if (i == self_idx) continue;
        if ((int)g_runtime[room_idx].monsters[i].x == nx &&
            (int)g_runtime[room_idx].monsters[i].y == ny)
            return 1;
    }
    return 0;
}

static int monster_step_toward(unsigned char room_idx, unsigned char idx, int tx, int ty)
{
    const RoomDef *room = &g_rooms[room_idx];
    MonsterRuntime *m = &g_runtime[room_idx].monsters[idx];
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
        if (room_tile_code(room, (unsigned char)nx, (unsigned char)ny) != TILE_FLOOR) continue;
        if (monster_occupied(room_idx, nx, ny, idx)) continue;
        m->x = (unsigned char)nx;
        m->y = (unsigned char)ny;
        return 1;
    }

    return 0;
}

static void monster_update_one(unsigned char room_idx, unsigned char idx,
                               unsigned char px, unsigned char py)
{
    const MonsterDef *md = &g_rooms[room_idx].monsters[idx];
    MonsterRuntime *m = &g_runtime[room_idx].monsters[idx];
    int dist = abs_i((int)m->x - (int)px) + abs_i((int)m->y - (int)py);

    if (m->state == MONSTER_PATROL) {
        int wx, wy;

        if (m->detect_enabled && dist <= (int)md->range &&
            has_line_of_sight(room_idx, (int)m->x, (int)m->y, (int)px, (int)py)) {
            m->state = MONSTER_CHASE;
        }

        if (m->state == MONSTER_PATROL) {
            get_patrol_waypoint(md, m->patrol_wp_idx, &wx, &wy);
            if (room_tile_code(&g_rooms[room_idx], (unsigned char)wx, (unsigned char)wy) != TILE_FLOOR) {
                m->patrol_wp_idx = (unsigned char)((m->patrol_wp_idx + 1) % 4);
                get_patrol_waypoint(md, m->patrol_wp_idx, &wx, &wy);
            }
            monster_step_toward(room_idx, idx, wx, wy);
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
                   !has_line_of_sight(room_idx, (int)m->x, (int)m->y, (int)px, (int)py)) {
            m->state = MONSTER_RETURNING;
            m->detect_enabled = 0;
        }

        if (m->state == MONSTER_CHASE) {
            monster_step_toward(room_idx, idx, (int)px, (int)py);
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
        monster_step_toward(room_idx, idx, (int)md->home_x, (int)md->home_y);
    }
}

static void monster_update_all(unsigned char room_idx, unsigned char px, unsigned char py)
{
    unsigned char i;
    unsigned char count = g_rooms[room_idx].monster_count;
    for (i = 0; i < count; i++) {
        monster_update_one(room_idx, i, px, py);
    }
}

static int monster_check_collision_and_mark(const GameState *st)
{
    int idx = monster_index_at(st->room, st->x, st->y);
    if (idx >= 0) {
        MonsterRuntime *m = &g_runtime[st->room].monsters[(unsigned char)idx];
        m->state = MONSTER_RETURNING;
        m->detect_enabled = 0;
    }
    return idx;
}

static void init_runtime(void)
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

static void build_header(const GameState *st, char *out)
{
    const RoomDef *r = &g_rooms[st->room];
    unsigned char p = 0;
    unsigned char i;

    out[p++] = 'R'; out[p++] = ':';
    for (i = 0; r->id[i] != '\0' && p < 38; i++) out[p++] = r->id[i];
    out[p++] = ' ';
    out[p++] = 'X'; out[p++] = ':';
    append_u8(out, &p, st->x);
    out[p++] = ' ';
    out[p++] = 'Y'; out[p++] = ':';
    append_u8(out, &p, st->y);
    out[p++] = ' ';
    out[p++] = 'Z'; out[p++] = ':';
    if (r->z_level < 0) {
        out[p++] = '-';
        append_u8(out, &p, (unsigned char)(-r->z_level));
    } else {
        append_u8(out, &p, (unsigned char)r->z_level);
    }
    out[p] = '\0';
}

static void update_camera(GameState *st)
{
    int cx = (int)st->x - (VIEW_W / 2);
    int cy = (int)st->y - (VIEW_H / 2);

    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    if (cx > (ROOM_W - VIEW_W)) cx = ROOM_W - VIEW_W;
    if (cy > (ROOM_H - VIEW_H)) cy = ROOM_H - VIEW_H;

    st->cam_x = (unsigned char)cx;
    st->cam_y = (unsigned char)cy;
}

static void render_header(const GameState *st)
{
    char header[41];

    build_header(st, header);
    clear_line(11);
    gotoxy(0, 11);
    cputs(header);
}

static void render_status(void)
{
    clear_line(12);
    gotoxy(0, 12);
    cputs(g_status);
}

static void render_hint(void)
{
    clear_line(13);
    gotoxy(0, 13);
    cputs("Press H for help");
    clear_line(14);
    gotoxy(0, 14);
    cputs("Move:WASD  Quit:Q  Confirm:1/0");
}

static void render_hud(const GameState *st)
{
    render_header(st);
    render_status();
    render_hint();
}

static void render_map(const GameState *st)
{
    char line[41];
    const RoomDef *r = &g_rooms[st->room];
    unsigned char row;
    unsigned char col;

    for (row = 0; row < VIEW_H; row++) {
        for (col = 0; col < VIEW_W; col++) {
            unsigned char wx = (unsigned char)(st->cam_x + col);
            unsigned char wy = (unsigned char)(st->cam_y + row);
            char ch;

            if (wx == st->x && wy == st->y) {
                ch = '!';
            } else if (monster_index_at(st->room, wx, wy) >= 0) {
                ch = '$';
            } else {
                ch = room_tile_char(r, wx, wy);
            }
            line[col] = ch;
        }
        line[VIEW_W] = '\0';
        gotoxy(0, row);
        cputs(line);
    }
}

static void render_full(const GameState *st)
{
    clrscr();
    render_map(st);
    render_hud(st);
}

static void render_player_step(const GameState *prev, const GameState *st)
{
    const RoomDef *r = &g_rooms[st->room];
    unsigned char old_col = (unsigned char)(prev->x - prev->cam_x);
    unsigned char old_row = (unsigned char)(prev->y - prev->cam_y);
    unsigned char new_col = (unsigned char)(st->x - st->cam_x);
    unsigned char new_row = (unsigned char)(st->y - st->cam_y);

    gotoxy(old_col, old_row);
    if (monster_index_at(st->room, prev->x, prev->y) >= 0)
        cputc('$');
    else
        cputc(room_tile_char(r, prev->x, prev->y));

    gotoxy(new_col, new_row);
    cputc('!');
}

static unsigned char prompt_yes_no(const char *msg)
{
    set_status(msg);
    clear_line(13);
    gotoxy(0, 13);
    cputs(msg);
    clear_line(14);
    gotoxy(0, 14);
    cputs("1=yes 0=no");

    while (1) {
        char c = cgetc();
        if (c == '1') return 1;
        if (c == '0') return 0;
    }
}

static void wait_any_key_msg(const char *msg)
{
    set_status(msg);
    clear_line(13);
    gotoxy(0, 13);
    cputs(msg);
    clear_line(14);
    gotoxy(0, 14);
    cputs("Press any key...");
    (void)cgetc();
}

static unsigned char show_help(void)
{
    g_help_scroll_offset = 0;
    clrscr();
    help_render();

    while (1) {
        char key = cgetc();
        if (key == ' ' ) {
            return 0;
        } else if (key == 'w' || key == 'W' || key == 'i' || key == 'I') {
            help_scroll(-1);
            help_render();
        } else if (key == 's' || key == 'S' || key == 'k' || key == 'K') {
            help_scroll(1);
            help_render();
        } else if (key == 'q' || key == 'Q') {
            if (prompt_quit()) return 1;
            help_render();
        }
    }
}

static const DoorDef *get_door_at(const RoomDef *room, unsigned char x, unsigned char y, unsigned char *idx)
{
    unsigned char i;
    for (i = 0; i < room->door_count; i++) {
        const DoorDef *d = &room->doors[i];
        if (d->orientation == ORIENT_HORIZONTAL) {
            if ((d->x == x || (unsigned char)(d->x + 1) == x) && d->y == y) {
                *idx = i;
                return d;
            }
        } else {
            if (d->x == x && (d->y == y || (unsigned char)(d->y + 1) == y)) {
                *idx = i;
                return d;
            }
        }
    }
    return 0;
}

static const StairDef *get_stair_at(const RoomDef *room, unsigned char x, unsigned char y, unsigned char *idx)
{
    unsigned char i;
    for (i = 0; i < room->stair_count; i++) {
        const StairDef *s = &room->stairs[i];
        if (s->x == x && s->y == y) {
            *idx = i;
            return s;
        }
    }
    return 0;
}

static const BoxDef *get_box_at(const RoomDef *room, unsigned char x, unsigned char y, unsigned char *idx)
{
    unsigned char i;
    for (i = 0; i < room->box_count; i++) {
        const BoxDef *b = &room->boxes[i];
        if (b->orientation == ORIENT_HORIZONTAL) {
            if (b->y == y && (b->x == x || (unsigned char)(b->x + 1) == x)) {
                *idx = i;
                return b;
            }
        } else {
            if (b->x == x && (b->y == y || (unsigned char)(b->y + 1) == y)) {
                *idx = i;
                return b;
            }
        }
    }
    return 0;
}

static MoveResult try_move(const GameState *st, signed char dx, signed char dy,
                           unsigned char *tx, unsigned char *ty)
{
    const RoomDef *room = &g_rooms[st->room];
    int nx = (int)st->x + dx;
    int ny = (int)st->y + dy;
    TileCode tile;

    if (nx < 0 || ny < 0 || nx >= ROOM_W || ny >= ROOM_H) return MOVE_BLOCKED;

    *tx = (unsigned char)nx;
    *ty = (unsigned char)ny;
    tile = room_tile_code(room, *tx, *ty);

    if (tile == TILE_WALL) return MOVE_BLOCKED;
    if (tile == TILE_DOOR) return MOVE_DOOR;
    if (tile == TILE_STAIR_DOWN || tile == TILE_STAIR_UP) return MOVE_STAIR;
    if (tile == TILE_BOX) return MOVE_BOX;

    return MOVE_OK;
}

static unsigned char do_door_transition(GameState *st, const DoorDef *door)
{
    int sx, sy;
    int dx = 0, dy = 0;
    unsigned char tr = door->target_room;
    unsigned char td = door->target_door;

    if (tr >= ROOM_COUNT || td >= g_rooms[tr].door_count) return 0;

    {
        const DoorDef *target = &g_rooms[tr].doors[td];
        if (target->wall_side == SIDE_NORTH) {
            sx = target->x;
            sy = target->y + 1;
            dy = 1;
        } else if (target->wall_side == SIDE_SOUTH) {
            sx = target->x;
            sy = target->y - 1;
            dy = -1;
        } else if (target->wall_side == SIDE_WEST) {
            sx = target->x + 1;
            sy = target->y;
            dx = 1;
        } else {
            sx = target->x - 1;
            sy = target->y;
            dx = -1;
        }
    }

    while (sx >= 0 && sy >= 0 && sx < ROOM_W && sy < ROOM_H) {
        if (room_tile_code(&g_rooms[tr], (unsigned char)sx, (unsigned char)sy) == TILE_FLOOR) {
            st->room = tr;
            st->x = (unsigned char)sx;
            st->y = (unsigned char)sy;
            return 1;
        }
        sx += dx;
        sy += dy;
    }

    return 0;
}

static unsigned char do_stair_transition(GameState *st, const StairDef *stair)
{
    unsigned char tr = stair->target_room;
    unsigned char ts = stair->target_stair;

    if (tr >= ROOM_COUNT || ts >= g_rooms[tr].stair_count) return 0;

    st->room = tr;
    st->x = g_rooms[tr].stairs[ts].x;
    st->y = g_rooms[tr].stairs[ts].y;
    return 1;
}

static void box_interaction(const BoxDef *box)
{
    char items[MSG_BUF];
    unsigned char p = 0;
    unsigned char i;

    wait_any_key_msg(box->effect);

    items[p++] = 'I';
    items[p++] = 't';
    items[p++] = 'e';
    items[p++] = 'm';
    items[p++] = 's';
    items[p++] = ':';
    items[p++] = ' ';

    for (i = 0; i < box->item_count; i++) {
        const char *name = box->items[i];
        unsigned char j;
        for (j = 0; name[j] != '\0' && p < (MSG_BUF - 2); j++) {
            items[p++] = name[j];
        }
        if (i + 1 < box->item_count && p < (MSG_BUF - 2)) {
            items[p++] = ',';
            items[p++] = ' ';
        }
    }
    items[p] = '\0';
    wait_any_key_msg(items);
}

static unsigned char prompt_quit(void)
{
    set_status("Quit? 1=yes 0=no");
    clear_line(13);
    gotoxy(0, 13);
    cputs("Quit? 1=yes 0=no");
    clear_line(14);
    gotoxy(0, 14);
    cputs("1=yes 0=no");
    while (1) {
        char c = cgetc();
        if (c == '1') return 1;
        if (c == '0') return 0;
    }
}

void game_run(void)
{
    GameState st;
    unsigned char running = 1;
    unsigned char move_counter = 0;

    init_runtime();

    st.room = 0;
    st.x = g_rooms[0].player_start_x;
    st.y = g_rooms[0].player_start_y;
    st.cam_x = 0;
    st.cam_y = 0;

    set_status("Ready.");
    update_camera(&st);
    render_full(&st);

    while (running) {
        GameState prev;
        unsigned char map_dirty = 0;
        unsigned char hud_dirty = 0;
        unsigned char full_dirty = 0;
        unsigned char tx = 0, ty = 0;
        signed char dx = 0, dy = 0;
        MoveResult mr;
        char key;

        prev = st;
        key = cgetc();
        if (key == 'q' || key == 'Q') {
            if (prompt_quit()) running = 0;
            else {
                set_status("Continue.");
                hud_dirty = 1;
            }
        }
        else if (key == 'h' || key == 'H') {
            if (show_help()) running = 0;
            else {
                set_status("Ready.");
                full_dirty = 1;
            }
        } else {
            if (key == 'w' || key == 'W' || key == 'i' || key == 'I') dy = -1;
            else if (key == 's' || key == 'S' || key == 'k' || key == 'K') dy = 1;
            else if (key == 'a' || key == 'A' || key == 'j' || key == 'J') dx = -1;
            else if (key == 'd' || key == 'D' || key == 'l' || key == 'L') dx = 1;
            else {
                set_status("Unknown key.");
                hud_dirty = 1;
            }

            if (dx != 0 || dy != 0) {
                mr = try_move(&st, dx, dy, &tx, &ty);
                if (mr == MOVE_OK) {
                    int hit;
                    unsigned char collided = 0;

                    st.x = tx;
                    st.y = ty;
                    set_status("Moved.");
                    map_dirty = 1;

                    hit = monster_check_collision_and_mark(&st);
                    if (hit >= 0) {
                        set_encounter_status(st.room, (unsigned char)hit);
                        collided = 1;
                    }

                    move_counter++;
                    if (move_counter >= 2) {
                        move_counter = 0;
                        monster_update_all(st.room, st.x, st.y);
                        if (!collided) {
                            hit = monster_check_collision_and_mark(&st);
                            if (hit >= 0)
                                set_encounter_status(st.room, (unsigned char)hit);
                        }
                    }
                } else if (mr == MOVE_BLOCKED) {
                    set_status("Blocked.");
                } else if (mr == MOVE_DOOR) {
                    unsigned char idx = 0;
                    const DoorDef *door = get_door_at(&g_rooms[st.room], tx, ty, &idx);
                    (void)idx;
                    if (!door) {
                        set_status("Door not found.");
                    } else if (prompt_yes_no("Enter room? 1/0")) {
                        if (do_door_transition(&st, door)) {
                            set_status("Room changed.");
                            move_counter = 0;
                            map_dirty = 1;
                        } else {
                            set_status("No connected room.");
                        }
                    } else {
                        set_status("Canceled.");
                    }
                } else if (mr == MOVE_STAIR) {
                    unsigned char idx = 0;
                    const StairDef *stair = get_stair_at(&g_rooms[st.room], tx, ty, &idx);
                    (void)idx;
                    if (!stair) {
                        set_status("Stair not found.");
                    } else if (prompt_yes_no("Use stairs? 1/0")) {
                        if (do_stair_transition(&st, stair)) {
                            set_status("Floor changed.");
                            move_counter = 0;
                            map_dirty = 1;
                        } else {
                            set_status("No connected stair.");
                        }
                    } else {
                        set_status("Canceled.");
                    }
                } else if (mr == MOVE_BOX) {
                    unsigned char idx = 0;
                    const BoxDef *box = get_box_at(&g_rooms[st.room], tx, ty, &idx);
                    (void)idx;
                    if (!box) {
                        set_status("Box not found.");
                    } else if (prompt_yes_no("Open box? 1/0")) {
                        box_interaction(box);
                        set_status("Box closed.");
                    } else {
                        set_status("Canceled.");
                    }
                }
                hud_dirty = 1;
            }
        }

        if (!running) break;

        update_camera(&st);

        if (full_dirty) {
            render_full(&st);
            continue;
        }

        if (map_dirty) {
            if (prev.room != st.room || prev.cam_x != st.cam_x || prev.cam_y != st.cam_y ||
                g_rooms[st.room].monster_count > 0) {
                render_map(&st);
            } else {
                render_player_step(&prev, &st);
            }
        }
        if (hud_dirty) {
            render_hud(&st);
        }
    }

    clrscr();
    gotoxy(0, 0);
    cputs("Bye.");
}
