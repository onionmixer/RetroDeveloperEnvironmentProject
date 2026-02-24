#include <assert.h>
#include <stdio.h>

#define ROOM_W 100
#define ROOM_H 100
#define VIEW_W 20
#define VIEW_H 20

static unsigned char clamp_u8(int v, int lo, int hi)
{
    if (v < lo) return (unsigned char)lo;
    if (v > hi) return (unsigned char)hi;
    return (unsigned char)v;
}

static void camera(unsigned char px, unsigned char py, unsigned char *cx, unsigned char *cy)
{
    int x = (int)px - (VIEW_W / 2);
    int y = (int)py - (VIEW_H / 2);
    *cx = clamp_u8(x, 0, ROOM_W - VIEW_W);
    *cy = clamp_u8(y, 0, ROOM_H - VIEW_H);
}

int main(void)
{
    unsigned char cx, cy;

    camera(0, 0, &cx, &cy);
    assert(cx == 0 && cy == 0);

    camera(50, 50, &cx, &cy);
    assert(cx == 40 && cy == 40);

    camera(99, 99, &cx, &cy);
    assert(cx == 80 && cy == 80);

    puts("test_camera: ok");
    return 0;
}
