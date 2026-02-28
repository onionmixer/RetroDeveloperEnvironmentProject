#ifndef LOGIC_H
#define LOGIC_H
#include "engine.h"

void logic_init(GameState *g);
void logic_update_camera(GameState *g);

void logic_decompress_room(unsigned char room);
TileCode logic_get_tile_code(unsigned char room,
                             unsigned char x, unsigned char y);

MoveResult logic_try_move(const GameState *g, signed char dx, signed char dy,
                          unsigned char *tx, unsigned char *ty);

const DoorDef *logic_find_door(unsigned char room,
                               unsigned char x, unsigned char y);
const StairDef *logic_find_stair(unsigned char room,
                                 unsigned char x, unsigned char y);
const BoxDef *logic_find_box(unsigned char room,
                             unsigned char x, unsigned char y);

unsigned char logic_do_door(GameState *g, const DoorDef *door);
unsigned char logic_do_stair(GameState *g, const StairDef *stair);

#endif
