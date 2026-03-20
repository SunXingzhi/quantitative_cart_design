#include "pid_controller.h"
#include <math.h>

static float clamp_float(float v, float min_v, float max_v)
{
    if (v < min_v)
        return min_v;
    if (v > max_v)
        return max_v;
    return v;
}

void pid_controller_init(pid_controller_t *pid, float kp, float ki, float kd, float output_limit)
{
    if (!pid)
        return;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_limit = output_limit;
    pid->angle0 = 0.0f;
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->first_update = true;
}

float pid_controller_update(pid_controller_t *pid, float error)
{
    if (!pid)
        return 0.0f;

    if (pid->first_update)
    {
        pid->last_error = error;
        pid->first_update = false;
    }

    pid->integral += error;
    float derivative = error - pid->last_error;
    float output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
    output = clamp_float(output, -pid->output_limit, pid->output_limit);
    pid->last_error = error;

    if (pid->angle0 <= 0.0f)
    {
        pid->angle0 = fabsf(error);
        if (pid->angle0 == 0.0f)
            pid->angle0 = 1.0f;
    }

    return output;
}

float pid_controller_output_speed(pid_controller_t *pid, float angle_error, float base_speed)
{
    if (!pid)
        return 0.0f;

    float correction = base_speed * pid_controller_update(pid, angle_error) / (pid->angle0 > 0.0f ? pid->angle0 : 1.0f);
    return clamp_float(correction, -100.0f, 100.0f);
}
