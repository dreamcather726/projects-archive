#ifndef MOTOR_ENCODER_H
#define MOTOR_ENCODER_H
#include "project_types.h"
#include "motor_channels.h"
extern volatile signed char encoder_direction[MOTOR_COUNT];
void Encoder_ReadAll(u32 APP_XDATA *counts);
void Encoder_Zero(u8 mask);
void Encoder_M1ISR(void);
void Encoder_M2ISR(void);
void Encoder_M3ISR(void);
void Encoder_M4ISR(void);
#endif
