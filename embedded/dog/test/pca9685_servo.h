#ifndef PCA9685_SERVO_H
#define PCA9685_SERVO_H

#include <Arduino.h>

// PCA9685 硬件配置：ESP32 通过 I2C 控制舵机驱动板。
#define PCA9685_ADDR 0x40
#define I2C_SDA 8
#define I2C_SCL 9
#define SERVO_FREQ 50

// 舵机脉宽范围，单位是 us。1500us 是程序默认中位。
#define SERVO_MIN_US 500
#define SERVO_CENTER_US 1500
#define SERVO_MAX_US 2500

void initPca9685Servo();
int clampPulseUs(int pulseUs);
int convertPulseUsToPcaCount(int pulseUs);
void writeServoPulseUs(int channel, int pulseUs);

#endif
