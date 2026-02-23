#include "public.h"

// printf重定向->USART3: PB10(TX) PB11(RX)
int fputc(int ch, FILE* f){
	USART_SendData(USART3, (uint8_t)ch);
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE)==RESET);
	return ch;
}

// IO 操作
void set_pin(GPIO_TypeDef* GPIO_x, uint16_t GPIO_Pin)
{
	GPIO_SetBits(GPIO_x, GPIO_Pin);
}

void reset_pin(GPIO_TypeDef* GPIO_x, uint16_t GPIO_Pin)
{
	GPIO_ResetBits(GPIO_x, GPIO_Pin);
}

void IO_init(GPIO_TypeDef* GPIO_x, GPIO_InitTypeDef* GPIO_struct)
{
	GPIO_Init(GPIO_x, GPIO_struct);
}

// usart 重定义
void ras_pi_usart_send_data(uint16_t data)
{
	USART_SendData(USART1, data);
}
void gps_usart_send_data(uint16_t data)
{
	USART_SendData(USART2, data);
}
void printf_usart_send_data(uint16_t data)
{
	USART_SendData(USART3, data);
}


