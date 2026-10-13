#include "SerialFrame.h"

// 固定帧格式：55 05 指令 数据 校验 FF
const byte FRAME_HEADER = 0x55;
const byte FRAME_LENGTH = 0x05;
const byte FRAME_TAIL = 0xFF;
const byte FRAME_SIZE = 6;

// 保存最近一次正确接收的指令和数据
byte frameCommand = 0;
byte frameData = 0;

// 计算“长度 + 指令 + 数据”，byte 会自动保留低 8 位
byte calculateChecksum(byte command, byte data) {
  return (byte)(FRAME_LENGTH + command + data);
}

void sendFrame(byte command, byte data) {
  // 按协议顺序填写 6 个字节
  byte frame[FRAME_SIZE];

  frame[0] = FRAME_HEADER;
  frame[1] = FRAME_LENGTH;
  frame[2] = command;
  frame[3] = data;
  frame[4] = calculateChecksum(command, data);
  frame[5] = FRAME_TAIL;

  // 一次发送完整数据帧
  Serial.write(frame, FRAME_SIZE);
}

bool readFrame() {
  // 数据没有达到 6 个字节时先等待，不读取半条帧
  while (Serial.available() >= FRAME_SIZE) {
    byte header = Serial.read();

    // 丢弃帧头 0x55 前面的干扰字节
    if (header != FRAME_HEADER) {
      continue;
    }

    // 找到帧头后，依次读取剩余 5 个字节
    byte length = Serial.read();
    byte command = Serial.read();
    byte data = Serial.read();
    byte checksum = Serial.read();
    byte tail = Serial.read();

    // 长度、帧尾或校验不正确时不执行指令
    if (length != FRAME_LENGTH) {
      continue;
    }

    if (tail != FRAME_TAIL) {
      continue;
    }

    if (checksum != calculateChecksum(command, data)) {
      continue;
    }

    // 全部检查通过后保存指令和数据
    frameCommand = command;
    frameData = data;
    return true;
  }

  return false;
}

// 主程序通过这两个函数取得接收结果
byte getFrameCommand() {
  return frameCommand;
}

byte getFrameData() {
  return frameData;
}

