#ifndef __PID_H__
#define	__PID_H__
#include "stm32f10x.h"

// debug--积分阈值（误差符合当前大小才进入）
#define	INTEGRAL_THREASHOLD	20	// 20rpm
#define	INTEGRAL_MAX		10000	

// PID argument structure setting size: 32B
typedef struct{
	float	kp;
	float	ki;
	float	kd;
	int32_t	target;
	int32_t	actual;
	int32_t	current_error;
	int32_t	total_errors;
//	int32_t	integral;
}	PID_TypeDef;

void	PID_deinit(void);
void	PID_init(void);
void	PID_argument_setting(PID_TypeDef *pid, uint8_t motor_id, float Kp, float Ki, float Kd, uint32_t target);
void	PID_factor_setting(float kp, float ki, float kd);
uint8_t	PID_get_new_duty_cycle(PID_TypeDef* pid_struct, uint8_t motor_id);
void	PID_motor_speed_setting(int32_t target_speed);
void	PID_target_motor_speed_setting(uint8_t motor_id);

#endif

