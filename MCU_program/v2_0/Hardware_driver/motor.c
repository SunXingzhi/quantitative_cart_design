#include "stm32f10x.h"                  // Device header
#include "motor.h"
#include <stdio.h>
#include "pid.h"
#include "public.h"
#include "delay.h"

// 电机检测状态
uint32_t			motor_period_times		= 0;	// 电机测量脉冲时间隔周期
volatile uint32_t		motor_first_CCR_count		= 0;	// 电机产生脉冲时的CCR值
volatile uint32_t		motor_last_CCR_count		= 0;
volatile uint8_t		motor_pulse_count		= 0;	// 计算当前电机一圈的脉冲次数
volatile uint8_t		motor_first_detection_status	= 0;	// 电机测速状态
volatile uint8_t		motor_finished_detection_status	= 0;
volatile uint16_t		motor_timer_overflow_count	= 0;	// 测量脉冲时的溢出次数
//static	uint16_t	motor_zero_pulse_times		= 0;	// 电机0脉冲次数（12次代表

extern volatile uint32_t	systick_counter;	// 系统滴答计数器
volatile uint32_t		last_capture_time[MOTOR_NUMBER] = {0};

volatile MOTOR_INFORMATION	motor_information	= {
	{
		MOTOR_FORWARD_DIRECTION, MOTOR_BACKED_DIRECTION
	}, 
	{
		MOTOR_FORWARD_DIRECTION, MOTOR_BACKED_DIRECTION
	}, 
	{0, 0, 0, 0}, 
	{0, 0, 0, 0},	
	DEFAULT_STATUS
};

static uint16_t		motors_actual_speeds[MOTOR_NUMBER]	= {0};

void motor_set_default_value(void)
{
	// reset the default value for turn direction 
//	reset_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_PIN);
//	control_car_direction(CAR_DIRECTION_FORWARD);
//	control_motors_direction(MOTOR_FORWARD_DIRECTION, CAR_LEFT_MOTORS);
//	control_motors_direction(MOTOR_FORWARD_DIRECTION, CAR_RIGHT_MOTORS);
	reset_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_LEFT_PIN);
	set_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_RIGHT_PIN);

	reset_pin(MOTOR_BRAKE_GPIO, MOTOR_BRAKE_PIN);
}


// 外部PID导入
extern	PID_TypeDef*	pid_struct;
// 电机部分初始化
// 需要确保GPIO外设时钟开启

void motor_peripheral_init(void)
{
	motor_gpio_init();
	motor_tim_init();
}

// 电机初始化
void motor_init(void)
{
	motor_peripheral_init();
	
	motor_set_default_value();
}

void motor_gpio_init(void)
{
	GPIO_InitTypeDef	gpio_tim3_pwm;
	GPIO_InitTypeDef	gpio_tim4_capture_input;	
	GPIO_InitTypeDef	gpio_motor_direction;
	GPIO_InitTypeDef	gpio_motor_brake;	// 配置芯片IO
	// 配置tim3(PWM)：复用推挽输出
	gpio_tim3_pwm.GPIO_Mode		= GPIO_Mode_AF_PP;
	gpio_tim3_pwm.GPIO_Pin		= GPIOA_MOTOR_PWM_PIN;
	gpio_tim3_pwm.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(MOTOR_GPIOA, &gpio_tim3_pwm);
	gpio_tim3_pwm.GPIO_Pin		= GPIOB_MOTOR_PWM_PIN;
	GPIO_Init(MOTOR_GPIOB, &gpio_tim3_pwm);

	// 配置tim4(IC): 上拉输入
	gpio_tim4_capture_input.GPIO_Mode	= GPIO_Mode_IPU;
	gpio_tim4_capture_input.GPIO_Pin	= INPUT_CAPTURE_PIN;
	gpio_tim4_capture_input.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(INPUT_CAPTURE_GPIO, &gpio_tim4_capture_input);
	
	// 配置电机转向IO：推挽输出
	gpio_motor_direction.GPIO_Mode	= GPIO_Mode_Out_PP;
	gpio_motor_direction.GPIO_Pin	= MOTOR_DIRECTION_PIN;
	gpio_motor_direction.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(MOTOR_DIRECTION_GPIO, &gpio_motor_direction);

	// 配置电机刹车IO：推挽输出
	gpio_motor_brake.GPIO_Mode	= GPIO_Mode_Out_PP;
	gpio_motor_brake.GPIO_Pin	= MOTOR_BRAKE_PIN;
	gpio_motor_brake.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(MOTOR_BRAKE_GPIO, &gpio_motor_brake);	
}

