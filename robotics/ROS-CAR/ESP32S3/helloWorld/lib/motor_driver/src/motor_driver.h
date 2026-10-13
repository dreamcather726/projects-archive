#pragma once

#include <Arduino.h>

static constexpr uint8_t RIGHT_MOTOR_PWM_PIN = 42; // Driver channel A: PWMA
static constexpr uint8_t RIGHT_MOTOR_DIR_PIN = 41; // Driver channel A: AN1
static constexpr uint8_t LEFT_MOTOR_PWM_PIN = 1;  // Driver channel B: PWMB
static constexpr uint8_t LEFT_MOTOR_DIR_PIN = 2;  // Driver channel B: BN1

static constexpr bool RIGHT_MOTOR_INVERT_DIR = false;
static constexpr bool LEFT_MOTOR_INVERT_DIR = true;
static constexpr bool RIGHT_MOTOR_REVERSE_USE_FORWARD_OUTPUT = true;
static constexpr uint32_t MOTOR_DIRECTION_SWITCH_DELAY_US = 1000U;

// 全局电机输出上限：30% 满量程。整数除法向下取整，76/255 ≈ 29.8%。
static constexpr int MOTOR_PWM_FULL_SCALE = 255;
static constexpr int MOTOR_PWM_LIMIT_PERCENT = 30;
static constexpr int MOTOR_PWM_LIMIT =
    (MOTOR_PWM_FULL_SCALE * MOTOR_PWM_LIMIT_PERCENT) / 100;

void motor_init();
void set_right_motor_pwm(int pwm);
void set_left_motor_pwm(int pwm);
