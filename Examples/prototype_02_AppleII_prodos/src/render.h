#ifndef RENDER_H
#define RENDER_H
#include "engine.h"

void render_load_tileset(unsigned char tileset_id);
void render_init(void);
void render_draw_map(const GameState *gs);
void render_draw_border(void);
void render_update_player(const GameState *gs);
void render_draw_monsters(const GameState *gs);
void render_print(unsigned char x, unsigned char y, const char *text);
void render_print_n(unsigned char x, unsigned char y, const char *text,
                    unsigned char maxlen);
void render_print_wrap(unsigned char x, unsigned char y, const char *text,
                       unsigned char w, unsigned char max_rows);
void render_clear_area(unsigned char x, unsigned char y,
                       unsigned char w, unsigned char h);
void render_update_status(const GameState *gs);
void render_set_status(const char *msg);
void render_redraw_all(const GameState *gs);
void render_cleanup(void);
void render_clear_screen(void);

unsigned char render_prompt_yes_no(const char *msg);
void render_wait_any_key(const char *msg);

#endif
