#ifndef RENDER_H
#define RENDER_H

#include "engine.h"

void render_set_status(GameState *g, const char *text);
void render_draw(const GameState *g);
int render_prompt_yes_no(GameState *g, const char *text);
void render_wait_any_key(GameState *g, const char *text);
int render_read_key(void);
int render_show_help(void);

#endif
