#ifndef __DELAY_H
#define __DELAY_H

#include <stdint.h>

extern volatile uint32_t systick_counter;  // 系统滴答计数器（单位：1ms）

void delay_init(void);          // 初始化 SysTick（1ms 中断）
void delay_ms(uint32_t ms);     // 毫秒延时（阻塞）
void delay_us(uint32_t us);     // 微秒延时（阻塞，基于循环，需校准）

#endif
