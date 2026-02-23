#include "stm32f10x.h"
#include "IT_control.h"
#include "communication.h"
#include "motor.h"
#include "gps.h"
#include "delay.h"

// Global variable configuration
extern char		usart1_received_data[50];
extern volatile char	usart1_received_byte;
extern volatile char*	usart1_latest_command;
//char			usart1_received_data_buffer[50];
extern volatile char	command_length;
extern uint8_t		Rx_state;			
extern uint8_t 		write_position;	
extern volatile uint8_t	receive_finished_status;
extern uint8_t		Serial_RxFlag;				//定义接收数据包标志位

extern	uint32_t 		motor_period_times;
extern	uint32_t 		motor_first_CCR_count;
extern	uint32_t		motor_last_CCR_count;
extern	volatile uint8_t	motor_pulse_count;
extern	volatile uint8_t	motor_first_detection_status;
extern	volatile uint8_t	motor_finished_detection_status;
extern	volatile uint16_t	motor_timer_overflow_count;
//extern	volatile uint32_t	systick_counter;	// 系统滴答计数器
extern	volatile uint32_t	last_capture_time[MOTOR_NUMBER];

//volatile uint32_t		systick_counter	= 0;	// 系统滴答计数器

// USART1 debug 接收中断
void USART1_IRQHandler(void)
{	
	// 这里先读取SR寄存器，在读取DR寄存器，清除OER位(仅限未使能ERR位时）
	// 为了清除溢出错误中断
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE)!=RESET){
		USART_ReceiveData(USART1);
		Rx_state = 0;			  // 重置状态机
		write_position = 0;		// 重置写指针
		return;
	}
	receive_finished_status	= 0;
	if(USART_GetITStatus(USART1, USART_IT_RXNE)==SET){
		uint8_t RxData = USART_ReceiveData(USART1);
		
		if(Rx_state==0){
			if(RxData=='@'){
				Rx_state = 1;
				write_position = 0; // 准备接收数据
			}
			// 否则忽略，继续等待 '@'
		}
		else if(Rx_state==1){
			if (RxData=='*'){
				// 包结束, 命令末尾添加 '\0'
				if(write_position<sizeof(usart1_received_data)-1){
					usart1_received_data[write_position] = '\0';
//					USART_send_string(USART1, &usart1_received_data[0]);
//					printf("command:%s\n", usart1_received_data);
					command_length = write_position; // 或 write_position + 1
//					parse_usart1_command(); // 解析命令

					receive_finished_status	= 1;
				}else{
					printf("ERROR:OVERFLOW\n");
					return;
				}
//				// 清空接收的数据
//				memset(usart1_received_data, 0, sizeof(usart1_received_data));
//				// 清空接受的缓冲区
//				
//				// 清空写指针和命令长度
//				printf("com_length: %d, write_position:%d\n", command_length, write_position);
//				command_length	= 0;
				write_position	= 0;
				Rx_state = 0; // 重置状态，等待下一个包
				
			}
			else{
				// 接收数据
				if(write_position<sizeof(usart1_received_data)-1){
					usart1_received_data[write_position] = RxData;
					write_position++;
				}
				// 否则串口发送失败信息
				else{
					printf("ERROR: index is more than the data array's length\n");
					
				}
			}
		}
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

// USART2接收数据中断处理函数
// 识别状态，将数据存入缓存区
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2,USART_IT_RXNE)!=0)
	{
		uint8_t		received_char;
		received_char	= USART2->DR;		//中断标志已清除
		
		//获取GPS结构体
		extern GPS_Handler_t gps_handler;
		
		//判断接收数据状态
		switch(gps_handler.state){
			case GPS_IDLE:
				//接收数据为空闲状态时
				if(received_char=='$'){
					gps_handler.raw_buffer[0]	= received_char;
					gps_handler.write_index	= 1;
					gps_handler.state		= GPS_RECEIVING;
				}
				break;
				
			case GPS_RECEIVING:
				//接收状态为接受中时
				if(received_char!='*'&&(gps_handler.write_index<GPS_BUFFER_SIZE-1)){
					//将数据存入缓冲区
					gps_handler.raw_buffer[(gps_handler.write_index)++]	= received_char;
					
				}else if(gps_handler.write_index>=GPS_BUFFER_SIZE){
					//缓冲区溢出时
					gps_handler.write_index	= 0;
					gps_handler.state	= GPS_IDLE;	//转换状态至空闲
				}else{
					//接收完成，状态切换为接收完成
					gps_handler.state	= GPS_COMPLETE;
					gps_handler.write_index	= 0;
				}
				break;
				
			case GPS_COMPLETE:
				//接收状态为接受完成
				break;
				
		}
	}
}

// TIM3	中断
void TIM3_IRQHandler(void)
{
	// 清除中断标志位
	if(TIM_GetITStatus(TIM3, TIM_IT_CC1)!=RESET){
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
		// 中断处理
	}
}

// 捕获电机速度中断
void TIM4_IRQHandler(void)
{
	uint16_t	current_value;
	uint32_t	current_tick;
	
	current_tick = systick_counter;
	if(TIM_GetFlagStatus(TIM4, TIM_IT_Update)==SET){
		if(motor_first_detection_status && !motor_finished_detection_status) {
			motor_timer_overflow_count++;
		}
		TIM_ClearFlag(TIM4, TIM_IT_Update);
	}
	
	// 电机1
	else if(TIM_GetFlagStatus(TIM4, TIM_IT_CC1)==SET){
		current_value			= TIM_GetCapture1(TIM4);
		last_capture_time[MOTOR_CH1]	= current_tick;  // 更新最后捕获时间
		
		if(!motor_first_detection_status){
			motor_first_CCR_count		= current_value;
			motor_first_detection_status	= 1;
			motor_timer_overflow_count	= 0;
			motor_pulse_count		= 1;
		} 
		else if(!motor_finished_detection_status){
			motor_pulse_count++;
			if(motor_pulse_count>=MOTOR_ENCODER_PULSE){
				motor_last_CCR_count		= current_value;
				motor_finished_detection_status	= 1;
			}
		}
		TIM_ClearFlag(TIM4, TIM_IT_CC1);
	}
	else if(TIM_GetFlagStatus(TIM4, TIM_IT_CC2)==SET){
		TIM_ClearFlag(TIM4, TIM_IT_CC2);
	}
	else if(TIM_GetFlagStatus(TIM4, TIM_IT_CC3)==SET){
		TIM_ClearFlag(TIM4, TIM_IT_CC3);
	}
	else if(TIM_GetFlagStatus(TIM4, TIM_IT_CC4)==SET){
		TIM_ClearFlag(TIM4, TIM_IT_CC4);
	}
 
}

