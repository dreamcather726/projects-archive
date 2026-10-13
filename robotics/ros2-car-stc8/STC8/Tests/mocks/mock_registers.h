#ifndef MOCK_REGISTERS_H
#define MOCK_REGISTERS_H
#include "project_types.h"
#define MAIN_Fosc 24000000L
#define xdata
#define code
#define ENABLE 1
#define DISABLE 0
#define Polity_1 1
#define Polity_2 2
#define Polity_3 3
extern u8 EA, ES, P_SW2, P1, P1PU, P3PU, RI, TI;
extern u8 MOTOR_STBY, MOTOR_DIR1, MOTOR_DIR2, MOTOR_DIR3, MOTOR_DIR4;
extern u8 ENCODER_B1, ENCODER_B2, ENCODER_B3, ENCODER_B4;
extern u16 mock_ccr1, mock_ccr2, mock_ccr3, mock_ccr4, mock_prescaler;
extern u8 PWMA_EGR, PWMA_SR1;
#define EAXSFR() (P_SW2 |= 0x80)
#define EAXRAM() (P_SW2 &= ~0x80)
#endif
