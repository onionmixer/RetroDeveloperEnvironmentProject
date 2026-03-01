#include "render.h"
#include "tiles.h"
#include "font.h"
#include "room_data.h"
#include "monster.h"
#include <ubox.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/*
 * Combined tileset: 256 tiles * 8 bytes = 2048 bytes each.
 * Tiles 0-31 from tiles.h, 32-127 from font.h, 128-255 zero.
 */
static uint8_t tileset_patterns[2048] = {
    TILE_PATTERNS_DATA,
    FONT_PATTERNS_DATA,
    /* [128-255] 128 empty tiles = 1024 bytes of zeros */
    0
};

static uint8_t tileset_colors[2048] = {
    TILE_COLORS_DATA,
    FONT_COLORS_DATA,
    /* [128-255] 128 empty tiles = 1024 bytes of zeros */
    0
};

static const uint8_t player_sprite[8] = {
    0x18, /* ...##... */
    0x18, /* ...##... */
    0x3C, /* ..####.. */
    0x5A, /* .#.##.#. */
    0x18, /* ...##... */
    0x3C, /* ..####.. */
    0x24, /* ..#..#.. */
    0x24  /* ..#..#.. */
};

static char status_buf[40];
static uint8_t status_dirty;
static unsigned char g_loaded_tileset = 0xFF;

/* Helper: get char from flat grid buffer with 16-bit offset */
static char grid_buf_get(uint8_t gx, uint8_t gy)
{
    return g_grid_buffer[(unsigned int)gy * ROOM_W + gx];
}

static uint8_t char_to_map_tile(uint8_t gx, uint8_t gy)
{
    char c = grid_buf_get(gx, gy);
    switch (c) {
        case '.': return TILE_FLOOR;
        case '#': return TILE_WALL;
        case '@':
            if ((gx > 0 && grid_buf_get(gx - 1, gy) == '@') ||
                (gx < ROOM_W - 1 && grid_buf_get(gx + 1, gy) == '@'))
                return TILE_DOOR_H;
            return TILE_DOOR_V;
        case '<': return TILE_STAIR_DN;
        case '>': return TILE_STAIR_UP;
        case '%': return TILE_BOX;
        default:  return TILE_EMPTY;
    }
}

void render_init(void)
{
    ubox_set_mode(2);
    ubox_set_colors(15, 1, 1);
    ubox_wvdp(1, 0xE0);
    ubox_disable_screen();
    ubox_set_tiles(tileset_patterns);
    ubox_set_tiles_colors(tileset_colors);
    ubox_fill_screen(TILE_EMPTY);
    ubox_set_sprite_pat8(player_sprite, 0);
    render_draw_border();
    /* DOS: skip ubox_init_isr() — ISR code in page 0 is unreachable
       when HTIMI hook is called during BIOS inter-slot context.
       ubox_wait() is overridden below with halt-based polling. */

    status_buf[0] = '\0';
    status_dirty = 1;
}

/*
 * DOS-compatible replacement for ubox_wait().
 * The library version relies on an ISR hook at HTIMI ($FD9F) which
 * fails under MSX-DOS2 because the hook is called with BIOS ROM
 * paged in on page 0, making user code at $0100+ inaccessible.
 * This version simply waits for 2 VDP interrupts using halt.
 */
void ubox_wait(void)
{
    __asm
    halt
    halt
    __endasm;
}

void render_draw_border(void)
{
    uint8_t i;

    ubox_put_tile(0, 1, TILE_BORDER_TL);
    ubox_put_tile(11, 1, TILE_BORDER_TR);
    ubox_put_tile(0, 12, TILE_BORDER_BL);
    ubox_put_tile(11, 12, TILE_BORDER_BR);

    for (i = 1; i <= 10; i++) {
        ubox_put_tile(i, 1, TILE_BORDER_H);
        ubox_put_tile(i, 12, TILE_BORDER_HB);
    }

    for (i = 2; i <= 11; i++) {
        ubox_put_tile(0, i, TILE_BORDER_V);
        ubox_put_tile(11, i, TILE_BORDER_VR);
    }
}

void render_draw_map(const GameState *gs)
{
    uint8_t vx, vy;
    uint8_t gx, gy;

    grid_load_room(gs->room);  /* cache hit returns immediately */

    for (vy = 0; vy < MAP_VIEW_H; vy++) {
        gy = gs->cam_y + vy;
        for (vx = 0; vx < MAP_VIEW_W; vx++) {
            gx = gs->cam_x + vx;
            ubox_put_tile(MAP_ORIGIN_X + vx, MAP_ORIGIN_Y + vy,
                          char_to_map_tile(gx, gy));
        }
    }
}

void render_draw_monsters(const GameState *gs)
{
    unsigned char i, cnt;
    unsigned char mx, my;
    int vx, vy;

    cnt = g_monster_count[gs->room];
    for (i = 0; i < cnt; i++) {
        mx = g_monsters_rt[i].x;
        my = g_monsters_rt[i].y;
        vx = (int)mx - (int)gs->cam_x;
        vy = (int)my - (int)gs->cam_y;
        if (vx >= 0 && vx < MAP_VIEW_W && vy >= 0 && vy < MAP_VIEW_H) {
            ubox_put_tile(MAP_ORIGIN_X + (uint8_t)vx,
                          MAP_ORIGIN_Y + (uint8_t)vy,
                          TILE_MONSTER);
        }
    }
}

