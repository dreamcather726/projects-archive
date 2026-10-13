#ifndef MOTOR_HAL_H
#define MOTOR_HAL_H
#include "project_types.h"
/* Compact STC8H1K08-only adapter derived from the STC SDK. */
void Hal_Init(void);
void Hal_PWM(const u16 APP_XDATA *compare);
u8 Hal_Send(const u8 APP_XDATA *bytes, u8 length);
#endif
