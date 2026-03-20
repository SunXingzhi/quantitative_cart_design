#include "joystick.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <string.h>
#include <errno.h>

int xbox_open(const char *device_path)
{
        int fd = open(device_path, O_RDONLY | O_NONBLOCK);
        return fd < 0 ? -1 : fd;
}

int xbox_map_read(int fd, xbox_map_t *map)
{
        if (!map || fd < 0)
                return -1;
        struct js_event js;
        ssize_t ret = read(fd, &js, sizeof(js));
        if (ret != sizeof(js))
                return -1;

        if (js.type & JS_EVENT_INIT)
                return -1;

        if (js.type & JS_EVENT_BUTTON)
        {
                switch (js.number)
                {
                case 0:
                        map->a = js.value;
                        break;
                case 1:
                        map->b = js.value;
                        break;
                case 2:
                        map->x = js.value;
                        break;
                case 3:
                        map->y = js.value;
                        break;
                case 4:
                        map->lb = js.value;
                        break;
                case 5:
                        map->rb = js.value;
                        break;
                case 6:
                        map->select = js.value;
                        break;
                case 7:
                        map->start = js.value;
                        break;
                case 8:
                        map->home = js.value;
                        break;
                case 9:
                        map->lo = js.value;
                        break;
                case 10:
                        map->ro = js.value;
                        break;
                default:
                        break;
                }
        }
        if (js.type & JS_EVENT_AXIS)
        {
                switch (js.number)
                {
                case 0:
                        map->lx = js.value;
                        break;
                case 1:
                        map->ly = js.value;
                        break;
                case 2:
                        map->lt = js.value;
                        break;
                case 3:
                        map->rx = js.value;
                        break;
                case 4:
                        map->ry = js.value;
                        break;
                case 5:
                        map->rt = js.value;
                        break;
                case 6:
                        map->xx = js.value;
                        break;
                case 7:
                        map->yy = js.value;
                        break;
                default:
                        break;
                }
        }

        return 0;
}

int xbox_close(int fd)
{
        if (fd >= 0)
                close(fd);
        return 0;
}
