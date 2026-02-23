#include "stm32f10x.h"
#include "Navigation.h"
#include "motor.h"
#include <string.h>
#include <stdio.h>
#include "PID.h"
#include "gps.h"
#include "communication.h"
#include <math.h>
// 外部变量声明
extern MOTOR_INFORMATION motor_information;	 // 电机信息 包括电机的 方向 速度占空比
extern char usart1_received_data[50];		   // main中接受通过串口树莓派的命令
extern volatile uint8_t receive_finished_status;// 串口接受完成标志位
extern PID_TypeDef* pid_struct;				 // 电机的PID
extern GPS_Handler_t gps_handler;			   // GPS的结构体

// 导航信息全局变量
NavigationInfo navigation_info = {
	.target_latitude = 0.0f,	// 下一点目标纬度
	.target_longitude = 0.0f,   // 下一点目标经度
	.current_latitude = 0.0f,   // 小车当前的纬度
	.current_longitude = 0.0f,  // 小车当前的经度
	.left_motor_speed = 0,	  // 左侧电机的速度
	.right_motor_speed = 0,	 // 右侧电机的速度
	.move_duration = 0,		 // 需要运动的时间
	.state = NAVIGATION_OFF	 // 导航的当前状态
};

void Navigation_Begin()
{
	// 设置电机状态为导航状态
	motor_information.motor_status = NEVIGATION_STATUS;
	
	// 设置导航状态为开始
	navigation_info.state = NAVIGATION_STARTING;
	
	// 发送导航开始ACK应答
	Navigation_SendAck(NAVIGATION_ACK_START, 0, 0);
	
	
}

void Navigation_SendInfo(uint8_t ack_type, float latitude, float longitude)
{
	char ack_buffer[50];
	
	switch(ack_type) {
		
		// 导航开始响应
		case NAVIGATION_ACK_START:
			snprintf(ack_buffer, sizeof(ack_buffer), "#n/0*");
			break;
		
		// 导航结束响应
		case NAVIGATION_ACK_END:
			snprintf(ack_buffer, sizeof(ack_buffer), "#n/1*");
			break;
		
		// 导航位置响应
		case NAVIGATION_ACK_POSITION:
                        printf("已被废除的命令\n");
			snprintf(ack_buffer, sizeof(ack_buffer), "#n/2/%.4f/%.4f*", latitude, longitude);
			break;
		
		// 导航运动完成响应
		case NAVIGATION_ACK_MOVE:
			snprintf(ack_buffer, sizeof(ack_buffer), "#n/3*");
			break;
		default:
			return;
	}
	
	// 发送ACK应答（这里使用USART1发送给树莓派）
	send_string_in_specific_way(USART_WAY, USART1, ack_buffer);
}

// 接受运动命令，这个函数只接受，不执行
void Navigation_ReceiveMoveInfo(char* received_data_buffer)
{
	char* token;
	char buffer[50];
	
	// // 复制数据到临时缓冲区
	// strncpy(buffer, received_data_buffer, sizeof(buffer));
	// buffer[sizeof(buffer)-1] = '\0';
	
	// 解析运动命令格式：@n/T/左速度/右速度/时间*
	token = strtok(buffer, "/");
	if(token == NULL) return;
	
	// 跳过命令类型'n'
	token = strtok(NULL, "/");
	if(token == NULL || token[0] != NAVIGATION_MOVE) return;
	
	// 获取左电机速度
	token = strtok(NULL, "/");
	if(token != NULL) {
		navigation_info.left_motor_speed = atoi(token);
	}
	
	// 获取右电机速度
	token = strtok(NULL, "/");
	if(token != NULL) {
		navigation_info.right_motor_speed = atoi(token);
	}
	
	// 获取运动时间
	token = strtok(NULL, "/");
	if(token != NULL) {
		navigation_info.move_duration = atoi(token);
	}  
	
	// 设置状态为执行运动
	navigation_info.state = NAVIGATION_EXECUTING_MOVE;
}

