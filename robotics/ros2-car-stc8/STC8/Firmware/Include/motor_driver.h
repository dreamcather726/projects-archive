#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H
#include "project_types.h"
#include "motor_channels.h"
extern s16 APP_XDATA motor_applied[MOTOR_COUNT];
void Motor_Disable(u8 mask);
void Motor_Reconfigure(u8 scope);
void Motor_Apply(const s16 APP_XDATA *requested);
#endif
