/*
  程序功能：使用 MPU6050 内部 DMP 解算姿态，并在 OLED 显示 Yaw、Pitch、Roll。
  使用方法：OLED 和 MPU6050 共用 GPIO 8/9；上电校准时保持模块静止，完成后转动模块。
  运行结果：OLED 实时显示 Y、P、R 三个姿态角；初始化失败时显示错误信息。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
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

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MPU6050 DMP");
  display.println("Calibrating...");
  display.display();

  mpu.initialize();

  if (!mpu.testConnection()) {
    showMpuError("CONNECT ERROR");
  }

  byte dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    showMpuError("DMP ERROR");
  }

  // 模块校准时应保持静止、水平放置
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);

  mpu.setDMPEnabled(true);
  dmpReady = true;
}

void loop() {
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MPU6050 DMP");

  display.setTextSize(2);
  display.setCursor(0, 14);
  display.print("Y:");
  display.println(yaw, 1);
  display.print("P:");
  display.println(pitch, 1);
  display.print("R:");
  display.println(roll, 1);

  display.display();
  delay(100);
}

// 从 DMP 的 FIFO 中读取一组姿态角
bool readDmpAngles() {
  if (dmpReady == false) {
    return false;
  }

  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) == false) {
    return false;
  }

  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  yaw = ypr[0] * 180.0 / PI;
  pitch = ypr[1] * 180.0 / PI;
  roll = ypr[2] * 180.0 / PI;

  return true;
}

// 在 OLED 上显示错误并停止程序
void showMpuError(String message) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MPU6050 ERROR");
  display.println(message);
  display.display();

  while (true) {
    delay(1000);
  }
}
