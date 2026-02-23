#ifndef __IT_control_H__
#define	__IT_control_H__

void	SysTick_Handler(void);
void	USART1_IRQHandler(void);	// USART1 接收中断
void	USART2_IRQHandler(void);		//USART2中断处理函数
void	TIM3_IRQHandler(void);
void	TIM4_IRQHandler(void);

#endif
