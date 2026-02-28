#ifndef MONSTER_H
#define MONSTER_H

#include "room_data.h"

#define MAX_MONSTERS 4

#define MON_IDLE       0
#define MON_CHASE      1
#define MON_RETURNING  2

typedef struct {
    unsigned char home_x;
    unsigned char home_y;
    unsigned char name_id;   /* index into g_monster_names[] */
    unsigned char range;     /* Manhattan detection distance */
} MonsterDef;

typedef struct {
    unsigned char x, y;
    unsigned char state;
    unsigned char detect_enabled;
} MonsterRT;

extern const unsigned char g_monster_count[ROOM_COUNT];
extern const MonsterDef g_monsters[ROOM_COUNT][MAX_MONSTERS];
extern const char * const g_monster_names[];
extern const unsigned char g_monster_name_count;

extern MonsterRT g_monsters_rt[MAX_MONSTERS];

void monster_init_room(unsigned char room);
void monster_update_all(unsigned char room,
                        unsigned char px, unsigned char py);
unsigned char monster_check_collision(unsigned char room,
                                      unsigned char px, unsigned char py);

#endif
