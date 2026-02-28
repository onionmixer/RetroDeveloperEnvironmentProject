#ifndef MONSTER_H
#define MONSTER_H
#include "engine.h"

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

extern RoomRuntime g_runtime[ROOM_COUNT];

void monster_init_all(void);

void monster_update_all(unsigned char room,
                        unsigned char px, unsigned char py);

int monster_check_collision(const GameState *st);

void monster_format_encounter_msg(char *buf, unsigned char buf_size,
                                  unsigned char room, unsigned char idx);

int monster_index_at(unsigned char room,
                     unsigned char x, unsigned char y);

#endif
