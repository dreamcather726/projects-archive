#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

// 初始化编码器引脚和中断
void encoder_init();

// 根据本次时间间隔更新速度和距离
void encoder_update(float intervalSeconds);

// 更新后直接读取下面4个变量
extern float speedA;       // 左轮速度，带正负号，单位cm/s
extern float speedB;       // 右轮速度，带正负号，单位cm/s
extern float distanceA;    // 左轮累计距离，单位cm
extern float distanceB;    // 右轮累计距离，单位cm

#endif