void Navigation_UpdateLocation(){
		
	// 检查GPS数据是否准备好
	if (gps_handler.state == GPS_COMPLETE && gps_handler.data_ready) {
		
		// 解析GPS数据获取经纬度
		GPS_Parse(&gps_handler);
		
		// 检查GPS数据有效性（A表示有效定位，V表示无效定位）
		if (gps_handler.status == 'A') {
			// 将字符串格式的经纬度转换为浮点数
			// 纬度格式：ddmm.mmmm（度分格式）
			char lat_deg_str[3] = {0};
			char lat_min_str[10] = {0};
			
			// 提取纬度度和分
			strncpy(lat_deg_str, gps_handler.latitude, 2);
			strncpy(lat_min_str, gps_handler.latitude + 2, 7);
			
			float lat_deg = atof(lat_deg_str);
			float lat_min = atof(lat_min_str);
			
			// 将度分格式转换为十进制度格式
			navigation_info.current_latitude = lat_deg + (lat_min / 60.0f);
			
			// 如果是南纬，需要取负数
			if (gps_handler.north_south == 'S') {
				navigation_info.current_latitude = -navigation_info.current_latitude;
			}
			
			// 经度格式：dddmm.mmmm（度分格式）
			char lon_deg_str[4] = {0};
			char lon_min_str[10] = {0};
			
			// 提取经度度和分
			strncpy(lon_deg_str, gps_handler.longtitude, 3);
			strncpy(lon_min_str, gps_handler.longtitude + 3, 7);
			
			float lon_deg = atof(lon_deg_str);
			float lon_min = atof(lon_min_str);
			
			// 将度分格式转换为十进制度格式
			navigation_info.current_longitude = lon_deg + (lon_min / 60.0f);
			
			// 如果是西经，需要取负数
			if (gps_handler.east_west == 'W') {
				navigation_info.current_longitude = -navigation_info.current_longitude;
			}		
			
			// 标记GPS数据已处理
			gps_handler.data_ready = 0;
			gps_handler.state = GPS_IDLE;
		} else {
		   
			// 如果GPS定位无效，直接返回，不更新数据
			return;
//			navigation_info.current_latitude = simulated_latitude;
//			navigation_info.current_longitude = simulated_longitude;
		}
	} else {
		// GPS数据未准备好，直接返回，不更新数据
		return;
		
//		navigation_info.current_latitude = simulated_latitude;
//		navigation_info.current_longitude = simulated_longitude;
	}
 
}

// 执行运动命令，先接受(ReceiveMoveInfo)再执行
void Navigation_Execute()
{
	if(navigation_info.state != NAVIGATION_EXECUTING_MOVE) {
		return;
	}
	
	// 设置电机速度
	// 左前轮和左后轮速度相同
	motor_information.motor_duty_cycle[MOTOR_LEFT_FORWARD] = 
		(navigation_info.left_motor_speed >= 0) ? 
		navigation_info.left_motor_speed : -navigation_info.left_motor_speed;
	
	motor_information.motor_duty_cycle[MOTOR_LEFT_BACKED] = 
		motor_information.motor_duty_cycle[MOTOR_LEFT_FORWARD];
	
	// 右前轮和右后轮速度相同
	motor_information.motor_duty_cycle[MOTOR_RIGHT_FORWARD] = 
		(navigation_info.right_motor_speed >= 0) ? 
		navigation_info.right_motor_speed : -navigation_info.right_motor_speed;
	
	motor_information.motor_duty_cycle[MOTOR_RIGHT_BACKED] = 
		motor_information.motor_duty_cycle[MOTOR_RIGHT_FORWARD];
	
	// 设置电机方向
	motor_information.motor_direction[CAR_LEFT_MOTORS] = 
		(navigation_info.left_motor_speed >= 0) ? 
		MOTOR_FORWARD_DIRECTION : MOTOR_BACKED_DIRECTION;
	
	motor_information.motor_direction[CAR_RIGHT_MOTORS] = 
		(navigation_info.right_motor_speed >= 0) ? 
		MOTOR_FORWARD_DIRECTION : MOTOR_BACKED_DIRECTION;
	
	// 设置PID目标速度
	for(int i = 0; i < MOTOR_NUMBER; i++) {
		pid_struct[i].target = get_target_motor_speed(
			motor_information.motor_duty_cycle[i]);
		target_motor_speed_setting(DEFAULT_STATUS, i, motor_information.motor_duty_cycle[i]);
	}
	
   // delay(navigation_info.move_duration);
	
	// 发送运动执行ACK
	Navigation_SendAck(NAVIGATION_ACK_MOVE, 0, 0);
	
	// 运动执行完成后，等待下一个位置请求
	navigation_info.state = NAVIGATION_WAITING_FOR_POSITION;
}

