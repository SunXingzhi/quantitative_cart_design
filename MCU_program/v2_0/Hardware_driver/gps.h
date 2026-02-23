#ifndef __GPS_H__
#define	__GPS_H__

#include "stm32f10x.h"
#include <stdio.h>

// ============================== GPS part =============================
//接收数据状态枚举
#define	GPS_BUFFER_SIZE		90
#define	GPS_COORDINATE_SIZE	26
#define	GPS_STATUS_OFFSET	17
#define	LATITUDE_OFFSET		19
#define	LONGTITUDE_OFFSET	32
#define	N_S_OFFSET		30
#define	E_W_OFFSET		44

//GPS接收数据状态
typedef enum{
	GPS_IDLE,
	GPS_RECEIVING,
	GPS_COMPLETE,
}GPS_State_t;

//GPS数据结构体
typedef struct{
	char			raw_buffer[GPS_BUFFER_SIZE];
	volatile uint16_t	write_index;
	volatile GPS_State_t	state;
	char			coordinate[GPS_COORDINATE_SIZE];
	char			latitude[12];
	char			longtitude[12];
	char			north_south;
	char			east_west;
	char			status;
	uint8_t			data_ready;
}GPS_Handler_t;

//浮点数联合体，分成四个字节，便于发送数据
typedef union{
	float	GPS_Data;
	uint8_t	GPS_Data_buffer[4];
}GPS_Data_union;

//函数声明
void USART2_Config(void);	//USART2配置

void GPS_Init(GPS_Handler_t* gps);	//初始化数据结构
void GPS_Parse(GPS_Handler_t* gps);	//解析定位信息
void Send32Bits(USART_TypeDef* USARTx, float GPS_Coordinate_Data);	//发送32位浮点数GPS数据
void SendChar(USART_TypeDef* USARTx, uint8_t data);		//发送单字节
void Send_GPS_String(USART_TypeDef* USARTx, char* string, uint32_t length);		//发送字符串数组


#endif
