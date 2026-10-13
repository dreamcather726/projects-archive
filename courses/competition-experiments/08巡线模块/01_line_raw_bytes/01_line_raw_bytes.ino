/*
  程序功能：通过 I2C 读取八路巡线模块返回的 32 个原始字节，并在串口显示。
  使用方法：巡线模块 SDA 接 GPIO 8、SCL 接 GPIO 9，打开 115200 串口并让各探头经过黑白表面。
  运行结果：串口每次显示 32 个十六进制原始字节，表面变化时对应数据随之改变。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>

#define PIN_SDA 8
#define PIN_SCL 9

const uint8_t SENSOR_ADDRESS = 0x27;
const uint8_t READ_COMMAND = 0x01;
const uint8_t DATA_LENGTH = 32;

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
}

void loop() {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(READ_COMMAND);
  int result = Wire.endTransmission();

  if (result != 0) {
    Serial.println("发送读取命令失败");
    delay(500);
    return;
  }

  delay(10);

  int receivedBytes = Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH);

  if (receivedBytes != DATA_LENGTH) {
    Serial.println("没有收到完整的 32 字节");

    while (Wire.available() > 0) {
      Wire.read();
    }

    delay(500);
    return;
  }

  Serial.print("原始字节：");

  for (int i = 0; i < DATA_LENGTH; i = i + 1) {
    int value = Wire.read();

    if (value < 16) {
      Serial.print("0");
    }

    Serial.print(value, HEX);
    Serial.print(" ");
  }

  Serial.println();
  delay(500);
}
