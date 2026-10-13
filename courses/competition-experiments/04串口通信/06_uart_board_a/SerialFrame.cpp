#include "SerialFrame.h"

// 校验位 = 长度 + 指令 + 数据，只保留低 8 位
byte calculateChecksum(byte command, byte data) {
  return (byte)(FRAME_LENGTH + command + data);
}
