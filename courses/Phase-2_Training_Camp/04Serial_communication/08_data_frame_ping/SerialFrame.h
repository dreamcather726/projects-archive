#ifndef SERIAL_FRAME_H
#define SERIAL_FRAME_H

#include <Arduino.h>

// 发送一条完整的 6 字节数据帧
void sendFrame(byte command, byte data);

// 检查串口中是否收到一条正确的数据帧
bool readFrame();

// 读取上一条正确数据帧中的指令和数据
byte getFrameCommand();
byte getFrameData();

#endif

