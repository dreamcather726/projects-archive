/*
  程序功能：板 A 把电脑输入的 LED 或 PING 命令转换成数据帧，通过软串口发送给板 B。
  使用方法：板间 TX/RX 交叉并共地；打开板 A 的 115200 串口，输入 led 255 或 ping。
  运行结果：led 255 使板 B 的 LED 最亮；ping 会收到板 B 的 0x85、0x00 回复。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <SoftwareSerial.h>
#include "SerialFrame.h"

#define PIN_UART_RX 42
#define PIN_UART_TX 41

#define CMD_LED 0x01
#define CMD_PING 0x05

// 创建软串口：第一个是 RX，第二个是 TX
SoftwareSerial SoftSerial(PIN_UART_RX, PIN_UART_TX);

// 保存板 B 回复的指令和数据
byte frameCommand = 0;
byte frameData = 0;

void setup() {
  // USB 串口：连接电脑的串口监视器
  Serial.begin(115200);

  // 直接启动软串口，板 A 和板 B 都使用 9600
  SoftSerial.begin(9600);

  Serial.println("输入 led 255 设置板 B 的亮度，输入 ping 测试通信");
}

void loop() {
  // 电脑输入文字，板 A 把它转换成二进制数据帧
  if (Serial.available() > 0) {
    String message = Serial.readStringUntil('\n');
    message.trim();

    if (message.startsWith("led ")) {
      int value = message.substring(4).toInt();
      value = constrain(value, 0, 255);
      sendSoftFrame(CMD_LED, value);
      Serial.println("板 A 已发送 LED 指令");
    } else if (message == "ping") {
      sendSoftFrame(CMD_PING, 0x00);
      Serial.println("板 A 已发送 PING 指令");
    } else {
      Serial.println("命令格式错误");
    }
  }

  // 板 A 从软串口接收板 B 的二进制应答
  if (readSoftFrame()) {
    Serial.printf("板 B 回复：指令 0x%02X，数据 0x%02X\n",
                  frameCommand, frameData);
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
