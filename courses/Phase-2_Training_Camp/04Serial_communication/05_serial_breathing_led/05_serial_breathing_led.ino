/*
  程序功能：通过串口数据帧设置 LED 亮度，并控制呼吸灯的启停和速度。
  使用方法：串口设为 115200 和 HEX；示例发送 55 05 03 01 09 FF 启动呼吸灯。
  运行结果：GPIO 2 的 LED 开始呼吸，并应答 55 05 83 01 89 FF；其他示例见下方指令表。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "SerialFrame.h"

#define CMD_LED 0x01
#define CMD_BREATH 0x03
#define CMD_SPEED 0x04
#define CMD_ERROR 0x7F

#define PIN_LED 2

// ---------------- 串口测试指令 ----------------
// 串口工具设置为：115200 波特率、HEX 发送、HEX 显示
//
// LED 亮度设置为 255：
// 发送：55 05 01 FF 05 FF
// 应答：55 05 81 FF 85 FF
//
// LED 亮度设置为 128：
// 发送：55 05 01 80 86 FF
// 应答：55 05 81 80 06 FF
//
// LED 关闭：
// 发送：55 05 01 00 06 FF
// 应答：55 05 81 00 86 FF
//
// 启动呼吸灯：
// 发送：55 05 03 01 09 FF
// 应答：55 05 83 01 89 FF
//
// 停止呼吸灯：
// 发送：55 05 03 00 08 FF
// 应答：55 05 83 00 88 FF
//
// 呼吸灯每步延时设置为 20 ms，0x14 等于十进制 20：
// 发送：55 05 04 14 1D FF
// 应答：55 05 84 14 9D FF

// 呼吸灯运行状态和亮度参数
bool breathing = false;
int brightness = 0;
int changeValue = 5;
int speedDelay = 20;

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  if (readFrame()) {
    byte command = getFrameCommand();
    byte data = getFrameData();
    runCommand(command, data);
  }

  if (breathing == true) {
    analogWrite(PIN_LED, brightness);
    brightness = brightness + changeValue;

    if (brightness >= 255) {
      brightness = 255;
      changeValue = -5;
    }

    if (brightness <= 0) {
      brightness = 0;
      changeValue = 5;
    }

    delay(speedDelay);
  }
}

void runCommand(byte command, byte data) {
  if (command == CMD_LED) {
    breathing = false;
    brightness = data;
    analogWrite(PIN_LED, brightness);
    sendFrame(0x81, data);
  } else if (command == CMD_BREATH && data <= 1) {
    breathing = data;
    if (breathing == false) {
      analogWrite(PIN_LED, 0);
    }
    sendFrame(0x83, data);
  } else if (command == CMD_SPEED && data >= 2 && data <= 100) {
    speedDelay = data;
    sendFrame(0x84, data);
  } else {
    sendFrame(CMD_ERROR, 0x02);
  }
}
