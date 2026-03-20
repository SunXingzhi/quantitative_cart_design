#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp;
    float ki;
    float kd;
    float output_limit;
    float angle0;
    float integral;
    float last_error;
    bool first_update;
} pid_controller_t;

void pid_controller_init(pid_controller_t *pid, float kp, float ki, float kd, float output_limit);
float pid_controller_update(pid_controller_t *pid, float error);
float pid_controller_output_speed(pid_controller_t *pid, float angle_error, float base_speed);

#ifdef __cplusplus
}
#endif

#endif // PID_CONTROLLER_H
