#include "stm32f10x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pid.h"
#include "motor.h"

// PID 部分
extern uint8_t*		actual_duty_cycle; 
PID_TypeDef*		pid_struct;
int32_t			target_value		= 0;
//int32_t		actual_value		= 0;
int32_t			integral_errors		= 0;
int32_t			current_error		= 0;

// PID结构体恢复默认值
void PID_deinit(void)
{
	if(pid_struct!=NULL){
		free(pid_struct);
		printf("pid struct has already restart. Please init PID_struct( PID_init() ) again.");
	}
}

// PID结构体初始化：创建与电机数量相同的指针数组，单独管理每个电机的PID
// 在使用PID前必须使用此函数
void PID_init(void) 
{

	pid_struct	= (PID_TypeDef*)malloc(sizeof(PID_TypeDef)*MOTOR_NUMBER);
	if(pid_struct==NULL){
		printf("failed to allocate the memory for pid structure\n");
	}
	
	memset(pid_struct, 0, sizeof(PID_TypeDef)*MOTOR_NUMBER);

}

PID_TypeDef* PID_arguments_setting(PID_TypeDef *pid, uint32_t target)
{
	return	NULL;
}

void PID_factor_setting(float kp, float ki, float kd)
{
	uint8_t	i;
	
	i	= 0;
	if(pid_struct==NULL){
		printf("Invalid pid structure pointer.");
	}
	
	// 设置所有电机
	for(i=0; i<MOTOR_NUMBER; i++){
		pid_struct[i].kp = kp;
		pid_struct[i].ki = ki;
		pid_struct[i].kd = kd;
	}
	
	return;
	
}

// 工具类：设置PID结构参数
void PID_motor_setting(PID_TypeDef *pid, uint8_t motor_number, float Kp, float Ki, float Kd, uint32_t target)
{
	
	switch(motor_number){
		case MOTOR_CH1:
			
			break;
		case MOTOR_CH2:
			break;
		case MOTOR_CH3:
			break;
		case MOTOR_CH4:
			break;
		default:
			printf("unknown motor id!");
		
	}
	pid->kp			= Kp;
	pid->ki			= Ki;
	pid->kd			= Kd;
	pid->target		= target;
	pid->actual		= 0;
	pid->current_error	= 0;
	pid->total_errors	= 0;
//	pid->integral		= 0;
}

// PID参数计算
// Note: 传入的PID结构需要提前计算好目标速度和实际速度，以及三个系数（KP KI KD)
// 使用int32_t代替float，牺牲一些精度，程序运行效率更快
// 返回补偿后的占空比
#define	GET_TARGET_MOTOR_CYCLE(motor_target)	((uint8_t)(((int32_t)(motor_target)*100)/MOTOR_RPM_MAX))
uint8_t PID_get_new_duty_cycle(PID_TypeDef* pid_struct, uint8_t motor_id) 
{
	int32_t		proportional_value;		// kp value
	int32_t		integral_value;		// ki value
	int32_t		differential_value;	// kd value
	int16_t		new_duty_cycle;
//	int		i;
	PID_TypeDef*	motor;
	
	// initializr
	new_duty_cycle		= 0;
	proportional_value	= 0;
	integral_value		= 0;
	differential_value	= 0;
	motor			= NULL;
	
	if(motor_id>MOTOR_NUMBER){
		return	0;
	}
	
	motor	= &pid_struct[motor_id];
	
	// initialize the duty cycle array
	
	// Notice: Error have a potentional which can be less than zero.
	motor->current_error	= (int32_t)((motor->target-motor->actual));
	motor->total_errors	= (int32_t)((motor->total_errors+motor->current_error));

	// caculate KP value
	proportional_value	= (int32_t)((motor->kp)*(motor->current_error));
	// caculate KI value
	if(abs(motor->current_error)<INTEGRAL_THREASHOLD){
		motor->total_errors	+= motor->current_error;
		if(motor->total_errors > INTEGRAL_MAX)		motor->total_errors	= INTEGRAL_MAX;
		else if(motor->total_errors < -INTEGRAL_MAX)	motor->total_errors	= -INTEGRAL_MAX;
	}
	// caculate KD value
	integral_value		= (int32_t)((motor->ki)*(motor->total_errors)); 
	// 限幅处理 (0-100%)
	new_duty_cycle		= proportional_value+integral_value+differential_value;
	
	if(new_duty_cycle<0) new_duty_cycle		= 0;
	else if(new_duty_cycle>100) new_duty_cycle	= 100;	
	
	return new_duty_cycle;
}

// 需要补充
void	PID_motor_speed_setting(int32_t target_speed);
void	PID_target_motor_speed_setting(uint8_t motor_id);

