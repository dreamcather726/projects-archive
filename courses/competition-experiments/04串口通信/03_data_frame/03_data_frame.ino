/*
  程序功能：接收并校验固定格式的数据帧，根据指令设置 LED 亮度并返回应答帧。
  使用方法：串口设为 115200、HEX 发送和显示；示例发送 55 05 01 FF 05 FF。
  运行结果：GPIO 2 的 LED 亮度变为 255，并应答 55 05 81 FF 85 FF。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "SerialFrame.h"

// 0x01 表示设置 LED 亮度，数据范围为 0x00～0xFF
#define CMD_LED 0x01
#define CMD_ERROR 0x7F

#define PIN_LED 2

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  if (readFrame()) {
    byte command = getFrameCommand();
    byte data = getFrameData();

    if (command == CMD_LED) {
      analogWrite(PIN_LED, data);

      // 0x81 是 0x01 指令的成功应答，数据原样返回
      sendFrame(0x81, data);
    } else {
      // 错误帧：0x01 表示不支持这个指令
      sendFrame(CMD_ERROR, 0x01);
    }
  }
}
