#ifndef LOGIC_H
#define LOGIC_H

#include "engine.h"

void logic_init(GameState *g);
void logic_update_camera(GameState *g);
char logic_get_tile(unsigned char room, unsigned char x, unsigned char y);

MoveResult logic_try_move(GameState *g, signed char dx, signed char dy);
int logic_find_door_at(unsigned char room, unsigned char x, unsigned char y);
int logic_find_stair_at(unsigned char room, unsigned char x, unsigned char y);
int logic_find_box_at(unsigned char room, unsigned char x, unsigned char y);

int logic_door_transition(GameState *g, int door_idx);
int logic_stair_transition(GameState *g, int stair_idx);

#endif
