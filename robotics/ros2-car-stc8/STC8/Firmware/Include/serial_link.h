#ifndef SERIAL_LINK_H
#define SERIAL_LINK_H
#include "project_types.h"
void Serial_RxISR(u8 value);
u8 Serial_Read(u8 APP_DATA *value);
u8 Serial_Recover(void);
void Serial_Send(const u8 APP_XDATA *bytes, u8 length);
#endif
