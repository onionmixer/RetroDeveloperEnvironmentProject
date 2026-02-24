#include <assert.h>
#include <stdio.h>

static int clamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int main(void)
{
    const int room_w = 100;
    const int room_h = 100;
    const int view_w = 12;
    const int view_h = 7;

    assert(clamp(0 - view_w / 2, 0, room_w - view_w) == 0);
    assert(clamp(50 - view_w / 2, 0, room_w - view_w) == 44);
    assert(clamp(99 - view_w / 2, 0, room_w - view_w) == 88);

    assert(clamp(0 - view_h / 2, 0, room_h - view_h) == 0);
    assert(clamp(50 - view_h / 2, 0, room_h - view_h) == 47);
    assert(clamp(99 - view_h / 2, 0, room_h - view_h) == 93);

    puts("test_camera: ok");
    return 0;
}
