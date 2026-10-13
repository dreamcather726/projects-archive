#include "speed_pid.h"

void PID_Clear(PID *pid)
{
  pid->lastError = 0;
  pid->previousError = 0;
  pid->output = 0;
}//清除之前的所有的累计误差

void PID_Init(PID *pid, float kp, float ki, float kd, float maxOutput, float minOutput)
{
  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
  pid->maxOutput = maxOutput;
  pid->minOutput = minOutput;//限制输出的大小
  PID_Clear(pid);
}

float PID_IncPIDCal(PID *pid, float currentSpeed, float targetSpeed)
{
  float error = targetSpeed - currentSpeed;

  // 增量式PID公式
  float change = pid->kp * (error - pid->lastError)
               + pid->ki * error
               + pid->kd * (error - 2 * pid->lastError + pid->previousError);

  pid->output += change;
  pid->output = constrain(pid->output, pid->minOutput, pid->maxOutput);

  pid->previousError = pid->lastError;  
  pid->lastError = error;
  return pid->output;
}
