#ifndef MOTION_CONTROL_H
#define MOTION_CONTROL_H

#include <stdint.h>

typedef struct {
    int32_t lx, ly, rx, ry, lt, rt;
    int32_t a, b, x, y;
} joystick_state_t;

typedef enum {
    CAR_STATUS_EXCEPTION_ERROR = -1,
    CAR_STATUS_STOP = 0,
    CAR_STATUS_DEFAULT = 1,
    CAR_STATUS_CHANGE_POSITION = 2,
    CAR_STATUS_BRAKE = 3,
    CAR_STATUS_SPRAY = 4,
    CAR_STATUS_GPS = 5,
    CAR_STATUS_NAVIGATION = 6,
    CAR_STATUS_OPEN_BRAKE = 8,
    CAR_STATUS_CLOSE_BRAKE = 9
} car_status_t;

typedef struct {
    float v_cx;
    float omega;
    int32_t lx;
    int32_t ly;
    int32_t rx;
    int32_t ry;
    int32_t lt;
    int32_t rt;
} joystick_converted_t;

typedef struct {
    int32_t v_fl, v_fr, v_bl, v_br;
} motor_speed_t;

typedef struct {
    int32_t angle_x, angle_y;
} motor_angle_t;

joystick_converted_t convert_joystick_data(const joystick_state_t *st);
motor_speed_t parsed_motors_speed(float v_cx, float omega);
motor_angle_t parsed_motors_angle(int32_t pos1, int32_t pos2);
float haversine_distance(float a_lat, float a_lon, float b_lat, float b_lon);
float calculate_heading_angle(float a_lat, float a_lon, float b_lat, float b_lon);

car_status_t get_status(int a, int b, int y);
joystick_state_t apply_status_to_joystick(car_status_t status, joystick_state_t st);

#endif // MOTION_CONTROL_H
