#ifndef MONSTER_H
#define MONSTER_H

#include "room_data.h"

#define MON_IDLE       0
#define MON_CHASE      1
#define MON_RETURNING  2

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char state;
    unsigned char detect_enabled;
} MonsterRT;

extern MonsterRT g_monsters_rt[MAX_MONSTERS];

void monster_init_room(unsigned char room);
void monster_update_all(unsigned char room,
                        unsigned char px, unsigned char py);
unsigned char monster_check_collision(unsigned char room,
                                      unsigned char px, unsigned char py);

#endif
