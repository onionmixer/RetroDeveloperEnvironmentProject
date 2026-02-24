#ifndef ENGINE_H
#define ENGINE_H

#include "room_data.h"

typedef enum {
    MOVE_OK = 0,
    MOVE_BLOCKED,
    MOVE_BLOCKED_BOX,
    MOVE_DOOR_PENDING,
    MOVE_STAIR_PENDING
} MoveResult;

typedef struct {
    unsigned char room;
    unsigned char x;
    unsigned char y;
    unsigned char cam_x;
    unsigned char cam_y;
    char status[80];
    unsigned char running;
} GameState;

#endif
