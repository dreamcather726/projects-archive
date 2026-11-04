#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

inline constexpr gpio_num_t MOTOR1_PWM = GPIO_NUM_41;
inline constexpr gpio_num_t MOTOR1_DIR = GPIO_NUM_1;

inline constexpr gpio_num_t MOTOR2_PWM = GPIO_NUM_42;
inline constexpr gpio_num_t MOTOR2_DIR = GPIO_NUM_2;

#ifdef __cplusplus
extern "C" {
#endif

/** 初始化电机引脚。 */
void motor_init(void);
/** 设置电机速度。 */
/*
motor_set_speed(1, 100); // 设置电机1的速度为100
motor_set_speed(2, -100); // 设置电机2的速度为-100

*/
void motor_set_speed(int motor, int speed);
#ifdef __cplusplus
}
#endif
