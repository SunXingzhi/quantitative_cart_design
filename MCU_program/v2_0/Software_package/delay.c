#include "delay.h"
#include "stm32f10x.h"

// 系统滴答计数器（由 SysTick 每 1ms 增加）
volatile uint32_t systick_counter = 0;

/**
 * @brief 初始化 SysTick 为 1ms 中断
 */
void delay_init(void)
{
	// 配置 SysTick：每 1ms 中断一次
	if (SysTick_Config(SystemCoreClock / 1000)) {
		while (1); // 错误处理
	}
}

/**
 * @brief 毫秒级延时（基于 SysTick）
 */
void delay_ms(uint32_t ms)
{
	if (ms == 0) return;
	uint32_t start = systick_counter;
	while ((systick_counter - start) < ms);
}

/**
 * @brief 微秒级延时（基于空循环）
 * @note 该函数精度依赖于：
 *	   - 主频（SystemCoreClock）
 *	   - 编译器优化等级（建议使用 -O1 或 -O2）
 *	   - 是否插入 __NOP() 防止优化
 *
 * 经实测（STM32F103C8T6 @ 72MHz，Keil MDK -O1）：
 *   每次循环约消耗 8 个 CPU 周期
 *   => 1us = 72 cycles => 需要 72 / 8 = 9 次循环
 *
 * 你可以根据实际波形用示波器校准系数。
 */
void delay_us(uint32_t us)
{
	if (us == 0) return;

	// 根据主频计算循环次数
	// 默认按 72MHz 校准，若主频不同请调整
#if defined(USE_HSE_8MHz) || defined(SYSTEM_CLOCK_8MHz)
	#define CYCLES_PER_US 1   // 8MHz: 1us = 8 cycles → ~1 loop
#elif defined(SYSTEM_CLOCK_48MHz)
	#define CYCLES_PER_US 6   // 示例：48MHz
#else
	// 默认 72MHz
	#define CYCLES_PER_US 9   // 72MHz 下经验值
#endif

	uint32_t count = us * CYCLES_PER_US;
	for (uint32_t i = 0; i < count; i++) {
		__NOP(); // 插入空操作，防止编译器优化掉循环
	}
}

/**
 * @brief SysTick 中断服务函数
 */
void SysTick_Handler(void)
{
	systick_counter++;
}
