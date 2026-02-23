#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "stm32f10x.h"

#include <stdbool.h>
#include <stdlib.h>
// #include "delay.h"

// 导航相关定义
#define TOLERANCE_DISTANCE      1.0f    // 目标点距离容差 单位m
#define TOLERANCE_ANGLE         10.0f   // 目标点角度容差 单位度

// 导航命令类型
#define NAVIGATION_START        'A'
#define NAVIGATION_END          'Z'
#define NAVIGATION_POSITION     'G'
#define NAVIGATION_MOVE         'T'

// 导航应答类型
#define NAVIGATION_ACK_START    0
#define NAVIGATION_ACK_END      1
#define NAVIGATION_ACK_POSITION 2
#define NAVIGATION_ACK_MOVE     3

// 外部变量声明


// 导航状态枚举
typedef enum {
        NAVIGATION_OFF = 0,
        NAVIGATION_STARTING,
        NAVIGATION_WAITING_FOR_POSITION,
        NAVIGATION_WAITING_FOR_MOVE,
        NAVIGATION_EXECUTING_MOVE,
        NAVIGATION_FINISHING
} NavigationState;


// 导航相关结构体
typedef struct {
    float target_latitude;
    float target_longitude;
    float current_latitude;
    float current_longitude;
    int16_t left_motor_speed;
    int16_t right_motor_speed;
    uint16_t move_duration;  // 运动时间，单位秒
    NavigationState state;
} NavigationInfo;

// 函数声明
void Navigation_Begin(void);
void Navigation_SendInfo(uint8_t ack_type, float latitude, float longitude);
void Navigation_ReceiveMoveInfo(char* received_data_buffer);
void Navigation_Execute(void);
void Navigation_End(void);
void Navigation_SendAck(uint8_t ack_type, float latitude, float longitude);
void Navigation_ParseCommand(char* received_data_buffer);
bool Navigation_CheckPositionReached(void);
void Navigation_CalculateMoveParameters(void);

#endif
