#include "SerialFrame.h"

// 固定帧格式：55 05 指令 数据 校验 FF
const byte FRAME_HEADER = 0x55;
const byte FRAME_LENGTH = 0x05;
const byte FRAME_TAIL = 0xFF;
const byte FRAME_SIZE = 6;

// 保存最近一次正确接收的指令和数据
byte frameCommand = 0;
byte frameData = 0;

// 计算“长度 + 指令 + 数据”，并保留低 8 位
byte calculateChecksum(byte command, byte data) {
  return (byte)(FRAME_LENGTH + command + data);
}

// 兼容原有库：没有指定串口时，默认使用 USB 串口 Serial
void sendFrame(byte command, byte data) {
  sendFrame(Serial, command, data);
}

bool readFrame() {
  return readFrame(Serial);
}

// 通过指定串口发送一条完整数据帧
void sendFrame(Stream &serialPort, byte command, byte data) {
  byte frame[FRAME_SIZE];

  frame[0] = FRAME_HEADER;
  frame[1] = FRAME_LENGTH;
  frame[2] = command;
  frame[3] = data;
  frame[4] = calculateChecksum(command, data);
  frame[5] = FRAME_TAIL;

  serialPort.write(frame, FRAME_SIZE);
}

// 从指定串口接收并检查一条数据帧
bool readFrame(Stream &serialPort) {
  while (serialPort.available() >= FRAME_SIZE) {
    byte header = serialPort.read();

    // 丢弃帧头 0x55 前面的干扰字节
    if (header != FRAME_HEADER) {
      continue;
    }

    byte length = serialPort.read();
    byte command = serialPort.read();
    byte data = serialPort.read();
    byte checksum = serialPort.read();
    byte tail = serialPort.read();

    // 长度、校验位和帧尾都正确才接收
    if (length == FRAME_LENGTH &&
        checksum == calculateChecksum(command, data) &&
        tail == FRAME_TAIL) {
      frameCommand = command;
      frameData = data;
      return true;
    }
  }

  return false;
}

byte getFrameCommand() {
  return frameCommand;
}

byte getFrameData() {
  return frameData;
}