void render_update_player(const GameState *gs)
{
    struct sprite_attr attr;
    attr.x = (gs->x - gs->cam_x) * 8 + MAP_ORIGIN_X * 8;
    attr.y = (gs->y - gs->cam_y) * 8 + MAP_ORIGIN_Y * 8 - 1;
    attr.pattern = 0;
    attr.attr = 8; /* color 8 = red */
    ubox_set_sprite_attr(&attr, 0);
}

void render_print(uint8_t x, uint8_t y, const char *text)
{
    while (*text) {
        ubox_put_tile(x, y, (uint8_t)*text);
        x++;
        text++;
    }
}

void render_print_n(uint8_t x, uint8_t y, const char *text, uint8_t maxlen)
{
    while (*text && maxlen > 0) {
        ubox_put_tile(x, y, (uint8_t)*text);
        x++;
        text++;
        maxlen--;
    }
}

void render_print_wrap(uint8_t x, uint8_t y, const char *text,
                       uint8_t w, uint8_t max_rows)
{
    uint8_t col, row, wlen;
    const char *p, *word;

    col = 0;
    row = 0;
    p = text;

    while (*p && row < max_rows) {
        /* skip leading spaces on a new line */
        if (col == 0) {
            while (*p == ' ') p++;
        }
        if (!*p) break;

        /* measure next word length */
        word = p;
        wlen = 0;
        while (p[wlen] && p[wlen] != ' ') wlen++;

        /* if word doesn't fit on current line, wrap */
        if (col > 0 && col + wlen > w) {
            row++;
            col = 0;
            continue;
        }

        /* if single word is longer than width, truncate it */
        if (wlen > w) wlen = w;

        /* print the word */
        while (wlen > 0 && row < max_rows) {
            ubox_put_tile(x + col, y + row, (uint8_t)*p);
            p++;
            col++;
            wlen--;
        }

        /* skip trailing spaces, add one space between words */
        while (*p == ' ') p++;
        if (*p && col < w) {
            col++;  /* visual space between words */
        }
    }
}

void render_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t ix, iy;
    for (iy = 0; iy < h; iy++) {
        for (ix = 0; ix < w; ix++) {
            ubox_put_tile(x + ix, y + iy, TILE_EMPTY);
        }
    }
}

void render_set_status(const char *msg)
{
    uint8_t i;
    for (i = 0; i < 39 && msg[i]; i++)
        status_buf[i] = msg[i];
    status_buf[i] = '\0';
    status_dirty = 1;
}

void render_update_status(const GameState *gs)
{
    char buf[20];

    sprintf(buf, "X:%3d  Y:%3d", gs->x, gs->y);
    render_print(13, 2, buf);
    sprintf(buf, "Z:%d", g_room_z[gs->room]);
    render_print(13, 3, buf);

    render_print_n(0, 0, g_room_names[gs->room], 32);

    if (status_dirty) {
        render_clear_area(13, 6, 19, 5);
        if (status_buf[0])
            render_print_wrap(13, 6, status_buf, 19, 5);
        status_dirty = 0;
    }
}

void render_redraw_all(const GameState *gs)
{
    ubox_disable_screen();
    ubox_fill_screen(TILE_EMPTY);
    render_draw_border();
    render_draw_map(gs);
    render_draw_monsters(gs);
    render_update_player(gs);
    render_update_status(gs);
    render_print(0, 23, "WASD:move H:help Q:quit");
    ubox_enable_screen();
}

void render_load_tileset(unsigned char tileset_id)
{
    unsigned char buf[144];
    char fname[7];  /* "TILES0\0" */
    int fd;

    if (tileset_id == g_loaded_tileset) return;

    fname[0] = 'T'; fname[1] = 'I'; fname[2] = 'L';
    fname[3] = 'E'; fname[4] = 'S';
    fname[5] = '0' + tileset_id;
    fname[6] = '\0';

    fd = open(fname, O_RDONLY, 0);
    if (fd >= 0) {
        if (read(fd, buf, 144) == 144) {
            memcpy(&tileset_patterns[0], &buf[0], 72);   /* patterns tiles 0-8 */
            memcpy(&tileset_colors[0], &buf[72], 72);     /* colors tiles 0-8 */
            g_loaded_tileset = tileset_id;
        }
        close(fd);
    }

    /* VRAM re-upload — render_redraw_all() does NOT call ubox_set_tiles() */
    ubox_set_tiles(tileset_patterns);
    ubox_set_tiles_colors(tileset_colors);
}

void render_cleanup(void)
{
    struct sprite_attr hide;
    hide.y = 208;
    hide.x = 0;
    hide.pattern = 0;
    hide.attr = 0;
    ubox_set_sprite_attr(&hide, 0);
    /* Restore SCREEN 0 (text mode) for DOS return */
    ubox_set_mode(0);
}
