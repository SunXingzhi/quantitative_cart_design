#include <stdio.h>
#include <string.h>
#include "gps.h"

float latitude_f;
float longtitude_f;

//GPS数据结构初始化
void GPS_Init(GPS_Handler_t* gps)
{
	memset(gps,0,sizeof(GPS_Handler_t));
	gps->state	= GPS_IDLE;
}

//GPS数据解析，提取经纬度信息
void GPS_Parse(GPS_Handler_t* gps)
{
	
	char latitude_dd[2];
	char longtitude_ddd[3];
	char latitude_7m[7];
	char longtitude_7m[7];
	float		latitude_a;	//坐标转换的整数部分
	float		latitude_b;	//坐标转换的小数部分
	float		longtitude_a;	//坐标转换的整数部分
	float		longtitude_b;	//坐标转换的小数部分
	extern float	latitude_f;
	extern float	longtitude_f;
	
	latitude_f	= 0.0f;
	longtitude_f	= 0.0f;
		
	if(gps->state==GPS_RECEIVING||GPS_IDLE){
		//如果数据还在接受中或空闲中，退出函数
		return;
	}else if(gps->state==GPS_COMPLETE){
		//提取坐标信息
		memcpy(gps->coordinate, (gps->raw_buffer)+LATITUDE_OFFSET, GPS_COORDINATE_SIZE);
		//提取信息的有效性
		gps->status	= gps->raw_buffer[GPS_STATUS_OFFSET];
		
		//提取纬度信息 ddmm.mmmmm
		memcpy(gps->latitude, (gps->raw_buffer)+LATITUDE_OFFSET, 10);
		//提取经度信息 dddmm.mmmmm
		memcpy(gps->longtitude, (gps->raw_buffer)+LONGTITUDE_OFFSET, 11);
		//提取南北纬信息
		gps->north_south	= gps->raw_buffer[N_S_OFFSET];
		//提取东西经信息
		gps->east_west		= gps->raw_buffer[E_W_OFFSET];
		//转换纬度
		memcpy(latitude_dd, gps->latitude, 2);
		memcpy(latitude_7m, (gps->latitude)+2, 2);
		memcpy(latitude_7m+2, (gps->latitude)+5, 5);
		//转换经度
		memcpy(longtitude_ddd, gps->longtitude,3);
		memcpy(longtitude_7m, (gps->longtitude)+3, 3);
		memcpy(longtitude_7m+3, (gps->longtitude)+6, 5);
		
		//字符串转换
		latitude_a	= (float)atoi(latitude_dd);
		latitude_b	= (float)atoi(latitude_7m);
		longtitude_a	= (float)atoi(longtitude_ddd);
		longtitude_b	= (float)atoi(longtitude_7m);
		
		//计算坐标
		latitude_f	= latitude_a+(latitude_b/6000000);
		longtitude_f	= longtitude_a+(longtitude_b/6000000);
		
		
	}
}

// ============================= debug =========================
// 串口发送字符串 前提：已配置好串口
void Send_GPS_String(USART_TypeDef* USARTx, char* string, uint32_t length)		//发送字符串数组
{
	int	i;
	for(i=0;i<length;i++){
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE)==RESET);
		USART_SendData(USARTx, string[i]);
	}
}
