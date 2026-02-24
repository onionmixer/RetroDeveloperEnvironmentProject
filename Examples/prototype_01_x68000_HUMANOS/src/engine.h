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

typedef enum {
    MONSTER_PATROL = 0,
    MONSTER_CHASE = 1,
    MONSTER_RETURNING = 2
} MonsterState;

typedef struct {
    unsigned char room;
    unsigned char x;
    unsigned char y;
    unsigned char cam_x;
    unsigned char cam_y;
    unsigned char monster_tick;
    unsigned char monster_move_enabled;
    char status[128];
    char status_prev[128];
    unsigned char running;
} GameState;

#endif