void motor_tim_init(void)
{
	// TIM3-电机PWM部分
	TIM_TimeBaseInitTypeDef	tim34_time_base;
	TIM_OCInitTypeDef	tim_oc_struct;
	NVIC_InitTypeDef	NVIC_InitStruct;
	// TIM2-云台控制部分
	TIM_TimeBaseInitTypeDef	tim2_time_base;
	// TIM4-电机获取电机速度部分
	TIM_ICInitTypeDef	tim4_ic_struct;
	
	// 配置 TIM2 Freq:50Hz, Duty cycle:0.5ms/20ms
	tim2_time_base.TIM_ClockDivision	= TIM_CKD_DIV1;
	tim2_time_base.TIM_CounterMode		= TIM_CounterMode_Up;
	tim2_time_base.TIM_Period		= SERVO_PULSE_MAX; // 19999
	tim2_time_base.TIM_Prescaler		= 71; // PSC+1=72分频	
	TIM_TimeBaseInit(SERVO_TIM, &tim2_time_base);
	
	// 配置 TIM3 PWM Freq: 5KHz
	// 定时器3输出时钟 = 1MHz（APB1时钟72MHz，预分频为0时不变）
	// 计数器向上计数，最大值199，PWM频率为5000Hz
	tim34_time_base.TIM_ClockDivision	= TIM_CKD_DIV1;
	tim34_time_base.TIM_CounterMode		= TIM_CounterMode_Up;
	tim34_time_base.TIM_Period		= (uint16_t)MOTOR_PULSE_MAX; // 199
	tim34_time_base.TIM_Prescaler		= 71; // PSC+1=72分频
	
	TIM_TimeBaseInit(MOTOR_TIM, &tim34_time_base);
	
	// TIM4 
	tim34_time_base.TIM_Period		= (uint16_t)MOTOR_CAPUTRE_MAX;	// 65535
	TIM_TimeBaseInit(CAPTURE_TIM, &tim34_time_base);

	// TIM2 PWM模式
	tim_oc_struct.TIM_OCMode	= TIM_OCMode_PWM1;		// 当计数器 < Pulse时输出高电平
	tim_oc_struct.TIM_OCPolarity	= TIM_OCPolarity_High;		// 有效电平为高
	tim_oc_struct.TIM_OutputState	= TIM_OutputState_Enable;	// 使能输出
	tim_oc_struct.TIM_Pulse		= SERVO_INIT_ANGLE;		// 初始占空比
	TIM_OC1Init(SERVO_TIM, &tim_oc_struct);
	TIM_OC2Init(SERVO_TIM, &tim_oc_struct);

	// TIM3 PWM模式
//	tim_oc_struct.TIM_Pulse		= (uint16_t)MOTOR_INIT_SPEED;	// 初始占空比
	tim_oc_struct.TIM_Pulse		= (uint16_t)MOTOR_INIT_SPEED_ZERO;	// 初始占空比
	TIM_OC1Init(MOTOR_TIM, &tim_oc_struct);
	TIM_OC2Init(MOTOR_TIM, &tim_oc_struct);
	TIM_OC3Init(MOTOR_TIM, &tim_oc_struct);
	TIM_OC4Init(MOTOR_TIM, &tim_oc_struct);
	
	// TIM3 使能预装载寄存器
	TIM_OC1PreloadConfig(MOTOR_TIM, TIM_OCPreload_Enable);	// 使能CH1输出比较预装载
	TIM_ARRPreloadConfig(MOTOR_TIM, ENABLE);			// 使能自动重装载预装载

	// TIM2 使能预装载寄存器
	TIM_OC1PreloadConfig(SERVO_TIM, TIM_OCPreload_Enable);	// 使能CH1输出比较预装载
	TIM_ARRPreloadConfig(SERVO_TIM, ENABLE);			// 使能自动重装载预装载
	
	// TIM3 TIM2 启动定时器和PWM输出
	TIM_Cmd(SERVO_TIM, ENABLE);
	TIM_Cmd(MOTOR_TIM, ENABLE);
	
	// TIM_CCxCmd(TIM3, TIM_Channel_1, TIM_CCx_Enable);不需要增加，已经隐式开启PWM

	// 配置输入捕获模式
	tim4_ic_struct.TIM_Channel	= TIM_Channel_1;
	tim4_ic_struct.TIM_ICFilter	= 0xFF;
	tim4_ic_struct.TIM_ICSelection	= TIM_ICSelection_DirectTI;
	tim4_ic_struct.TIM_ICPrescaler	= TIM_ICPSC_DIV1;
	tim4_ic_struct.TIM_ICPolarity	= TIM_ICPolarity_Rising;
	TIM_ICInit(CAPTURE_TIM, &tim4_ic_struct);
	
	tim4_ic_struct.TIM_Channel	=  TIM_Channel_2;
	TIM_ICInit(CAPTURE_TIM, &tim4_ic_struct);
	
	tim4_ic_struct.TIM_Channel	=  TIM_Channel_3;
	TIM_ICInit(CAPTURE_TIM, &tim4_ic_struct);
	
	tim4_ic_struct.TIM_Channel	=  TIM_Channel_4;
	TIM_ICInit(CAPTURE_TIM, &tim4_ic_struct);

	// 6. 配置 NVIC
	NVIC_InitStruct.NVIC_IRQChannel				= TIM4_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority	= 1;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority		= 0;
	NVIC_InitStruct.NVIC_IRQChannelCmd			= ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	// TIM4中断使能(4个通道+重载值溢出）
	TIM_ITConfig(CAPTURE_TIM, TIM_IT_Update, ENABLE);
	TIM_ITConfig(CAPTURE_TIM, TIM_IT_CC1, ENABLE);
	TIM_ITConfig(CAPTURE_TIM, TIM_IT_CC2, ENABLE);
	TIM_ITConfig(CAPTURE_TIM, TIM_IT_CC3, ENABLE);
	TIM_ITConfig(CAPTURE_TIM, TIM_IT_CC4, ENABLE);
	
	// 启动TIM4
	TIM_Cmd(CAPTURE_TIM, ENABLE);
}

