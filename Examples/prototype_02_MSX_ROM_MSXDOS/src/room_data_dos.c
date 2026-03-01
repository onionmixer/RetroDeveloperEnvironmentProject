#include "room_data.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* Runtime grid buffer — loaded from disk per room */
char g_grid_buffer[ROOM_H * ROOM_W];  /* 720 bytes BSS */
unsigned char g_loaded_room_grid = 0xFF;

void grid_load_room(unsigned char room)
{
    char fname[7];  /* "ROOM00\0" */
    int fd, n;

    if (room == g_loaded_room_grid) return;

    /* Build filename: ROOM00, ROOM01, ... */
    fname[0] = 'R'; fname[1] = 'O'; fname[2] = 'O'; fname[3] = 'M';
    fname[4] = '0' + (room / 10);
    fname[5] = '0' + (room % 10);
    fname[6] = '\0';

    fd = open(fname, O_RDONLY, 0);
    if (fd >= 0) {
        n = read(fd, g_grid_buffer, ROOM_W * ROOM_H);
        close(fd);
        if (n == ROOM_W * ROOM_H) {
            g_loaded_room_grid = room;
            return;
        }
    }

    /* Error fallback: fill with walls */
    memset(g_grid_buffer, '#', ROOM_W * ROOM_H);
    g_loaded_room_grid = room;
}

/* --- All metadata below is identical to room_data.c --- */

const char g_room_names[ROOM_COUNT][24] = {
    "Abandoned Hall",
    "Crystal Cavern",
    "Shadow Labyrinth",
};

const unsigned char g_room_tileset_id[ROOM_COUNT] = {0, 0, 0};

const unsigned char g_room_z[ROOM_COUNT] = {
    0, 0, 1
};
const unsigned char g_player_start_x[ROOM_COUNT] = {
    5, 14, 14
};
const unsigned char g_player_start_y[ROOM_COUNT] = {
    11, 11, 11
};

/* Doors: Room 0 east wall <-> Room 1 west wall */
const unsigned char g_door_count[ROOM_COUNT] = { 1, 1, 0 };
const DoorMsx g_doors[ROOM_COUNT][MAX_DOORS] = {
    {
        { 29, 9, 1, 3, 1, 0 },  /* Room 0: east wall door -> Room 1 door 0 */
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 9, 1, 2, 0, 0 },   /* Room 1: west wall door -> Room 0 door 0 */
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
    },
    {
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 0 },
    },
};

/* Stairs: Room 0 down <-> Room 2 up, Room 1 down <-> Room 2 up */
const unsigned char g_stair_count[ROOM_COUNT] = { 1, 1, 1 };
const StairMsx g_stairs[ROOM_COUNT][MAX_STAIRS] = {
    {
        { 5, 11, 0, 2, 0 },     /* Room 0: stair down at (5,11) -> Room 2 stair 0 */
        { 0, 0, 0, 0, 0 },
    },
    {
        { 12, 11, 0, 2, 0 },    /* Room 1: stair down at (12,11) -> Room 2 stair 0 */
        { 0, 0, 0, 0, 0 },
    },
    {
        { 7, 10, 1, 0, 0 },     /* Room 2: stair up at (7,10) -> Room 0 stair 0 */
        { 0, 0, 0, 0, 0 },
    },
};

/* Boxes */
const unsigned char g_box_count[ROOM_COUNT] = { 3, 3, 1 };
const BoxMsx g_boxes[ROOM_COUNT][MAX_BOXES] = {
    {
        { 8, 8, 0, 0, 1, 2, { 1, 4, 0, 0, 0 } },
        { 13, 11, 0, 0, 3, 1, { 6, 0, 0, 0, 0 } },
        { 13, 19, 0, 1, 2, 3, { 7, 3, 5, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
    },
    {
        { 9, 8, 0, 0, 1, 2, { 2, 9, 0, 0, 0 } },
        { 18, 8, 0, 0, 4, 1, { 8, 0, 0, 0, 0 } },
        { 13, 19, 0, 1, 3, 2, { 10, 5, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
    },
    {
        { 15, 10, 0, 0, 2, 2, { 3, 1, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },
    },
};

const char * const g_placed_by[] = {
    "default",
    "Goblin Trickster",
};
const unsigned char g_placed_by_count = 2;

const char * const g_box_effects[] = {
    "",
    "Dust swirls. Nothing unusual.",
    "Sparks fly from the lid!",
    "Musty smell. Seems safe.",
    "A rune glows and sears you!",
};
const unsigned char g_box_effect_count = 5;

/* Monsters */
const unsigned char g_monster_count[ROOM_COUNT] = { 3, 2, 3 };
const MonsterDef g_monsters[ROOM_COUNT][MAX_MONSTERS] = {
    /* Room 0: Abandoned Hall */
    {
        { 20,  4, 0, 5 },  /* Cave Spider */
        { 10, 17, 1, 4 },  /* Dark Slime */
        { 24, 18, 3, 6 },  /* Goblin Scout */
        {  0,  0, 0, 0 },
    },
    /* Room 1: Crystal Cavern */
    {
        { 20,  5, 5, 5 },  /* Stone Golem */
        { 10, 17, 6, 6 },  /* Bat Swarm */
        {  0,  0, 0, 0 },
        {  0,  0, 0, 0 },
    },
    /* Room 2: Shadow Labyrinth */
    {
        { 20,  4, 2, 4 },  /* Shadow Rat */
        {  5, 16, 4, 5 },  /* Skeleton */
        { 25, 16, 7, 4 },  /* Fire Imp */
        {  0,  0, 0, 0 },
    },
};

const char * const g_monster_names[] = {
    "Cave Spider",
    "Dark Slime",
    "Shadow Rat",
    "Goblin Scout",
    "Skeleton",
    "Stone Golem",
    "Bat Swarm",
    "Fire Imp",
};
const unsigned char g_monster_name_count = 8;

const char * const g_item_names[] = {
    "",
    "Gold Coin",
    "Mana Crystal",
    "Poison Vial",
    "Torch",
    "Iron Sword",
    "Health Potion",
    "Cursed Amulet",
    "Leather Shield",
    "Speed Boots",
    "Ancient Scroll",
};
const unsigned char g_item_count = 11;
