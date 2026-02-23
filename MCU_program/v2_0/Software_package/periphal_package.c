/* 软件外设封装库
 * 用途	将标准外设库的函数进行二次封装，便于理解业务逻辑以及整理项目
 * 包含	TIMER、USART、GPIO、DMA、RCC、I2C 等外设
 *
 *
 *
 *
 */
#include "stm32f10x.h"
#include "periphal_package.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "delay.h"
#include "motor.h"
#include "pid.h"

//#include "OLED.h"
// 声明外部变量
extern	uint32_t 		motor_period_times;
extern	uint32_t 		motor_first_CCR_count;
extern	uint32_t		motor_last_CCR_count;
extern	volatile uint8_t	motor_pulse_count;
extern	volatile uint8_t	motor_first_detection_status;
extern	volatile uint8_t	motor_finished_detection_status;
extern	volatile uint16_t	motor_timer_overflow_count;
extern	volatile uint32_t	systick_counter;	// 系统滴答计数器
extern	volatile uint32_t	last_capture_time[MOTOR_NUMBER];






	




