#include "system_config.h"
#include "system_clock.h"
#include "motor.h"
#include "pid.h"
#include "gps.h"
#include "OLED.h"
#include "delay.h"
extern	uint32_t 		motor_period_times;
extern	uint32_t 		motor_first_CCR_count;
extern	uint32_t		motor_last_CCR_count;
extern	volatile uint8_t	motor_pulse_count;
extern	volatile uint8_t	motor_first_detection_status;
extern	volatile uint8_t	motor_finished_detection_status;
extern	volatile uint16_t	motor_timer_overflow_count;
extern	volatile uint32_t	systick_counter;	// 系统滴答计数器
extern	volatile uint32_t	last_capture_time[MOTOR_NUMBER];

extern	PID_TypeDef*		pid_struct;


// GPIO 配置
void GPIO_config(void)
{
	GPIO_InitTypeDef	gpio_LED;


	GPIO_InitTypeDef	gpio_usart_struct;

	
	// pwm输入引脚
	gpio_LED.GPIO_Mode	= GPIO_Mode_Out_PP;
//	gpio_LED.GPIO_Mode	= GPIO_Mode_Out_PP;
	gpio_LED.GPIO_Pin	= GPIO_Pin_1 | GPIO_Pin_2;	
	gpio_LED.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_LED);
	
	// usart1引脚：配置为复用推挽输出模式
	gpio_usart_struct.GPIO_Mode	= GPIO_Mode_AF_PP;
	gpio_usart_struct.GPIO_Pin	= GPIO_Pin_9;
	gpio_usart_struct.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio_usart_struct);				// PA9 复用推免
	gpio_usart_struct.GPIO_Mode	= GPIO_Mode_IN_FLOATING;	// PA10 浮空输入
	gpio_usart_struct.GPIO_Pin	= GPIO_Pin_10;
	GPIO_Init(GPIOA, &gpio_usart_struct);	
	
	// usart2引脚：配置为上拉输入模式
	gpio_usart_struct.GPIO_Mode	= GPIO_Mode_IPU; 
	gpio_usart_struct.GPIO_Pin	= GPIO_Pin_3;
	GPIO_Init(GPIOA, &gpio_usart_struct);
	
	// usart3引脚：配置为复用推挽输出模式
	gpio_usart_struct.GPIO_Mode	= GPIO_Mode_AF_PP;
	gpio_usart_struct.GPIO_Pin	= GPIO_Pin_10;
	GPIO_Init(GPIOB, &gpio_usart_struct);				// PB10 复用推免
	gpio_usart_struct.GPIO_Mode	= GPIO_Mode_IN_FLOATING;	// PA11 浮空输入
	gpio_usart_struct.GPIO_Pin	= GPIO_Pin_11;
	GPIO_Init(GPIOB, &gpio_usart_struct);
}

// USART 配置
void USART_config(void)
{
	USART_InitTypeDef	usart_struct;
	NVIC_InitTypeDef	nvic_usart1_struct;
	
	// 配置串口(使用APB2 的 USART1)
	USART_DeInit(USART1);
	USART_DeInit(USART3);
	usart_struct.USART_BaudRate		= 9600;
	// 轮询机制，不需要硬件流控制
	usart_struct.USART_Mode			= USART_Mode_Tx | USART_Mode_Rx;
	usart_struct.USART_Parity		= USART_Parity_No;
	usart_struct.USART_StopBits		= USART_StopBits_1;
	usart_struct.USART_WordLength		= USART_WordLength_8b;
	usart_struct.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	
	USART_Init(USART1, &usart_struct);
	USART_Init(USART3, &usart_struct);
	
	
	// USART1 接收中断配置
	nvic_usart1_struct.NVIC_IRQChannel			= USART1_IRQn;
	nvic_usart1_struct.NVIC_IRQChannelCmd			= ENABLE;
	nvic_usart1_struct.NVIC_IRQChannelPreemptionPriority	= 1;
	nvic_usart1_struct.NVIC_IRQChannelSubPriority		= 0;
	

	NVIC_Init(&nvic_usart1_struct);
	
	USART_Cmd(USART1, ENABLE);
	USART_Cmd(USART3, ENABLE);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

//USART2 配置
//接收GPS模块经纬度数据的串口配置
void USART2_Config(void)
{
	USART_InitTypeDef	usart2_struct;
	NVIC_InitTypeDef	nvic_usart2_struct;
	
	//配置USART2，接收数据，波特率9600
	usart2_struct.USART_BaudRate		= 9600;
	usart2_struct.USART_Mode		= USART_Mode_Rx;
	usart2_struct.USART_Parity		= USART_Parity_No;
	usart2_struct.USART_StopBits		= USART_StopBits_1;
	usart2_struct.USART_WordLength		= USART_WordLength_8b;
	usart2_struct.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;
	USART_Init(USART2,&usart2_struct);
	
	//配置USART中断
	nvic_usart2_struct.NVIC_IRQChannel			= USART2_IRQn;
	nvic_usart2_struct.NVIC_IRQChannelPreemptionPriority	= 0;
	nvic_usart2_struct.NVIC_IRQChannelSubPriority		= 0;
	nvic_usart2_struct.NVIC_IRQChannelCmd			= ENABLE;
	NVIC_Init(&nvic_usart2_struct);
	
	//使能USART
	USART_Cmd(USART2,ENABLE);
	
	//开启USART2接收中断
	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
}


void systick_init(void)
{
	/* SystemCoreClock / 1000    1ms中断一次
	 * SystemCoreClock / 100000  10us中断一次
	 * SystemCoreClock / 1000000 1us中断一次
	 */
 
	/* 嘀嗒定时器每计数一次为 1/72M，此处计数 72个数，即1uS中断一次 */
	if(SysTick_Config(SystemCoreClock / 1000000))	// ST V3.5.0库版本
	{ 
		/* Capture error */ 
		while(1);
	}
	
	// 关闭滴答定时器  
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;	
}



// 硬件初始化函数
void hardware_init(void)
{
//	OLED_Init();
//	OLED_ShowString(1, 1, "success");
	servo_init();
	motor_init();
	
}

// include systick, delay
void software_package_init(void)
{
	// TIM... 
	systick_init();
	
	return ;
}

// 系统初始化函数
void system_init(void)
{
	int	i;
	
	i	= 0;
	RCC_PLL_config();
//	SysTick_config();  
	delay_init();	// 代替systick_config
	GPIO_config();
	USART_config();
	//GPS接收串口配置
	USART2_Config();
	printf("USART is configured successfully!\n");	
	PID_init();	// 确保PID结构已初始化
	
	
	// 初始化最后捕获时间为当前时间 (防止启动时误判)
	for(i=0; i<MOTOR_NUMBER; i++){
		last_capture_time[i]	= systick_counter;
	}
	// 电机测速相关变量的初始化
	motor_period_times		= 0;
	motor_first_CCR_count		= 0;
	motor_last_CCR_count		= 0;
	motor_pulse_count		= 0;
	motor_first_detection_status 	= 0;
	motor_finished_detection_status = 0;
	motor_timer_overflow_count	= 0;
	
	// 初始化PID结构体的实际速度值
	if(pid_struct!=NULL){
		for(i=0; i<MOTOR_NUMBER; i++){
			pid_struct[i].actual		= 0;
			pid_struct[i].target		= 0;
			pid_struct[i].current_error	= 0;
			pid_struct[i].total_errors	= 0;
		}
	}
	
	printf("System initialization completed.\n");	
}

void NVIC_interupt_config(void)
{
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_2);
}
