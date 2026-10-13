/*
  程序功能：板 B 通过软串口接收板 A 的数据帧，控制 LED 并发送应答帧。
  使用方法：板 B 上传本程序，GPIO 41 TX 接板 A 的 GPIO 42 RX，GPIO 42 RX 接板 A 的 GPIO 41 TX，并共地。
  运行结果：板 A 输入 led 数值时板 B 改变 LED 亮度；输入 ping 时板 B 返回通信应答。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <SoftwareSerial.h>
#include "SerialFrame.h"

#define PIN_UART_RX 42
#define PIN_UART_TX 41
#define PIN_LED 2

#define CMD_LED 0x01
#define CMD_PING 0x05
#define CMD_ERROR 0x7F

// 创建软串口：第一个是 RX，第二个是 TX
SoftwareSerial SoftSerial(PIN_UART_RX, PIN_UART_TX);

// 保存板 A 发来的指令和数据
byte frameCommand = 0;
byte frameData = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);

  // 直接启动软串口，板 A 和板 B 都使用 9600
  SoftSerial.begin(9600);
}

void loop() {
  if (readSoftFrame()) {
    if (frameCommand == CMD_LED) {
      analogWrite(PIN_LED, frameData);
      sendSoftFrame(0x81, frameData);
    } else if (frameCommand == CMD_PING) {
      sendSoftFrame(0x85, 0x00);
    } else {
      sendSoftFrame(CMD_ERROR, 0x01);
    }
  }
}

// 使用软串口发送一条完整数据帧
void sendSoftFrame(byte command, byte data) {
  SoftSerial.write(FRAME_HEADER);
  SoftSerial.write(FRAME_LENGTH);
  SoftSerial.write(command);
  SoftSerial.write(data);
  SoftSerial.write(calculateChecksum(command, data));
  SoftSerial.write(FRAME_TAIL);
}

// 使用软串口接收并检查一条数据帧
bool readSoftFrame() {
  while (SoftSerial.available() >= FRAME_SIZE) {
    byte header = SoftSerial.read();

    // 丢弃帧头前面的干扰字节
    if (header != FRAME_HEADER) {
      continue;
    }

    byte length = SoftSerial.read();
    byte command = SoftSerial.read();
    byte data = SoftSerial.read();
    byte checksum = SoftSerial.read();
    byte tail = SoftSerial.read();

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