// 电机获取速度
// 这里的逻辑有问题，传入为单个通道，则输出应该也为单个通道
uint16_t* get_motor_speed(uint8_t motor_ch) 
{
	uint32_t	period_counts;
	
	uint8_t		i;
	
	period_counts	= (uint32_t)motor_last_CCR_count 
			-(uint32_t)motor_first_CCR_count 
			+(uint32_t)motor_timer_overflow_count*(MOTOR_CAPUTRE_MAX+1UL);
	
	// 数组初始化
//	motors_actual_speeds	= (uint16_t*)malloc(MOTOR_NUMBER*sizeof(motors_actual_speeds));
//	if(motors_actual_speeds==NULL){
//		printf("failed to assign the memory for motors speed array.");
//	}
//	memset(motors_actual_speeds, 0, MOTOR_NUMBER*sizeof(uint16_t));
	
	// 防除零与超范围保护
	if(period_counts<100 || period_counts>2000000UL){ // 2秒最大周期
		return NULL;
	}
	
	// 查看PID数据结构是否有效
	if(pid_struct==NULL){
		printf("pid structure is null.");
	}
	
	switch(motor_ch){
		case MOTOR_CH1:
			pid_struct[0].actual	= (uint16_t)(60000000UL/period_counts);
			break;
		case MOTOR_CH2:
			pid_struct[1].actual	= (uint16_t)(60000000UL/period_counts);
			break;
		case MOTOR_CH3:
			pid_struct[2].actual	= (uint16_t)(60000000UL/period_counts);			
			break;
		case MOTOR_CH4:
			pid_struct[3].actual	= (uint16_t)(60000000UL/period_counts);			
			break;
		default:
			printf("invalid motor id");
			return	NULL;
	
	}
	
	for(i=0;i<4;i++){
		motors_actual_speeds[i]	= pid_struct[i].actual;
	}
	
	return	motors_actual_speeds;
}

