/*
  程序功能：通过 I2C 读取八路巡线模块，并解析、显示 8 路模拟量数据。
  使用方法：巡线模块 SDA 接 GPIO 8、SCL 接 GPIO 9，打开 115200 串口并移动黑线或白色底板。
  运行结果：串口依次显示 CH1～CH8 的模拟量，可观察每一路对黑白表面的响应。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>

#define PIN_SDA 8
#define PIN_SCL 9

const uint8_t SENSOR_ADDRESS = 0x27;
const uint8_t READ_COMMAND = 0x01;
const uint8_t DATA_LENGTH = 32;

uint16_t sensorValue[8];

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
}

void loop() {
  if (readSensorValues()) {
    for (int i = 0; i < 8; i = i + 1) {
      Serial.print("S");
      Serial.print(i + 1);
      Serial.print(":");
      Serial.print(sensorValue[i]);
      Serial.print("  ");
    }

    Serial.println();
  } else {
    Serial.println("巡线模块读取失败");
  }

  delay(200);
}

bool readSensorValues() {
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(READ_COMMAND);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(10);

  int receivedBytes = Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH);

  if (receivedBytes != DATA_LENGTH) {
    while (Wire.available() > 0) {
      Wire.read();
    }

    return false;
  }

  // 按示例协议跳过前 16 个字节
  for (int i = 0; i < 16; i = i + 1) {
    Wire.read();
  }

  // 后 16 个字节组成 8 路大端 16 位模拟量
    for (int i = 0; i < 8; i = i + 1) {
      uint8_t highByte = Wire.read();
      uint8_t lowByte = Wire.read();

      sensorValue[i] = highByte * 256 + lowByte;
    }

  return true;
}
