/*
  程序功能：板 A 把电脑输入的 LED 或 PING 命令转换成数据帧，通过 UART1 发送给板 B。
  使用方法：板间 TX/RX 交叉并共地；打开板 A 的 115200 串口，输入 led 255、led 0 或 ping。
  运行结果：led 255 使板 B 的 LED 最亮，led 0 使 LED 熄灭；ping 会收到板 B 的 0x85、0x00 回复。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "SerialFrame.h"

#define PIN_UART_RX 42
#define PIN_UART_TX 41

#define CMD_LED 0x01
#define CMD_PING 0x05

// 使用 ESP32-S3 的 UART1 创建板间硬件串口
HardwareSerial BoardSerial(1);

void setup() {
  // USB 串口连接电脑，用于输入文字命令和显示板 B 的应答
  Serial.begin(115200);

  // A、B 两块板之间都使用 UART1、9600、8N1
  BoardSerial.begin(9600, SERIAL_8N1,
                    PIN_UART_RX, PIN_UART_TX);

  Serial.println("输入 led 255 开灯，输入 led 0 关灯，输入 ping 测试通信");
}

void loop() {
  // 电脑输入文字，板 A 使用 SerialFrame 库向板 B 发送数据帧
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();

    if (message.startsWith("led ")) {
      int value = message.substring(4).toInt();
      value = constrain(value, 0, 255);

      sendFrame(BoardSerial, CMD_LED, (byte)value);
      Serial.println("板 A 已发送 LED 指令");
    } else if (message == "ping") {
      sendFrame(BoardSerial, CMD_PING, 0x00);
      Serial.println("板 A 已发送 PING 指令");
    } else {
      Serial.println("命令格式错误");
    }
  }

  // SerialFrame 库从 UART1 接收并校验板 B 的应答帧
  if (readFrame(BoardSerial)) {
    byte frameCommand = getFrameCommand();
    byte frameData = getFrameData();

    Serial.printf("板 B 回复：指令 0x%02X，数据 0x%02X\n",
                  frameCommand, frameData);
  }
}
