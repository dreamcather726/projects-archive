#ifndef MOTOR_PARAMETERS_H
#define MOTOR_PARAMETERS_H
#include "project_types.h"
#include "motor_channels.h"
/* Stable parameter IDs: gains use /64. */
#define PAR_POLARITY 0
#define PAR_MAX_PWM 1
#define PAR_MAX_SPEED 2
#define PAR_CPR 3
#define PAR_KP 4
#define PAR_KI 5
#define PAR_I_LIMIT 6
#define PAR_PWM_ACCEL 7
#define PAR_PWM_DECEL 8
#define PAR_REVERSE_MS 9
#define PAR_STALL_PWM 10
#define PAR_STALL_SPEED 11
#define PAR_STALL_MS 12
#define PAR_SPEED_ACCEL 13
#define PAR_SPEED_DECEL 14
#define MOTOR_PARAM_COUNT 15
#define CONFIG_SCHEMA_VERSION 4U
#define PAR_TIMEOUT 128
typedef struct { u16 value[MOTOR_PARAM_COUNT]; } MotorParameters;
typedef struct {
    MotorParameters motor[MOTOR_COUNT];
    u16 timeout_ms;
} Configuration;
extern Configuration APP_XDATA settings;
/* 0=defaults, 2=RAM modified; configuration is volatile. */
extern u8 config_state;
void Config_Default(void);
u8 Config_Access(u8 write, u8 scope, u8 parameter, u16 APP_XDATA *value);
#endif
