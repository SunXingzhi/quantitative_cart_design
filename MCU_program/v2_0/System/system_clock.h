#ifndef __SYSTEM_CLOCK_H__
#define	__SYSTEM_CLOCK_H__
#include "stm32f10x.h"

// ========================== System clock part ==========================
#define	APB2_PERIPHAL_CLOCK		RCC_APB2Periph_GPIOA	|	\
					RCC_APB2Periph_AFIO	|	\
					RCC_APB2Periph_USART1	|	\
					RCC_APB2Periph_GPIOB		
					
	
#define	APB1_PERIPHAL_CLOCK		RCC_APB1Periph_USART3	|	\
					RCC_APB1Periph_TIM3	|	\
					RCC_APB1Periph_TIM4	|	\
					RCC_APB1Periph_TIM2	|	\
					RCC_APB1Periph_USART2



// 配置时钟
void RCC_PLL_config(void);
void SysTick_config(void);

#endif


