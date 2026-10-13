#ifndef CONTROL_LOGIC_H
#define CONTROL_LOGIC_H
#include "project_types.h"
#include "parameters.h"
#define DUTY_LIMIT 1000
#define SPEED_LIMIT 10000
typedef struct { s32 integral; } SpeedController;
void Speed_Reset(SpeedController APP_XDATA *controller);
s16 Speed_Step(SpeedController APP_XDATA *controller, s16 target, s16 measured,
               const MotorParameters APP_XDATA *parameters);
s16 Control_Ramp(s16 current, s16 target, u16 step);
#endif