// 电机获取目标速度
// 处理好映射关系。最低转速要求占空比在10%
uint16_t get_target_motor_speed(int duty_cycle)
{
	int	target_speed;
	
	target_speed	= 0;
	if(duty_cycle<MOTOR_DRIVER_MIN_DUTY_CYCLE){
		return MOTOR_DRIVER_MIN_DUTY_CYCLE;
	}
	if(duty_cycle>100){
		duty_cycle	= 100;
	}
	target_speed	= (int32_t)((duty_cycle-MOTOR_DRIVER_MIN_DUTY_CYCLE)*MOTOR_SPEED_STEP_PER_DUTY_CYCLE+MOTOR_TARGET_MIN_SPEED);
	
	return	target_speed;
}

// 获取目标电机TIM输出脉冲
#define	GET_TARGET_MOTOR_PULSE(duty_cycle)	(duty_cycle*MOTOR_PULSE_MAX)/100
uint16_t get_target_motor_pulse(uint8_t duty_cycle)
{
	if(duty_cycle>100){
		duty_cycle	= 100;
	}
//	else if(duty_cycle<0){
//		duty_cycle	= 0;
//	}
	return	GET_TARGET_MOTOR_PULSE(duty_cycle);
}

/* 设置目标电机速度
 * 使用该函数前需要确保电机方向
 */
void target_motor_speed_setting(uint8_t motors_status, uint8_t motor_ch, uint8_t duty_cycle)
{
	int32_t	pulse;
	
	// 获取占空比对应脉冲
	pulse	= 0;
	pulse	= get_target_motor_pulse(duty_cycle);
	
	if(motors_status==DEFAULT_STATUS || motors_status==IN_PLACE_TURNING_STATUS){ 
		switch(motor_ch){
			case MOTOR_CH1:
				TIM_SetCompare1(MOTOR_TIM, pulse);
				break;
			case MOTOR_CH2:
				TIM_SetCompare2(MOTOR_TIM, pulse);
				break;
			case MOTOR_CH3:
				TIM_SetCompare3(MOTOR_TIM, pulse);
				break;
			case MOTOR_CH4:
				TIM_SetCompare4(MOTOR_TIM, pulse);
				break;
			default:
				printf("Unknown motor channel!");
				break;
		}
	} else{
		// 执行相关警告逻辑
	}
	
	return;
}

uint16_t* get_target_motors_speed()
{
	return	NULL;
}

// 检测电机是否转动函数
//#define	JUDGE_MOTOR_STOP(MOTOR_CH)				\
//	if((current_time-last_capture_time[MOTOR_CH])>MOTOR_STOP_TIMEOUT_MS){	\
//		motor_finished_detection_status	= 1;		\
//		if(pid_struct!=NULL && MOTOR_CH<MOTOR_NUMBER){	\
//			pid_struct[MOTOR_CH].actual	= 0;	\
//		}						\
//								\
//		last_capture_time[MOTOR_CH]	= current_time;	\
//		printf("Motor stop detected\n");		\
//	}							\
	
void check_motor_stop(int16_t motor_id) 
{
	static uint32_t	last_check_time;
	uint32_t	current_time;
	
	current_time	= systick_counter;
	last_check_time	= 0;
	if((current_time-last_check_time)<100)  // 每100ms检查
		return;
	last_check_time	= current_time;
	
}

// 控制电机刹车-需要参考模拟开关的高电平有效/低电平有效
// 不建议应用层使用该函数!!
void control_motors_brake(bool state)
{
	// 设置电机速度为0
	if(state){
		printf("Open the Brake\n,");
		// 开启电磁制动器
		set_pin(MOTOR_BRAKE_GPIO, MOTOR_BRAKE_PIN);	
		// 测试低电平是否可以松开制动器
//		reset_pin(MOTOR_BRAKE_GPIO, MOTOR_BRAKE_PIN);
	} else{
		printf("Close the Brake\n,");
		// 关闭电磁制动器
		reset_pin(MOTOR_BRAKE_GPIO, MOTOR_BRAKE_PIN);
	}
}

/* 控制刹车,传参有两个:
 * ONLY_STOP_MOTORS:	仅停止电机
 * WITH_OPEN_BRAKE:	停止电机的同时开启制动器
 */
void car_stop(int8_t additional_operation)
{
//	printf("%s, %d", __FILE__, __LINE__);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH1, 0);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH2, 0);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH3, 0);
	target_motor_speed_setting(DEFAULT_STATUS, MOTOR_CH4, 0);

	if(additional_operation==BY_BRAKES_BUT_LOCKED){
		// 开启制动器
		control_motors_brake(true);
		// 先使用delay实现电机停止,后续再使用事件触发
//		delay_ms(500);
	} 
	else if(additional_operation==WITHOUT_BRAKES){
		return;
	} 
	else if(additional_operation==BY_BRAKES_BUT_ACTIVE){
		control_motors_brake(true);
		delay_us(1000);
		control_motors_brake(false);
	}

}

