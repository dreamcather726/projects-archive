/*
  程序功能：板 B 使用 SerialFrame 库从 UART1 接收板 A 的数据帧，控制 LED 并发送应答帧。
  使用方法：板 B 的 GPIO 41 TX 接板 A 的 GPIO 42 RX，GPIO 42 RX 接板 A 的 GPIO 41 TX，并共地。
  运行结果：板 A 输入 led 255 时板 B 的 LED 点亮，输入 led 0 时 LED 熄灭；输入 ping 时返回通信应答。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "SerialFrame.h"

#define PIN_UART_RX 42
#define PIN_UART_TX 41
#define PIN_LED 2

#define CMD_LED 0x01
#define CMD_PING 0x05
#define CMD_ERROR 0x7F

// 使用 ESP32-S3 的 UART1 创建板间硬件串口
HardwareSerial BoardSerial(1);

void setup() {
  pinMode(PIN_LED, OUTPUT);

  // A、B 两块板之间都使用 UART1、9600、8N1
  BoardSerial.begin(9600, SERIAL_8N1,
                    PIN_UART_RX, PIN_UART_TX);
}

void loop() {
  // SerialFrame 库负责从 UART1 接收、寻找帧头并校验数据帧
  if (readFrame(BoardSerial)) {
    byte frameCommand = getFrameCommand();
    byte frameData = getFrameData();

    if (frameCommand == CMD_LED) {
      analogWrite(PIN_LED, frameData);
      sendFrame(BoardSerial, 0x81, frameData);
    } else if (frameCommand == CMD_PING) {
      sendFrame(BoardSerial, 0x85, 0x00);
    } else {
      sendFrame(BoardSerial, CMD_ERROR, 0x01);
    }
  }
}
