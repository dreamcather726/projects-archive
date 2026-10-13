#include "speed_pid.h"

// PID初始化：设置PID参数和输出范围
void PID_Init(PID *pid, float Kp, float Ki, float Kd,
              float maxValue, float minValue) {
  // 最大值和最小值相同，说明范围无效
  if (maxValue == minValue) {
    return;
  }

  pid->P = Kp;
  pid->I = Ki;
  pid->D = Kd;
  pid->OutputMax = maxValue;
  pid->OutputMin = minValue;
}

// 清除历史误差和输出，不改变PID参数
void PID_Clear(PID *pid) {
  pid->LastError = 0.0;
  pid->PrevError = 0.0;
  pid->Output = 0.0;
}

// 增量式PID计算
float PID_IncPIDCal(PID *pid, float NowValue, float AimValue) {
  // 当前误差 = 目标值 - 当前值
  float error = AimValue - NowValue;

  // 计算本次应该增加或减少多少输出
  float increment =
      pid->P * (error - pid->LastError)
    + pid->I * error
    + pid->D * (error - 2 * pid->LastError + pid->PrevError);

  // 把本次增量累加到原来的输出
  pid->Output = pid->Output + increment;

  // 输出限幅，避免输出超过执行器允许范围
  if (pid->Output > pid->OutputMax) {
    pid->Output = pid->OutputMax;
  }

  if (pid->Output < pid->OutputMin) {
    pid->Output = pid->OutputMin;
  }

  // 保存误差，供下一次计算使用
  pid->PrevError = pid->LastError;
  pid->LastError = error;

  return pid->Output;
}

