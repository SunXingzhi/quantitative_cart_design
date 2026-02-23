#include "stm32f10x.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Navigation.h>
#include "communication.h"
#include <stdarg.h>
#define MAX_FORMAT_BUFFER_SIZE 256
#include "pid.h"
#include "motor.h"
#include "motor.h"

// Global variable configuration
char				usart1_received_data[50];
volatile char			usart1_received_byte;
volatile char*			usart1_latest_command;
//char				usart1_received_data_buffer[50];
volatile char			command_length		= 0;
uint8_t				Rx_state		= 0;			
uint8_t 			write_position		= 0;	
volatile uint8_t		receive_finished_status	= 0;
uint8_t				Serial_RxFlag;				//定义接收数据包标志位
extern MOTOR_INFORMATION	motor_information;
extern PID_TypeDef*		pid_struct;

// 定义枚举变量
COMMUNICATION_WAY communication_way;

// 串口发送字符串
void USART_send_string(USART_TypeDef* USARTx, char* data_string)
{
	int	i;
	
	// 检测字符串
	if(data_string==NULL){
		// 置位错误提示
		return;
	}
	for(i=0;data_string[i]!='\0';i++){
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE)==RESET);
		USART_SendData(USARTx, data_string[i]);
	}
}

// 指定发送方式发送数据
void send_string_in_specific_way(COMMUNICATION_WAY way,  \
				void* device_address, char* data_string)
{
	switch(way){
		case USART_WAY:
			// 转换设备地址
			USART_send_string((USART_TypeDef*)device_address, data_string);
			break;
                // TODO: 其他发送方式
		case SOCKET_WAY:
			break;
		case I2C_WAY:
			break;
		case CAN_WAY:
			break;
		case BLUETOOTH_WAY:
			break;
	}
	return ;
}

