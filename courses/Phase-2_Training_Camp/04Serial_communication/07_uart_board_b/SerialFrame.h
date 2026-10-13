#ifndef SERIAL_FRAME_H
#define SERIAL_FRAME_H

#include <Arduino.h>

// 默认使用 USB 串口 Serial，兼容今天课堂中的原有调用方式
void sendFrame(byte command, byte data);
bool readFrame();

// 指定串口对象，用于 HardwareSerial 等 Stream 串口对象
void sendFrame(Stream &serialPort, byte command, byte data);
bool readFrame(Stream &serialPort);

// 读取上一条正确数据帧中的指令和数据
byte getFrameCommand();
byte getFrameData();

#endif
