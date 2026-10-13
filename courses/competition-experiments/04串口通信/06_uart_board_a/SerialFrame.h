#ifndef SERIAL_FRAME_H
#define SERIAL_FRAME_H

#include <Arduino.h>

// 固定帧格式：55 05 指令 数据 校验 FF
const byte FRAME_HEADER = 0x55;
const byte FRAME_LENGTH = 0x05;
const byte FRAME_TAIL = 0xFF;
const byte FRAME_SIZE = 6;

// 计算加法校验位
byte calculateChecksum(byte command, byte data);

#endif
