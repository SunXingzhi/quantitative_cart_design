#include "motion_control.h"
#include "config.h"
#include <math.h>

joystick_converted_t convert_joystick_data(const joystick_state_t *st)
{
        joystick_converted_t out;
        float v_cx = -(float)st->ly / JOY_AXIS_MAX * CAR_SPEED_MAX;
        float omega = (float)st->rx / JOY_AXIS_MAX * CAR_ANGULAR_SPEED_MAX;

        if (v_cx > CAR_SPEED_MAX)
                v_cx = CAR_SPEED_MAX;
        if (v_cx < -CAR_SPEED_MAX)
                v_cx = -CAR_SPEED_MAX;
        if (omega > CAR_ANGULAR_SPEED_MAX)
                omega = CAR_ANGULAR_SPEED_MAX;
        if (omega < -CAR_ANGULAR_SPEED_MAX)
                omega = -CAR_ANGULAR_SPEED_MAX;

        out.v_cx = v_cx;
        out.omega = omega;
        out.lx = st->lx;
        out.ly = st->ly;
        out.rx = st->rx;
        out.ry = st->ry;
        out.lt = st->lt;
        out.rt = st->rt;
        return out;
}

motor_speed_t parsed_motors_speed(float v_cx, float omega)
{
        motor_speed_t out;
        float left_right_dist = LEFT_RIGHT_DISTANCE_MM / 1000.0f;
        float v_left = v_cx - (left_right_dist / 2.0f) * omega;
        float v_right = v_cx + (left_right_dist / 2.0f) * omega;

        int32_t left_percent = (int32_t)roundf(v_left / CAR_SPEED_MAX * 100.0f);
        int32_t right_percent = (int32_t)roundf(v_right / CAR_SPEED_MAX * 100.0f);

        if (left_percent < -100)
                left_percent = -100;
        if (left_percent > 100)
                left_percent = 100;
        if (right_percent < -100)
                right_percent = -100;
        if (right_percent > 100)
                right_percent = 100;

        out.v_fl = left_percent;
        out.v_fr = -right_percent;
        out.v_bl = left_percent;
        out.v_br = -right_percent;
        return out;
}

static int32_t angle_mapping(int32_t pos)
{
        float ratio = ((float)pos - JOY_AXIS_MIN) / ((float)JOY_AXIS_MAX - JOY_AXIS_MIN);
        if (ratio < 0.0f)
                ratio = 0.0f;
        if (ratio > 1.0f)
                ratio = 1.0f;
        return (int32_t)roundf(ratio * 180.0f);
}

motor_angle_t parsed_motors_angle(int32_t pos1, int32_t pos2)
{
        motor_angle_t out;
        out.angle_x = angle_mapping(pos1);
        out.angle_y = angle_mapping(pos2);
        return out;
}

float haversine_distance(float a_latitude, float a_longitude, float b_latitude, float b_longitude)
{
        float lat1 = a_latitude * PI / 180.0f;
        float lon1 = a_longitude * PI / 180.0f;
        float lat2 = b_latitude * PI / 180.0f;
        float lon2 = b_longitude * PI / 180.0f;

        float dlat = lat2 - lat1;
        float dlon = lon2 - lon1;

        float sin_dlat = sinf(dlat / 2.0f);
        float sin_dlon = sinf(dlon / 2.0f);

        float a = sin_dlat * sin_dlat + cosf(lat1) * cosf(lat2) * sin_dlon * sin_dlon;
        float c = 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));

        return 6371000.0f * c;
}

float calculate_heading_angle(float a_latitude, float a_longitude, float b_latitude, float b_longitude)
{
        float lat1 = a_latitude * PI / 180.0f;
        float lon1 = a_longitude * PI / 180.0f;
        float lat2 = b_latitude * PI / 180.0f;
        float lon2 = b_longitude * PI / 180.0f;

        float dlon = lon2 - lon1;

        float x = sinf(dlon) * cosf(lat2);
        float y = cosf(lat1) * sinf(lat2) - sinf(lat1) * cosf(lat2) * cosf(dlon);

        float heading = atan2f(x, y) * 180.0f / PI;
        if (heading < 0)
                heading += 360.0f;
        else if (heading >= 360.0f)
                heading -= 360.0f;

        return heading;
}

static car_status_t car_status_register = CAR_STATUS_DEFAULT;

car_status_t get_status(int a, int b, int y)
{
        car_status_t new_status = car_status_register;

        if (a)
                new_status = CAR_STATUS_BRAKE;
        else if (y)
                new_status = CAR_STATUS_CHANGE_POSITION;
        else if (b)
                new_status = CAR_STATUS_DEFAULT;

        if (new_status != car_status_register)
        {
                if (new_status == CAR_STATUS_BRAKE && car_status_register == CAR_STATUS_DEFAULT)
                        new_status = CAR_STATUS_OPEN_BRAKE;
                else if (new_status == CAR_STATUS_DEFAULT && car_status_register == CAR_STATUS_BRAKE)
                        new_status = CAR_STATUS_CLOSE_BRAKE;
                car_status_register = new_status;
        }

        return car_status_register;
}

joystick_state_t apply_status_to_joystick(car_status_t status, joystick_state_t st)
{
        switch (status)
        {
        case CAR_STATUS_STOP:
                st.lx = st.ly = st.rx = st.ry = st.lt = st.rt = 0;
                break;
        case CAR_STATUS_DEFAULT:
                st.a = st.b = st.x = st.y = 0;
                break;
        case CAR_STATUS_BRAKE:
        case CAR_STATUS_OPEN_BRAKE:
        case CAR_STATUS_CLOSE_BRAKE:
                st.ly = 0;
                st.rx = 0;
                break;
        default:
                break;
        }
        return st;
}
