/*
  程序功能：读取 MPU6050 的三轴加速度和三轴陀螺仪原始数据，并通过串口显示。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9，上传后打开 115200 串口并移动、转动模块。
  运行结果：串口持续显示 AX、AY、AZ、GX、GY、GZ 六个原始数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

#define PIN_SDA 8
#define PIN_SCL 9

MPU6050 mpu;

int16_t ax, ay, az;
int16_t gx, gy, gz;

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 连接成功");
  } else {
    Serial.println("MPU6050 连接失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  Serial.print("AX:");
  Serial.print(ax);
  Serial.print(" AY:");
  Serial.print(ay);
  Serial.print(" AZ:");
  Serial.print(az);

  Serial.print(" GX:");
  Serial.print(gx);
  Serial.print(" GY:");
  Serial.print(gy);
  Serial.print(" GZ:");
  Serial.println(gz);

  delay(200);
}
