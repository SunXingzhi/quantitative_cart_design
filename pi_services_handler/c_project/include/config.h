#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define DEVICE_PATH "/dev/input/js0"
#define SERIAL_PATH "/dev/ttyAMA0"
#define BAUD_RATE 9600
#define TIMEOUT_TIME_SEC 1

#define FRONT_BACKED_DISTANCE_MM 726
#define LEFT_RIGHT_DISTANCE_MM 560
#define JOY_AXIS_MAX 32767
#define JOY_AXIS_MIN -32767
#define CAR_SPEED_MAX 1.5f
#define CAR_ANGULAR_SPEED_MAX 2.0f
#define PI 3.14159265358979323846f

#define SOCKET_BIND_ADDR "0.0.0.0"
#define SOCKET_PORT 65432

#define MAX_POINTS 10

#endif // CONFIG_H
