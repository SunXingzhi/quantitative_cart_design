#include "system_clock.h"

// 配置时钟
void RCC_PLL_config(void)
{
	// 时钟复位
	RCC_DeInit();
	// 配置HSE
	RCC_HSEConfig(RCC_HSE_ON);
	// 等待HSE时钟开启
	RCC_WaitForHSEStartUp();
	// 配置 PLL 时钟
	RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

	// 配置 HCLK PCLK1 PCLK2 分频系数
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_PCLK1Config(RCC_HCLK_Div2);
	RCC_PCLK2Config(RCC_HCLK_Div1);
	
	// 开启 PLL 时钟
	RCC_PLLCmd(ENABLE);
	// 等待 PLL 锁定
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY)==RESET){
		// 等待 PLL 就绪
	}	
	
	// 配置系统时钟源
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	
	// 开启外设时钟
	RCC_APB2PeriphClockCmd(APB2_PERIPHAL_CLOCK, ENABLE);
	RCC_APB1PeriphClockCmd(APB1_PERIPHAL_CLOCK, ENABLE);
	
}

// 系统滴答计数器配置
void SysTick_config(void)
{
	// 配置1ms中断
	if(SysTick_Config(SystemCoreClock/1000)){
		while(1); // 配置失败处理
	}
	// 设置中断优先级 (确保低于TIM4中断)
	NVIC_SetPriority(SysTick_IRQn, 0x0F);	
}
