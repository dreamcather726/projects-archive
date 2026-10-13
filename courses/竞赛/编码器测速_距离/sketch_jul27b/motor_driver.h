#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include <Arduino.h>

// 电机允许的最大PWM，正数表示正转，负数表示反转
const int MOTOR_MAX_PWM = 150;

// 初始化电机引脚
void motor_init();

// 设置左右电机PWM，范围为-150~150
void motor_set_pwm(int pwmA, int pwmB);

#endif
