#ifndef SPEED_PID_H
#define SPEED_PID_H

#include <Arduino.h>

// 增量式PID需要保存的参数和上一次结果
struct PID
{
  float kp;
  float ki;
  float kd;
  float maxOutput;
  float minOutput;
  float lastError;
  float previousError;
  float output;
};

void PID_Init(PID *pid, float kp, float ki, float kd, float maxOutput, float minOutput);
void PID_Clear(PID *pid);
float PID_IncPIDCal(PID *pid, float currentSpeed, float targetSpeed);//calculate

#endif
