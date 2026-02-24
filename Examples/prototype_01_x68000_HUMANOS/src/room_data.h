#ifndef ROOM_DATA_H
#define ROOM_DATA_H

#define ROOM_COUNT 3
#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 20
#define VIEW_H 20

#define MAX_DOORS 4
#define MAX_STAIRS 2
#define MAX_BOXES 10
#define MAX_ITEMS_PER_BOX 5
#define MAX_MONSTERS 10

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char orientation; /* 0=horizontal,1=vertical */
    unsigned char wall_side;   /* 0=north,1=south,2=west,3=east */
    unsigned char target_room;
    unsigned char target_index;
} DoorX68;

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char type;        /* 0=down,1=up */
    unsigned char target_room;
    unsigned char target_index;
} StairX68;

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char orientation; /* 0=horizontal,1=vertical */
    unsigned char placed_by_id;
    unsigned char effect_id;
    unsigned char item_count;
    unsigned char item_ids[MAX_ITEMS_PER_BOX];
} BoxX68;

typedef struct {
    unsigned char x;
    unsigned char y;
    unsigned char name_id;
    unsigned char range;
} MonsterX68;

extern const char g_room_grids[ROOM_COUNT][ROOM_H][ROOM_W + 1];
extern const char g_room_names[ROOM_COUNT][24];
extern const unsigned char g_room_z[ROOM_COUNT];
extern const unsigned char g_player_start_x[ROOM_COUNT];
extern const unsigned char g_player_start_y[ROOM_COUNT];

extern const unsigned char g_door_count[ROOM_COUNT];
extern const DoorX68 g_doors[ROOM_COUNT][MAX_DOORS];

extern const unsigned char g_stair_count[ROOM_COUNT];
extern const StairX68 g_stairs[ROOM_COUNT][MAX_STAIRS];

extern const unsigned char g_box_count[ROOM_COUNT];
extern const BoxX68 g_boxes[ROOM_COUNT][MAX_BOXES];

extern const unsigned char g_monster_count[ROOM_COUNT];
extern const MonsterX68 g_monsters[ROOM_COUNT][MAX_MONSTERS];
extern const char * const g_monster_names[];
extern const unsigned char g_monster_name_count;

extern const char * const g_placed_by[];
extern const unsigned char g_placed_by_count;
extern const char * const g_box_effects[];
extern const unsigned char g_box_effect_count;
extern const char * const g_item_names[];
extern const unsigned char g_item_count;

#endif
