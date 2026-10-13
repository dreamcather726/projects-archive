#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H
#include "project_types.h"
#ifndef MOTOR_ENCODER_AB
#error Select STC8_4Motor_AOnly (MOTOR_ENCODER_AB=0)
#endif
#if MOTOR_ENCODER_AB
#error V4 uses four independent DIR outputs and four encoder A inputs
#endif
#ifdef MOTOR_HOST_TEST
#include "mock_registers.h"
#endif

/* Compile-time board configuration. Runtime parameters are in parameters.h. */
#define FOSC_HZ MAIN_Fosc
#define UART_BAUD 115200UL
#define PWM_FREQUENCY 20000UL
#define PWM_COUNTS (FOSC_HZ / PWM_FREQUENCY)
#define CONTROL_PERIOD_MS 50U
#define CONTROL_MAX_DELAY_MS 100U
/* 0x00: PWM1P..4P = P1.0/1.2/1.4/1.6.
 * 0xC0: move only PWM4P to P3.4 (leave P3.3 PWM4N disabled).
 */
#define BOARD_PWM_ROUTE 0x00U
#define MOTOR_DIR1_PORT 1
#define MOTOR_DIR1_PIN 1
#define MOTOR_DIR2_PORT 1
#define MOTOR_DIR2_PIN 3
#define MOTOR_DIR3_PORT 1
#define MOTOR_DIR3_PIN 5
#define MOTOR_DIR4_PORT 1
#define MOTOR_DIR4_PIN 7
#define BOARD_INVERT_DEFAULT_MASK 0U
#ifndef BOARD_COMM_DIAGNOSTICS
#define BOARD_COMM_DIAGNOSTICS 0
#endif

/* Fixed resources: UART1 P3.0/RX, P3.1/TX; STBY P5.4;
 * A1=P3.2/INT0, A2=P3.3/INT1, A3=P3.6/INT2, A4=P3.7/INT3.
 * Encoders count falling edges; hardware PWM uses only positive outputs.
 */
#define BOARD_ENCODER_MASK 0xCCU
#define BOARD_PWM_P1_MASK (0x15U | (BOARD_PWM_ROUTE==0 ? 0x40U : 0U))
#define BOARD_PWM_P3_MASK (BOARD_PWM_ROUTE==0 ? 0U : 0x10U)
#define DIR_MASK(port) ((MOTOR_DIR1_PORT==port ? (1U<<MOTOR_DIR1_PIN) : 0U) | \
                       (MOTOR_DIR2_PORT==port ? (1U<<MOTOR_DIR2_PIN) : 0U) | \
                       (MOTOR_DIR3_PORT==port ? (1U<<MOTOR_DIR3_PIN) : 0U) | \
                       (MOTOR_DIR4_PORT==port ? (1U<<MOTOR_DIR4_PIN) : 0U))
#define DIR_VALID(port,pin) (((port)==1 || (port)==3) && (pin)>=0 && (pin)<=7)
#define DIR_ID(port,pin) ((port)*8+(pin))
#if !DIR_VALID(MOTOR_DIR1_PORT,MOTOR_DIR1_PIN) || !DIR_VALID(MOTOR_DIR2_PORT,MOTOR_DIR2_PIN) || \
    !DIR_VALID(MOTOR_DIR3_PORT,MOTOR_DIR3_PIN) || !DIR_VALID(MOTOR_DIR4_PORT,MOTOR_DIR4_PIN)
#error DIR must use available P1 or P3 GPIO pins
#endif
#if BOARD_PWM_ROUTE != 0 && BOARD_PWM_ROUTE != 0xC0
#error Only P1 PWM pins or PWM4P on P3.4 are available on this board
#endif
#if (DIR_MASK(1) & BOARD_PWM_P1_MASK) || (DIR_MASK(3) & (BOARD_PWM_P3_MASK | 0xCFU))
#error DIR conflicts with a PWM, UART or encoder input
#endif
#if DIR_ID(MOTOR_DIR1_PORT,MOTOR_DIR1_PIN)==DIR_ID(MOTOR_DIR2_PORT,MOTOR_DIR2_PIN) || \
    DIR_ID(MOTOR_DIR1_PORT,MOTOR_DIR1_PIN)==DIR_ID(MOTOR_DIR3_PORT,MOTOR_DIR3_PIN) || \
    DIR_ID(MOTOR_DIR1_PORT,MOTOR_DIR1_PIN)==DIR_ID(MOTOR_DIR4_PORT,MOTOR_DIR4_PIN) || \
    DIR_ID(MOTOR_DIR2_PORT,MOTOR_DIR2_PIN)==DIR_ID(MOTOR_DIR3_PORT,MOTOR_DIR3_PIN) || \
    DIR_ID(MOTOR_DIR2_PORT,MOTOR_DIR2_PIN)==DIR_ID(MOTOR_DIR4_PORT,MOTOR_DIR4_PIN) || \
    DIR_ID(MOTOR_DIR3_PORT,MOTOR_DIR3_PIN)==DIR_ID(MOTOR_DIR4_PORT,MOTOR_DIR4_PIN)
#error DIR pins must be independent
#endif
#if PWM_COUNTS < 2 || PWM_COUNTS > 65535UL || (FOSC_HZ % PWM_FREQUENCY)!=0
#error Invalid hardware PWM frequency
#endif
#if CONTROL_PERIOD_MS != 50U
#error Changing the control period requires updating parameter units and PI tuning
#endif
#if BOARD_INVERT_DEFAULT_MASK > 15U
#error Default direction inversion mask must use only bits 0..3
#endif
#ifndef MOTOR_HOST_TEST
#define PORT_REGISTER_INNER(port) P##port
#define PORT_REGISTER(port) PORT_REGISTER_INNER(port)
sbit MOTOR_STBY=P5^4;
sbit MOTOR_DIR1=PORT_REGISTER(MOTOR_DIR1_PORT)^MOTOR_DIR1_PIN;
sbit MOTOR_DIR2=PORT_REGISTER(MOTOR_DIR2_PORT)^MOTOR_DIR2_PIN;
sbit MOTOR_DIR3=PORT_REGISTER(MOTOR_DIR3_PORT)^MOTOR_DIR3_PIN;
sbit MOTOR_DIR4=PORT_REGISTER(MOTOR_DIR4_PORT)^MOTOR_DIR4_PIN;
#endif
#endif
