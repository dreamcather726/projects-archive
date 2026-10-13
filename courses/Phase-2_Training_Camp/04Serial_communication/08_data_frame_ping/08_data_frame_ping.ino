/*
  程序功能：接收固定格式数据帧，控制 LED 亮度或执行 PING 连通性测试。
  使用方法：串口设为 115200、HEX 发送和显示。
  运行结果：LED 指令控制 GPIO 2；PING 数据为 0x00 时返回 0x85 成功应答。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "SerialFrame.h"

#define CMD_LED 0x01
#define CMD_PING 0x05
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
      sendFrame(0x81, data);
    } else if (command == CMD_PING) {
      if (data == 0x00) {
        sendFrame(0x85, data);
      } else {
        // 0x02 表示指令存在，但数据不合法
        sendFrame(CMD_ERROR, 0x02);
      }
    } else {
      // 0x01 表示不支持这个指令
      sendFrame(CMD_ERROR, 0x01);
    }
  }
}

