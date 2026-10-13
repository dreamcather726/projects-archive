#ifndef MOTOR_APP_H
#define MOTOR_APP_H
#include "project_types.h"
#include "motor_channels.h"
#define MODE_STOP 0
#define MODE_PWM 1
#define MODE_SPEED 2
#define MODE_DECEL 3
#define FAULT_TIMEOUT 1
#define FAULT_RX_OVERFLOW 2
#define FAULT_ENCODER 4
#define FAULT_SCHEDULER 8
extern volatile u8 app_fault;
extern volatile u8 app_mode;
extern volatile u16 app_ticks;
extern volatile u16 app_command_age;
extern volatile u16 app_control_age;
extern u8 motor_modes[MOTOR_COUNT];
extern u8 fault_channels;
extern u32 APP_XDATA encoder_counts[MOTOR_COUNT];
void App_TickISR(void);
#if BOARD_COMM_DIAGNOSTICS
extern volatile u8 app_debug_stage,app_debug_rx_irq,app_debug_rx_main,app_debug_frames;
#endif
#ifdef MOTOR_HOST_TEST
void App_Init(void);
void App_Poll(void);
#endif
#endif
