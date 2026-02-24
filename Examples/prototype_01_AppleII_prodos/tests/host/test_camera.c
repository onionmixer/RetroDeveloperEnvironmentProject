#include <stdio.h>

#define VIEW_W 10
#define VIEW_H 10
#define ROOM_W 100
#define ROOM_H 100

static unsigned char clamp_cam(int v, int hi)
{
    if (v < 0) return 0;
    if (v > hi) return (unsigned char)hi;
    return (unsigned char)v;
}

int main(void)
{
    unsigned char cam_x, cam_y;

    cam_x = clamp_cam(0 - (VIEW_W / 2), ROOM_W - VIEW_W);
    cam_y = clamp_cam(0 - (VIEW_H / 2), ROOM_H - VIEW_H);
    if (cam_x != 0 || cam_y != 0) return 1;

    cam_x = clamp_cam(50 - (VIEW_W / 2), ROOM_W - VIEW_W);
    cam_y = clamp_cam(50 - (VIEW_H / 2), ROOM_H - VIEW_H);
    if (cam_x != 45 || cam_y != 45) return 2;

    cam_x = clamp_cam(99 - (VIEW_W / 2), ROOM_W - VIEW_W);
    cam_y = clamp_cam(99 - (VIEW_H / 2), ROOM_H - VIEW_H);
    if (cam_x != 90 || cam_y != 90) return 3;

    puts("CAMERA_OK");
    return 0;
}
