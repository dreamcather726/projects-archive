#ifndef SPEED_PID_H
#define SPEED_PID_H

#include <Arduino.h>

// PID运行时保存的参数和历史数据
typedef struct {
  // PID参数
  float P;
  float I;
  float D;

  // 输出限幅
  float OutputMax;
  float OutputMin;

  // 历史误差
  float LastError;
  float PrevError;

  // 输出值
  float Output;
} PID;

// 函数声明
void PID_Init(PID *pid, float Kp, float Ki, float Kd,
              float maxValue, float minValue);
void PID_Clear(PID *pid);
float PID_IncPIDCal(PID *pid, float NowValue, float AimValue);

#endif

