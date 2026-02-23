#include "delay.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "system_config.h"
#include "gps.h"
#include "motor.h"
#include "public.h"
#include "pid.h"
#include "communication.h"

#define	STM32F103C8T6_HEAP_SIZE		0x00000200
#define	STM32F103C8T6_STACK_SIZE	0X00000400

// 分配内存函数重定向
// 传参为单元大小，单元个数
// 分配成功后自动设置内存数据为0
void*	safely_allocate_HEAP_memory(size_t unit_size, uint32_t unit_number, char initial_char_value)
{
	void*	data_buffer;
	
	data_buffer	= NULL;
	data_buffer	= malloc(unit_number*unit_size);
	if(data_buffer==NULL || (unit_size*unit_number>=STM32F103C8T6_HEAP_SIZE)){
		return	NULL;
	}
	
	memset(data_buffer, 0, unit_number*unit_size);
	
	return	data_buffer;
}

uint8_t	safely_free_HEAP_memory(void* data)
{
	if(data!=NULL){
		free(data);
	}
	
	return 0;
}

// 声明外部变量
extern uint32_t	motor_period_times;		// 电机测量脉冲时间隔周期
extern uint32_t	motor_first_CCR_count;		// 电机产生脉冲时的CCR值
extern uint32_t	motor_last_CCR_count;
extern uint8_t	write_position;
extern uint8_t	Rx_state;
extern volatile uint8_t		motor_pulse_count;		// 计算当前电机一圈的脉冲次数
extern volatile uint8_t		motor_first_detection_status;	// 电机测速状态
extern volatile uint8_t		motor_finished_detection_status;
extern volatile uint16_t	motor_timer_overflow_count;
extern volatile char		command_length;
// 串口接收状态
extern	volatile uint8_t	receive_finished_status;

extern	char	usart1_received_data[50];
// 电机PID参数结构体引用
extern	PID_TypeDef*		pid_struct;
extern	int32_t			current_error;
extern	int32_t			integral_errors;
//extern	int32_t			target_value;
uint8_t*			actual_duty_cycle;
extern	MOTOR_INFORMATION	motor_information;
extern	uint32_t		last_capture_time[MOTOR_NUMBER];
extern	volatile uint32_t	systick_counter;

// GPS 经纬度数据
extern float latitude_f;
extern float longtitude_f;


// 函数声明
void*	safely_allocate_HEAP_memory(size_t unit_size, uint32_t unit_number, char initial_char_value);
uint8_t	safely_free_HEAP_memory(void* data_buffer);

//全局变量定义
GPS_Handler_t	gps_handler;	//GPS定位数据结构

// 主程序入口
int main(){
	// 声明变量
	uint16_t*	motor_speed;
	char		usart1_received_data_buffer[50];
	uint8_t*	new_duty_cycle;
        char            gps_received_data_buffer[50];
//	static uint32_t last_pid_time;
	
//	last_pid_time	= 0;
	motor_speed	= NULL;
	new_duty_cycle	= NULL;
	
	system_init();
	hardware_init();
	software_package_init();
	// 中断 配置
	NVIC_interupt_config();
	
	actual_duty_cycle	= (uint8_t*)safely_allocate_HEAP_memory(sizeof(uint8_t), MOTOR_NUMBER, 0);
	if(actual_duty_cycle!=NULL){
		printf("Allocate memory for actual_duty_cycle successfully.");
	}
	
	new_duty_cycle		= (uint8_t*)safely_allocate_HEAP_memory(sizeof(uint8_t), MOTOR_NUMBER, 0);
	if(new_duty_cycle!=NULL){
		printf("Allocate memory for new_duty_cycle successfully.");
	}
	
	// 串口接收缓冲区初始化
	memset(usart1_received_data_buffer, 0, sizeof(usart1_received_data_buffer));
        memset(gps_received_data_buffer, 0, sizeof(gps_received_data_buffer));
	// 设置PID参数
	PID_factor_setting(0.01, 0.009, 0.1);

	//GPS数据结构初始化
	GPS_Init(&gps_handler);
	
	// 设置电机初始值
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH1, 50);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH2, 50);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH3, 50);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH4, 50);
	
	while(1){
		//GPS解析和发送经纬度数据
		//在接收状态为完成时，解析数据
		if(gps_handler.state==GPS_COMPLETE){
			//解析数据
			GPS_Parse(&gps_handler);
			
			if((gps_handler.raw_buffer[GPS_STATUS_OFFSET])=='A'){
				//发送数据(debug)
				Send_GPS_String(USART3, gps_handler.coordinate, GPS_COORDINATE_SIZE);
                                // 构造经纬度数据字符串
                                sprintf(gps_received_data_buffer, "#n/2/%.6f/%.6f*\n", latitude_f, longtitude_f);
                                // todo 检验这里的字符串
                                send_string_in_specific_way(USART_WAY, USART1, gps_received_data_buffer);
                        }
			//转换状态
			gps_handler.state	= GPS_IDLE;
		}
		
		// 接收并处理树莓派命令
		if(receive_finished_status==1){
			// 解析数据(包括清除接收数据的缓冲区)
			// 解析后pid结构中已经包含了目标速度
			parse_usart1_command(usart1_received_data_buffer);
			// 清空接收的数据
			memset(usart1_received_data, 0, sizeof(usart1_received_data));
			// 清空接受的缓冲区
			memset(usart1_received_data_buffer, 0, sizeof(usart1_received_data_buffer));
			// 清空命令长度
			command_length		= 0;
			// 重置接收状态
			receive_finished_status	= 0;
		}
		
		check_motor_stop(MOTOR_CH1);  
		// 处理电机逻辑
		if(motor_finished_detection_status==1){
			motor_speed = (get_motor_speed(MOTOR_CH1));
			
//			// 获取新占空比,存储在全局变量中
			new_duty_cycle[MOTOR_CH1]	= PID_get_new_duty_cycle(pid_struct, MOTOR_CH1);
//			// 设置电机速度
////			PID_target_motor_speed_setting(MOTOR_CH1, actual_duty_cycle[MOTOR_CH1]);
//			target_motor_speed_setting(MOTOR_CH1, new_duty_cycle[MOTOR_CH1]);
//			
//			printf("%u,%u,%u\n", pid_struct[MOTOR_CH1].target, new_duty_cycle[MOTOR_CH1], motor_speed[MOTOR_CH1]);
			// 重置状态机
			motor_finished_detection_status	= 0;
			motor_first_detection_status	= 0;	// 允许新周期开始
			motor_pulse_count		= 0;	// 重置脉冲计数
   					
			// 释放内存
//			free(motor_speed);
//			motor_speed	= NULL;
		}
		
	}
}
