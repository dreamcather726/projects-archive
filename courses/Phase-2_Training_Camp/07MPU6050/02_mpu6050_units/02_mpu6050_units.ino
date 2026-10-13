/*
  程序功能：读取 MPU6050 数据，并转换为加速度和角速度的常用单位后串口显示。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9，打开 115200 串口并改变模块姿态和转速。
  运行结果：串口显示三轴加速度 g 和三轴角速度 °/s，平放时某一加速度轴约为 1g。
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

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 连接失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
 
  // initialize() 默认设置为加速度 ±2g、角速度 ±250°/s
  float axG = ax / 16384.0;
  float ayG = ay / 16384.0;
  float azG = az / 16384.0;

  float gxDps = gx / 131.0;
  float gyDps = gy / 131.0;
  float gzDps = gz / 131.0;

  Serial.print("加速度(g) X:");
  Serial.print(axG, 2);
  Serial.print(" Y:");
  Serial.print(ayG, 2);
  Serial.print(" Z:");
  Serial.print(azG, 2);

  Serial.print(" 角速度(度/秒) X:");
  Serial.print(gxDps, 1);
  Serial.print(" Y:");
  Serial.print(gyDps, 1);
  Serial.print(" Z:");
  Serial.println(gzDps, 1);

  delay(200);
}
