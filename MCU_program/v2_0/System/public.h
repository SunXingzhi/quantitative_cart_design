#ifndef __PUBLIC_H__
#define	__PUBLIC_H__
#include "stm32f10x.h"
#include <stdio.h>
#include "stm32f10x_conf.h"
int	fputc(int ch, FILE* f);

// IO 操作
void set_pin(GPIO_TypeDef* GPIO_x, uint16_t GPIO_Pin);
void reset_pin(GPIO_TypeDef* GPIO_x, uint16_t GPIO_Pin);
void IO_init(GPIO_TypeDef* GPIO_x, GPIO_InitTypeDef* GPIO_struct);

// usart 重定义
void ras_pi_usart_send_data(uint16_t data);
void gps_usart_send_data(uint16_t data);
void printf_usart_send_data(uint16_t data);

#endif
