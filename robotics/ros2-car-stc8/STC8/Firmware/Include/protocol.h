#ifndef MOTOR_PROTOCOL_H
#define MOTOR_PROTOCOL_H
#include "project_types.h"
#include "motor_channels.h"
#define COMMAND_SIZE 14
#define STATUS_SIZE 64
#define FW_VERSION 0x0401U
#define FW_CAPABILITIES 0x0F3FU
#ifdef BOARD_COMM_DIAGNOSTICS
#if BOARD_COMM_DIAGNOSTICS
#undef FW_CAPABILITIES
#define FW_CAPABILITIES 0x0F20U
#endif
#endif
#define RESPONSE_TYPE 0x93
#define CMD_STOP 0x13
#define CMD_QUERY 0x14
#define CMD_ZERO 0x15
#define CMD_SINGLE 0x16
#define CMD_BATCH 0x17
#define CMD_HEARTBEAT 0x18
#define CMD_CLEAR 0x19
#define CMD_PARAM_GET 0x20
#define CMD_PARAM_SET 0x21
#define CMD_DEFAULT 0x23
typedef struct {
    u8 bytes[COMMAND_SIZE];
    u8 used;
} PacketParser;
typedef struct {
    u8 type;
    u8 sequence;
    s16 values[MOTOR_COUNT];
    u8 flags;
} MotorCommand;
u8 Protocol_Crc(const u8 APP_XDATA *bytes, u8 length);
void Protocol_Reset(PacketParser APP_XDATA *parser);
u8 Protocol_Feed(PacketParser APP_XDATA *parser, u8 value, MotorCommand APP_XDATA *command);
void Protocol_Put16(u8 APP_XDATA *bytes, u16 value);
void Protocol_Put32(u8 APP_XDATA *bytes, u32 value);
#endif
