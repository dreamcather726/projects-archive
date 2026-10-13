/*
  程序功能：使用 MPU6050 内部 DMP 解算 Yaw、Pitch、Roll，并通过串口显示。
  使用方法：SDA 接 GPIO 8、SCL 接 GPIO 9；上电校准时保持静止，之后打开 115200 串口转动模块。
  运行结果：串口持续显示 Yaw、Pitch、Roll 角度；无磁力计约束时 Yaw 可能缓慢漂移。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

#define PIN_SDA 8
#define PIN_SCL 9

MPU6050 mpu;

// DMP 解算需要使用的变量
bool dmpReady = false;
uint8_t fifoBuffer[64];
Quaternion quaternion;
VectorFloat gravity;
float ypr[3];

float yaw = 0;
float pitch = 0;
float roll = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 连接失败");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("正在初始化 DMP...");
  byte dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    Serial.print("DMP 初始化失败，错误代码：");
    Serial.println(dmpStatus);

    while (true) {
      delay(1000);
    }
  }

  // 模块校准时应保持静止、水平放置
  Serial.println("正在校准，请保持模块静止...");
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);
  mpu.PrintActiveOffsets();

  // 开启 MPU6050 内部的 DMP
  mpu.setDMPEnabled(true);
  dmpReady = true;
  Serial.println("DMP 初始化完成");
}

void loop() {
  if (readDmpAngles()) {
    Serial.print("Yaw:");
    Serial.print(yaw, 1);
    Serial.print("  Pitch:");
    Serial.print(pitch, 1);
    Serial.print("  Roll:");
    Serial.println(roll, 1);
  }

  delay(100);
}

// 从 DMP 的 FIFO 中读取一组姿态角
bool readDmpAngles() {
  if (dmpReady == false) {
    return false;
  }

  // 没有收到完整数据包时，暂时不更新角度
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) == false) {
    return false;
  }

  // FIFO 数据包 → 四元数 → 重力向量 → Yaw/Pitch/Roll
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  yaw = ypr[0] * 180.0 / PI;
  pitch = ypr[1] * 180.0 / PI;
  roll = ypr[2] * 180.0 / PI;

  return true;
}
