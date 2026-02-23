#ifndef __MOTOR_H__
#define __MOTOR_H__
#include "stm32f10x.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
// =========================== motor part ========================
#define	MOTOR_NUMBER			4
// motor channel defination
#define MOTOR_CH1       		0   	// 电机1：TIM3_CH1（PA6）+ IN1=PA1、IN2=PA2
#define MOTOR_CH2       		1   	// 电机2：TIM3_CH2（PA7）+ IN1=PA3、IN2=PA4
#define MOTOR_CH3       		2   	// 电机3：TIM3_CH3（PB0）+ IN1=PB1、IN2=PB2
#define MOTOR_CH4       		3   	// 电机4：TIM3_CH4（PB1）+ IN1=PB3、IN2=PB4
#define	MOTOR_PULSE_MAX			199UL
#define MOTOR_CAPUTRE_MAX		65535UL
#define	MOTOR_RPM_MAX			2400
#define MOTOR_ENCODER_PULSE		13	// 电机每圈产生12个脉冲
#define MOTOR_CAPTURE_TIMER_FREQ	1000000	// TIM4 频率
#define	SERVO_PULSE_MAX			19999
#define	MOTOR_INIT_SPEED		100
#define	MOTOR_INIT_SPEED_ZERO		0
#define SERVO_INIT_ANGLE		2500
#define	MOTOR_DRIVER_MIN_DUTY_CYCLE	10	
#define	MOTOR_TARGET_MIN_SPEED		120
#define	MOTOR_SPEED_STEP_PER_DUTY_CYCLE	24
#define	MOTOR_STOP_TIMEOUT_MS		200	// unit:ms
// motor PWM TIM3				
#define	MOTOR_GPIOA			GPIOA
#define	MOTOR_GPIOB			GPIOB
#define	GPIOA_MOTOR_PWM_PIN		GPIO_Pin_6 | GPIO_Pin_7
#define	GPIOB_MOTOR_PWM_PIN		GPIO_Pin_0 | GPIO_Pin_1
#define	MOTOR_TIM			TIM3
#define	CAPTURE_TIM			TIM4
#define	SERVO_TIM			TIM2
// motor diriction define
#define	MOTOR_DIRECTION_POSITIVE	(uint8_t)1
#define	MOTOR_DIRECTION_NEGATIVE	(uint8_t)-1

// car direction define(The brake site is the head of car)
#define	CAR_DIRECTION_FORWARD		(int8_t)0
#define CAR_DIRECTION_BACKWARD		(int8_t)1

// servo PWM TIM2 CH1 CH2	
#define SERVO_GPIO			GPIOA
#define GPIO_SERVO_PWM_PIN		GPIO_Pin_0 | GPIO_Pin_1
	
// input capture TIM4
#define	INPUT_CAPTURE_GPIO		GPIOB
#define	INPUT_CAPTURE_PIN		GPIO_Pin_6	|	\
					GPIO_Pin_7	|	\
					GPIO_Pin_8	|	\
					GPIO_Pin_9		

#define	MOTOR_DIRECTION_GPIO		GPIOB		
#define	MOTOR_DIRECTION_PIN		GPIO_Pin_12	|	\
					GPIO_Pin_13	|	\
					GPIO_Pin_14	|	\
					GPIO_Pin_15	
#define	MOTOR_DIRECTION_CONTROL_LEFT_PIN GPIO_Pin_12	| GPIO_Pin_14
#define	MOTOR_DIRECTION_CONTROL_RIGHT_PIN GPIO_Pin_13	| GPIO_Pin_15

#define	MOTOR_BRAKE_GPIO		GPIOA	
#define	MOTOR_BRAKE_PIN			GPIO_Pin_4	|	\
					GPIO_Pin_5



// PID target motor speed
#define	GET_TARGET_MOTOR_SPEED(duty_cycle)	duty_cycle*MOTOR_RPM_MAX/100
#define	PERCENT_TARGET_MOTOR_CYCLE(duty_cycle)	duty_cycle/100
// 电机方向定义
#define	MOTOR_FORWARD_DIRECTION		0
#define MOTOR_BACKED_DIRECTION		1
#define	MOTOR_NULL_DIRECTION		2

// 车载电机重定义
#define	MOTOR_LEFT_FORWARD	MOTOR_CH1
#define	MOTOR_RIGHT_FORWARD	MOTOR_CH2
#define	MOTOR_LEFT_BACKED	MOTOR_CH3
#define	MOTOR_RIGHT_BACKED	MOTOR_CH4

#define	CAR_LEFT_MOTORS		0	// lf, lb
#define	CAR_RIGHT_MOTORS	1	// rf, rb

// 定义车辆停止方式
#define WITHOUT_BRAKES	0
#define BY_BRAKES_BUT_ACTIVE	1	// 短暂制动后立即释放
#define	BY_BRAKES_BUT_LOCKED	2	// 制动并保持锁定状态
// motor information setting
typedef	struct{
	GPIO_TypeDef*	GPIO;
	uint16_t	MOTOR_PIN;
}	MOTOR_PIN_INFORMATION;

// 电机状态枚举
// 
typedef enum{
	DEFAULT_STATUS		= 0,	// 电机默认状态
	NEVIGATION_STATUS	,	// 导航状态
	BRAKE_STATUS,			// 刹车状态
	IN_PLACE_TURNING_STATUS	// 原地转向状态
}	MOTOR_STATUS;

// 电机信息结构
typedef struct{
	int8_t		motor_direction[2];	// 当前电机方向
	int8_t		motor_last_direction[2];// 上一次电机方向
	uint16_t	target_motor_speed[4];		// 正数
	uint8_t		motor_duty_cycle[4];
	MOTOR_STATUS	motor_status;
}	MOTOR_INFORMATION;

typedef struct{
	uint8_t		servo_angle;
	uint8_t		servo_duty_cycle;
}	SERVO_INFORMATION;

typedef struct{
	uint32_t		first_CCR;
	uint32_t		last_CCR;
	uint16_t		overflow_count;
	uint8_t			pulse_count;
	volatile uint8_t	first_detection;
	volatile uint8_t	finished_detection;	
}	MOTOR_CAPTURE_TypeDef;

void		motor_init(void);
void		servo_init(void);
void		motor_gpio_init(void);
void		motor_tim_init(void);
void		motor_set_default_value(void);
void		car_stop(int8_t additional_operation);
//void motor_speed_setting(int speed_retio);	// 改变电机速度
void		target_motor_speed_setting(uint8_t motors_status, uint8_t motor_id, uint8_t duty_cycle);	// 改变电机速度
void		process_motor_capture(uint8_t motor_id, uint16_t current_value);
uint16_t*	get_motor_speed(uint8_t motor_ch);
void		check_motor_stop(int16_t motor_id);		// 检测电机是否停止
// DEBUG
uint16_t	get_target_motor_speed(int duty_cycle);
uint16_t*	get_target_motors_speed(void);
uint16_t	get_motor_actual_speed(uint8_t motor_id);
void		control_motors_brake(bool state);
void		servo_angle_change(uint16_t angle1,uint16_t angle2);
void		control_motors_direction(int8_t motor_direction, int8_t motor_site);
void 		control_car_direction(int8_t car_direction);
bool*		check_site_motors_direction(	bool* change_status_buffer, 
						int8_t* last_motor_direction, 
						volatile int8_t* motor_direction);
#endif

