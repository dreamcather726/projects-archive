#ifndef LINE_SENSOR_H
#define LINE_SENSOR_H

#include <Arduino.h>

// 引脚定义
#define SDA_PIN 8
#define SCL_PIN 9

// I2C时序参数
#define I2C_DELAY_US 10
#define I2C_TIMEOUT_US 5000

// 循迹模块参数
#define LINE_ADDR    0x27
#define CMD_READ     0x01
#define DATA_LEN     32
#define THRESHOLD    100
#define HIGH_IS_ONE  true

// 读取8路原始模拟值+开关量
bool readLineSensor(uint16_t adcBuf[8], uint8_t stateBuf[8]);

// 探测模块是否在线
bool sensorProbe();

// 总线复位修复
void recoverI2CBus();

#endif