// 检查两侧电机是否发生变化
// 默认传参数组为2个数据(代表车的两侧电机),否则无效
bool*	check_site_motors_direction(bool* change_status_buffer, int8_t* last_motor_direction, volatile int8_t* motor_direction)
{
	int	i;
	if(last_motor_direction==NULL || motor_direction==NULL){
		printf("invalid motor_direction");
		return NULL;
	}
	
	for(i=0; i<2; i++){
		change_status_buffer[i]	= false;
	}
	i	= 0;
	
	for(i=0;i<2;i++){
		if(last_motor_direction[i]!=motor_direction[i]){
			change_status_buffer[i]	= true;
			printf("direction%d changed.\n", i);
		} else{
			change_status_buffer[i]	= false;
		}
	}
	return change_status_buffer;
}


/* 控制无刷电机正反转-控制继电器短接2个引脚，需要设置互斥锁--不能同时接通两路继电器
 * 使用单刀双掷开关芯片，IO低电平正向，高电平反向.
 * 控制转向前一定要确定方向与速度.
 * 方向这里默认认为是改变的,因为使用该函数的地方一定是有电机转向需求,无需开启制动器;
 * 如果方向相反/有PWM,一定要先停止电机,开启制动,等待电机速度为0时才转向.
 * motor_site: CAR_LEFT_MTOROS(0) or CAR_RIGHT_MOTORS(1)
 */
void control_motors_direction(int8_t motor_direction, int8_t motor_site)
{
	car_stop(BY_BRAKES_BUT_ACTIVE);
	printf("Already brake.\n");
	if(motor_site==CAR_LEFT_MOTORS){
		if(motor_direction==MOTOR_FORWARD_DIRECTION){
			reset_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_LEFT_PIN);			
		}
		else if(motor_direction==MOTOR_BACKED_DIRECTION){
			set_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_LEFT_PIN);			
		}
	}
	else if(motor_site==CAR_RIGHT_MOTORS){
		if(motor_direction==MOTOR_FORWARD_DIRECTION){
			set_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_RIGHT_PIN);			
		
		}
		else if(motor_direction==MOTOR_BACKED_DIRECTION){
			reset_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_RIGHT_PIN);
		}		
	} else{
		printf("Invalid motor site.");
	}
	
}

// 控制车辆行驶方向
void control_car_direction(int8_t car_direction)
{
//	printf("%s, %d", __FILE__, __LINE__);
	switch(car_direction){
		case CAR_DIRECTION_FORWARD:
			control_motors_brake(true);
			break;
		case CAR_DIRECTION_BACKWARD:
			control_motors_brake(true);
			reset_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_RIGHT_PIN);
			set_pin(MOTOR_DIRECTION_GPIO, MOTOR_DIRECTION_CONTROL_LEFT_PIN);			
			break;			
		default:
			// 电机停止
//			car_stop();
			printf("car direction error:%d.\n", car_direction);
			break;
	}
}



// 可以增加条件编译保证时钟启动正确
void servo_init(void)
{
	GPIO_InitTypeDef	gpio_tim2_pwm;
	// 配置tim2(PWM): 复用推挽输出
	gpio_tim2_pwm.GPIO_Mode		= GPIO_Mode_AF_PP;
	gpio_tim2_pwm.GPIO_Pin		= GPIO_SERVO_PWM_PIN;
	gpio_tim2_pwm.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_Init(SERVO_GPIO, &gpio_tim2_pwm);
	
	// 增加默认值
}

// 改变舵机角度
void servo_angle_change(uint16_t angle1,uint16_t angle2)
{
		if (angle1 > 180) angle1 = 180;			   
		uint16_t pulse1 = 500 + (angle1 * 2000) / 180; 
		TIM_SetCompare1(TIM2, pulse1);
		
		if (angle2 > 180) angle2 = 180;			   
		uint16_t pulse2 = 500 + (angle2 * 2000) / 180; 
		TIM_SetCompare2(TIM2, pulse2);				
}