#define	GET_MOTORS_DIRECTION	{	\
	\
}
// 解析串口命令
void parse_usart1_command(char* received_data_buffer)
{
	char*	buffer;
	char	command_type;
//	int	motor_speed[4];
	int	i;
//	int 	j;
	int	z;
	int	x;
	bool	is_turned[2];			// 查看两侧电机方向是否改变的状态值
//	int8_t	last_motor_direction[2];	// 两侧电机
	int32_t	motor_duty_cycle;
        int8_t  temp_direction[2];

        // 安全检查
	if(command_length==0 || usart1_received_data[command_length]!='\0'){
		printf(" ERR:MISSING_NULL\n");
		// 清空缓冲区数据
		memset(received_data_buffer, 0, command_length);
		return;
	}
	
	// 变量初始化
	for(i=0;i<2;i++){
		is_turned[i]		= false;
	}
	motor_duty_cycle	= 0;
	command_type	= usart1_received_data[0];
	
	// 复制到缓冲区供 strtok 使用, 需要注意范围
	memcpy(received_data_buffer, usart1_received_data, command_length);
	
	switch(command_type){
		// 导航命令 n/v_lf/v_rf/move_time
		case 'n':{
                        Navigation_ParseCommand(received_data_buffer);
                }
                // 电机控制 m/lf/rf/lb/rb
                case 'm':{ 
//			// 当接收到新的电机控制命令时，重置电机状态为默认状态
			if(motor_information.motor_status==BRAKE_STATUS) {
				// 如果当前是制动状态，先释放制动
				control_motors_brake(false);
			}
			// 重置电机状态为默认状态
			motor_information.motor_status = DEFAULT_STATUS;
			
			if(motor_information.motor_status==NEVIGATION_STATUS){
				printf("warning: current car status isn't deault status.\n");
				break;
			}
//			printf("recd:%s\n", received_data_buffer);
			buffer	= strtok(received_data_buffer, "/");
			buffer	= strtok(NULL, "/"); // 跳过 "m"
			i	= 0;
			
			
			temp_direction[CAR_LEFT_MOTORS] = motor_information.motor_direction[CAR_LEFT_MOTORS];
			temp_direction[CAR_RIGHT_MOTORS] = motor_information.motor_direction[CAR_RIGHT_MOTORS];
			
			while(buffer!=NULL && i<4){
//				motor_speed[i]	= atoi(buffer);
				motor_duty_cycle	= atoi(buffer);
				
				if(motor_duty_cycle<0){
					motor_information.motor_duty_cycle[i]	= -motor_duty_cycle;
					// 根据电机所在侧设置临时方向
					if(i==MOTOR_LEFT_FORWARD || i==MOTOR_LEFT_BACKED){
						temp_direction[CAR_LEFT_MOTORS]	= MOTOR_BACKED_DIRECTION;
					} else{
						temp_direction[CAR_RIGHT_MOTORS]	= MOTOR_FORWARD_DIRECTION;
					}
					
				} else{ // 0同样代表正向
//				
					motor_information.motor_duty_cycle[i]		= motor_duty_cycle;
					if(i==MOTOR_LEFT_FORWARD || i==MOTOR_LEFT_BACKED){
						temp_direction[CAR_LEFT_MOTORS]		= MOTOR_FORWARD_DIRECTION;
					} else{
						temp_direction[CAR_RIGHT_MOTORS]	= MOTOR_BACKED_DIRECTION;
					}
				}
				
				motor_information.target_motor_speed[i]	= GET_TARGET_MOTOR_SPEED(motor_information.motor_duty_cycle[i]);			
				pid_struct[i].target			= get_target_motor_speed(motor_information.motor_duty_cycle[i]);
				
				i++;
				buffer = strtok(NULL, "/");
			}
			
			if(i==4){
				// 对于原地转弯等情况，两侧电机方向相同
				motor_information.motor_direction[CAR_LEFT_MOTORS] = temp_direction[CAR_LEFT_MOTORS];
				motor_information.motor_direction[CAR_RIGHT_MOTORS] = temp_direction[CAR_RIGHT_MOTORS];
			}
			if(i!=4){
				printf(" ERR:MOTOR_NEED_4_PARAMS\n");
				break;
			}
//			target_value	= motor_speed[0];
//			target_value	= get_target_motor_speed(motor_information.motor_duty_cycle[MOTOR_CH1]);
			// debug:改变电机速度
//			PID_target_motor_speed_setting(MOTOR_CH1, motor_information.motor_duty_cycle[MOTOR_CH1]);
			// 根据方向是否改变调整电机方向
			check_site_motors_direction(	is_turned, 
							motor_information.motor_last_direction,
							motor_information.motor_direction);
//			i	= 0;
//			printf("last1:%c, now1:%c;last2:%c, now2:%c\n", motor_information.motor_last_direction[0],
//				motor_information.motor_direction[0], 
//				motor_information.motor_last_direction[1],
//				motor_information.motor_direction[1]);
			for(i=0;i<2;i++){
				if(is_turned[i]==true){
					control_motors_direction(motor_information.motor_direction[i], i);
					motor_information.motor_last_direction[i]	= motor_information.motor_direction[i];
				}
			}
			
			target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH1, motor_information.motor_duty_cycle[MOTOR_CH1]);
			target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH2, motor_information.motor_duty_cycle[MOTOR_CH2]);			
			target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH3, motor_information.motor_duty_cycle[MOTOR_CH3]);			
			target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH4, motor_information.motor_duty_cycle[MOTOR_CH4]);			
			printf(" MOTORS lf SET:%u\n", motor_information.target_motor_speed[MOTOR_CH1]);
			
			break;
		}
		
		case 'b':{ // 刹车 @b/1* 开启刹车, @b/0* 关闭刹车
			// set all motors to 0
//			printf("recd buffer:%s|\n", received_data_buffer);
			buffer	= strtok(received_data_buffer, "/");
			buffer	= strtok(NULL, "/");	
//			printf(" 4324234242424BRAKE\n");			
			if(atoi(buffer)==1){
				car_stop(BY_BRAKES_BUT_LOCKED);	// 强延时0.5s
				// 设置电机状态
				motor_information.motor_status	= BRAKE_STATUS;
							
			} 
			else if(atoi(buffer)==0){
				// 关闭制动器
				control_motors_brake(false);
				motor_information.motor_status	= DEFAULT_STATUS;
//				printf(" Closing Brake...\n");
			}

			break;
		}
		
		case 'v':{ // 舵机 @v/x/z*
			buffer	= strtok(received_data_buffer, "/");
			buffer	= strtok(NULL, "/");
			x	= buffer ? atoi(buffer) : 90;	// 设置默认角度
			buffer	= strtok(NULL, "/");
			z	= buffer ? atoi(buffer) : 90;	// 设置默认角度
			
			if(x<0 || x>180 || z<0 || z>180){
				printf("ERR:SERVO_RANGEx:%d,z:%d\n", x, z);
				break;
			}
			
			servo_angle_change(x, z);
			printf("SERVO SET,X:%d Z:%d\n",x, z);
			break;
		}
		
		case 'o':{ // 开启GPS
			// gps_enabled = 1;
			USART_send_string(USART1, "GPS ON\n");
			// GPS模块接口
			break;
		}
		
		case 'c':{ // 关闭GPS
			// gps_enabled = 0;
			printf("GPS OFF\n");
			break;
		}
		
//		default:{
//			printf("ERR:UNKNOWN_CMD\n");
//			break;
//		}
	}
	// 清空缓冲区数据
	memset(received_data_buffer, 0, command_length);
}
