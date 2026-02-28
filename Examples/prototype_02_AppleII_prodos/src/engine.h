#ifndef ENGINE_H_INCLUDED
#define ENGINE_H_INCLUDED

#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 10
#define VIEW_H 10
#define ROOM_COUNT 3
#define MAX_DOORS 4
#define MAX_STAIRS 2
#define MAX_BOXES 10
#define MAX_MONSTERS 10
#define MAX_ITEMS_PER_BOX 5
#define MSG_BUF 64

#define INVALID_INDEX 255

/* Viewport tile coordinates */
#define MAP_ORIGIN_X 1
#define MAP_ORIGIN_Y 2
#define MAP_VIEW_W  10
#define MAP_VIEW_H  10

/* Status area */
#define STATUS_X    13
#define STATUS_Y     6
#define STATUS_W    27
#define STATUS_H     5

/* Message area */
#define MSG_AREA_X   0
#define MSG_AREA_Y  14
#define MSG_AREA_W  40
#define MSG_AREA_H   8

typedef enum {
    ORIENT_HORIZONTAL = 0,
    ORIENT_VERTICAL = 1
} Orientation;

typedef enum {
    SIDE_NORTH = 0,
    SIDE_SOUTH = 1,
    SIDE_WEST = 2,
    SIDE_EAST = 3
} WallSide;

typedef enum {
    STAIR_DOWN = 0,
    STAIR_UP = 1
} StairType;

typedef enum {
    MONSTER_PATROL = 0,
    MONSTER_CHASE,
    MONSTER_RETURNING
} MonsterState;

typedef enum {
    MOVE_OK = 0,
    MOVE_BLOCKED,
    MOVE_DOOR,
    MOVE_STAIR,
    MOVE_BOX
} MoveResult;

typedef enum {
    TILE_FLOOR = 0,
    TILE_WALL = 1,
    TILE_DOOR = 2,
    TILE_STAIR_DOWN = 3,
    TILE_STAIR_UP = 4,
    TILE_BOX = 5
} TileCode;

typedef struct {
    unsigned char x;
    unsigned char y;
    Orientation orientation;
    WallSide wall_side;
    unsigned char target_room;
    unsigned char target_door;
} DoorDef;

typedef struct {
    unsigned char x;
    unsigned char y;
    StairType type;
    unsigned char target_room;
    unsigned char target_stair;
} StairDef;

typedef struct {
    unsigned char x;
    unsigned char y;
    Orientation orientation;
    const char *placed_by;
    const char *effect;
    unsigned char item_count;
    const char *items[MAX_ITEMS_PER_BOX];
} BoxDef;

typedef struct {
    unsigned char home_x;
    unsigned char home_y;
    unsigned char range;
    const char *name;
} MonsterDef;

typedef struct {
    const char *id;
    const char *name;
    signed char z_level;
    const unsigned char *grid_rle;
    unsigned int grid_rle_size;

    unsigned char door_count;
    DoorDef doors[MAX_DOORS];

    unsigned char stair_count;
    StairDef stairs[MAX_STAIRS];

    unsigned char box_count;
    BoxDef boxes[MAX_BOXES];

    unsigned char monster_count;
    MonsterDef monsters[MAX_MONSTERS];

    unsigned char player_start_x;
    unsigned char player_start_y;
} RoomDef;

typedef struct {
    unsigned char room;
    unsigned char x;
    unsigned char y;
    unsigned char cam_x;
    unsigned char cam_y;
    unsigned char running;
} GameState;

#endif
