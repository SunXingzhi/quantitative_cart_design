/* 	@file
 *	@brief 
 */
#ifndef __COMMUNICATION_H__
#define	__COMMUNICATION_H__

#include "stm32f10x.h"
// USART
#define	RASPBERRY_SERIAL	USART1
#define	RASPBERRY_SERIAL_GPIO	GPIOA
#define	RASPBERRY_SERIAL_PIN_RX	GPIO_Pin_10
#define	RASPBERRY_SERIAL_PIN_TX	GPIO_Pin_11

#define	GPS_PART_SERIAL		USART2
#define	GPS_PART_SERIAL_GPIO_RX	GPIOA
#define	GPS_PART_SERIAL_PIN_RX	GPIO_Pin_3
#define	GPS_PART_SERIAL_GPIO_TX	GPIOA
#define	GPS_PART_SERIAL_PIN_TX	GPIO_Pin_9

#define	DEBUG_SERIAL		USART3
#define	DEBUG_SERIAL_GPIO	GPIOB
#define	DEBUG_SERIAL_PIN_RX	GPIO_Pin_11
#define	DEBUG_SERIAL_PIN_TX	GPIO_Pin_10

// 定义发送方式句柄
typedef enum communication_way{
	
	USART_WAY	= 0,
	SOCKET_WAY,
	I2C_WAY,
	CAN_WAY,
	BLUETOOTH_WAY
} COMMUNICATION_WAY;



void	USART_config(void);		// USART1 配置
void	GPS_USART_config(void);		//GPS 串口配置
void	USART_send_string(USART_TypeDef* USARTx, char* data_string);	// 串口发送字符串
void	parse_usart1_command(char* receive_data_buffer);		// 解析串口命令:电机速度
void	send_string_in_specific_way(COMMUNICATION_WAY way, void* device_address, char* data_string);
void	send_formatted_string_in_specific_way(COMMUNICATION_WAY way, void* device_address, const char* format, ...);	// 支持格式化字符串的发送函数

// interrupt

#endif


