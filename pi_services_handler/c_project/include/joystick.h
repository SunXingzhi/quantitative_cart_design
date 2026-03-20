#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

typedef struct {
    int32_t time;
    int32_t a, b, x, y;
    int32_t lb, rb, select, start, home, lo, ro;
    int32_t lx, ly, rx, ry, lt, rt, xx, yy;
} xbox_map_t;

int xbox_open(const char *device_path);
int xbox_map_read(int fd, xbox_map_t *map);
int xbox_close(int fd);

#endif // JOYSTICK_H
