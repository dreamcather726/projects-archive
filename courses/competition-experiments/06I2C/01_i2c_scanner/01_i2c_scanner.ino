/*
  程序功能：扫描 I2C 总线，查找已连接设备并在串口显示设备地址。
  使用方法：I2C 设备的 SDA 接 GPIO 8、SCL 接 GPIO 9，打开波特率为 115200 的串口监视器。
  运行结果：串口显示找到的十六进制地址，例如 MPU6050 为 0x68、OLED 常为 0x3C。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>

#define PIN_SDA 8
#define PIN_SCL 9

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  Serial.println("开始扫描 I2C 地址");
}

void loop() {
  int deviceCount = 0;

  // I2C 的 7 位设备地址通常在 0x01～0x7E 之间
  for (int address = 1; address < 127; address = address + 1) {
    Wire.beginTransmission(address);
    int result = Wire.endTransmission();

    if (result == 0) {
      Serial.print("找到设备：0x");

      if (address < 16) {
        Serial.print("0");
      }

      Serial.println(address, HEX);
      deviceCount = deviceCount + 1;
    }
  }

  Serial.print("设备数量：");
  Serial.println(deviceCount);
  Serial.println();

  delay(3000);
}
