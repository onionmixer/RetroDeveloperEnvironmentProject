#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include "engine.h"

#define MAP_ORIGIN_X    1
#define MAP_ORIGIN_Y    2
#define MAP_VIEW_W     10
#define MAP_VIEW_H     10

void render_init(void);
void render_draw_map(const GameState *gs);
void render_draw_border(void);
void render_update_player(const GameState *gs);
void render_draw_monsters(const GameState *gs);
void render_print(uint8_t x, uint8_t y, const char *text);
void render_print_n(uint8_t x, uint8_t y, const char *text, uint8_t maxlen);
void render_clear_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void render_update_status(const GameState *gs);
void render_set_status(const char *msg);
void render_redraw_all(const GameState *gs);
void render_cleanup(void);
#ifdef MSXDOS
void render_load_tileset(unsigned char tileset_id);
#endif

#endif