void Navigation_End(void)
{
	// 停止所有电机
	for(int i = 0; i < MOTOR_NUMBER; i++) {
		motor_information.motor_duty_cycle[i] = 0;
		target_motor_speed_setting(DEFAULT_STATUS, i, 0);
	}
	
	// 恢复默认状态
	motor_information.motor_status = DEFAULT_STATUS;
	navigation_info.state = NAVIGATION_OFF;
	
	// 发送导航结束ACK应答
	Navigation_SendAck(NAVIGATION_ACK_END, 0, 0);
	

}

void Navigation_SendAck(uint8_t ack_type, float latitude, float longitude)
{
	Navigation_SendInfo(ack_type, latitude, longitude);
}


// 在main中调用，主要逻辑
void Navigation_ParseCommand(char* received_data_buffer)
{
	char* token;
	char buffer[50];
	
	if(received_data_buffer[0] != 'n') return;
	
	// 复制数据到临时缓冲区
	strncpy(buffer, received_data_buffer, sizeof(buffer));
	buffer[sizeof(buffer)-1] = '\0';
	
	token = strtok(buffer, "/");
	if(token == NULL) return;
	
	// 获取子命令类型
	token = strtok(NULL, "/");
	if(token == NULL) return;
	
	switch(token[0]) {
		// 开始导航
		case NAVIGATION_START:
			Navigation_Begin();
			break;
			
		// 结束导航
		case NAVIGATION_END:
			Navigation_End();
			break;
		
		// 树莓派请求位置信息，单片机发送位置信息(已被废除,改为定时间发送当前小车位置)	
		// case NAVIGATION_POSITION:
		// 	// 更新当前GPS位置
		// 	Navigation_UpdateLocation();
			
		// 	// 发送位置信息ACK
		// 	Navigation_SendAck(NAVIGATION_ACK_POSITION, 
		// 					 navigation_info.current_latitude,
		// 					 navigation_info.current_longitude);
		// 	break;
			
		// 执行运动
		case NAVIGATION_MOVE:
			Navigation_ReceiveMoveInfo(received_data_buffer);
			break;
			
	}
}

bool Navigation_CheckPositionReached(void)
{
	// 简化的位置到达检查
	// 在实际应用中应该计算实际距离和角度差
	float lat_diff = navigation_info.target_latitude - navigation_info.current_latitude;
	float lon_diff = navigation_info.target_longitude - navigation_info.current_longitude;
	
	// 距离计算（实际应该使用Haversine公式）
	float distance = sqrtf(lat_diff * lat_diff + lon_diff * lon_diff);
	
	return (distance <= TOLERANCE_DISTANCE);
}

void Navigation_CalculateMoveParameters(void)
{

	
}

/*


		// 接收并处理树莓派命令
		if(receive_finished_status == 1){
			// 检查是否为导航命令
			if(usart1_received_data[0] == 'n') {
				Navigation_ParseCommand(usart1_received_data);
			} else {
				// 解析其他命令(包括清除接收数据的缓冲区)
				parse_usart1_command(usart1_received_data_buffer);
			}
			
			// 清空接收的数据
			memset(usart1_received_data, 0, sizeof(usart1_received_data));
			// 清空接受的缓冲区
			memset(usart1_received_data_buffer, 0, sizeof(usart1_received_data_buffer));
			// 清空命令长度
			command_length = 0;
			// 重置接收状态
			receive_finished_status = 0;
		}


if(navigation_info.state == NAVIGATION_EXECUTING_MOVE) {
			// 记录运动开始时间
			if(navigation_move_start_time == 0) {
				navigation_move_start_time = systick_counter;
				Navigation_Execute();
			}
			
			// 检查运动是否完成
			if((systick_counter - navigation_move_start_time) >= (navigation_info.move_duration * 1000)) {
				// 运动完成，停止电机
				for(int i = 0; i < MOTOR_NUMBER; i++) {
					target_motor_speed_setting(i, 0);
				}
				navigation_move_start_time = 0;
				navigation_info.state = NAVIGATION_WAITING_FOR_POSITION;
				printf("Navigation move completed\n");
			}
		}
*/

