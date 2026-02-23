#ifndef __SYSTEM_CONFIG_H__
#define	__SYSTEM_CONFIG_H__
#include "system_clock.h"

void 	GPIO_config(void);
void 	USART_config(void);
void 	USART2_Config(void);
void	hardware_init(void);		// motor, servo, OLED...
void	software_package_init(void);
void	system_init(void);
void	NVIC_interupt_config(void);
void	systick_init(void);

#endif

