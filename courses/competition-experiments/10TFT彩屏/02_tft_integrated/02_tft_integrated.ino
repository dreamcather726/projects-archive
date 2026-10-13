/*
  程序功能：使用 DMP 解算 MPU6050 姿态，并把姿态、红外、距离和 RGB 状态显示在 ST7789 TFT。
  使用方法：按程序引脚连接 TFT 和各模块；上电校准时保持 MPU6050 静止，再操作各传感器。
  运行结果：TFT 显示 Yaw、Pitch、Roll、红外、距离和 RGB，彩灯随距离改变颜色。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40
#define PIN_TRIG 12
#define PIN_ECHO 13
#define PIN_RGB 48

const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;
const uint16_t TFT_NATIVE_WIDTH = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
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

  pinMode(PIN_IR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextColor(ST77XX_YELLOW);
  screen.setTextSize(2);
  screen.setCursor(55, 100);
  screen.print("DMP CALIBRATING");

  mpu.initialize();

  if (!mpu.testConnection()) {
    stopProgram("MPU6050 ERROR");
  }

  byte dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    stopProgram("DMP INIT ERROR");
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

  rgb.begin();
  rgb.clear();
  rgb.show();

  // 固定标题和标签只绘制一次
  drawStaticScreen();
}

void loop() {
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  bool irDetected = digitalRead(PIN_IR) == LOW;
  float distance = readDistance();

  String colorName;
  uint16_t colorText;

  if (distance == 0) {
    rgb.setPixelColor(0, rgb.Color(0, 0, 0));
    colorName = "OFF";
    colorText = ST77XX_WHITE;
  } else if (distance <= 20) {
    rgb.setPixelColor(0, rgb.Color(30, 0, 0));
    colorName = "RED";
    colorText = ST77XX_RED;
  } else if (distance <= 50) {
    rgb.setPixelColor(0, rgb.Color(30, 20, 0));
    colorName = "YELLOW";
    colorText = ST77XX_YELLOW;
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 30, 0));
    colorName = "GREEN";
    colorText = ST77XX_GREEN;
  }

  rgb.show();
  updateScreen(irDetected, distance, colorName, colorText);
  delay(100);
}

// 固定内容只绘制一次，避免反复清屏造成闪烁
void drawStaticScreen() {
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextWrap(false);

  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(50, 8);
  screen.print("SENSOR DASHBOARD");
  screen.drawFastHLine(0, 34, 320, ST77XX_BLUE);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(8, 48);
  screen.print("Yaw:");
  screen.setCursor(8, 78);
  screen.print("Pitch:");
  screen.setCursor(8, 108);
  screen.print("Roll:");
  screen.setCursor(8, 138);
  screen.print("IR:");
  screen.setCursor(8, 168);
  screen.print("Dist:");
  screen.setCursor(8, 198);
  screen.print("RGB:");
}

// 只刷新数值区域，不重复绘制标题和标签
void updateScreen(bool irDetected, float distance,
                  String colorName, uint16_t colorText) {
  screen.setTextSize(2);

  screen.fillRect(105, 45, 210, 22, ST77XX_BLACK);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(110, 48);
  screen.print(yaw, 1);
  screen.print(" deg");

  screen.fillRect(105, 75, 210, 22, ST77XX_BLACK);
  screen.setCursor(110, 78);
  screen.print(pitch, 1);
  screen.print(" deg");

  screen.fillRect(105, 105, 210, 22, ST77XX_BLACK);
  screen.setCursor(110, 108);
  screen.print(roll, 1);
  screen.print(" deg");

  screen.fillRect(105, 135, 210, 22, ST77XX_BLACK);
  screen.setCursor(110, 138);

  if (irDetected) {
    screen.setTextColor(ST77XX_RED);
    screen.print("DETECTED");
  } else {
    screen.setTextColor(ST77XX_GREEN);
    screen.print("CLEAR");
  }

  screen.fillRect(105, 165, 210, 22, ST77XX_BLACK);
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(110, 168);

  if (distance == 0) {
    screen.print("NO ECHO");
  } else {
    screen.print(distance, 1);
    screen.print(" cm");
  }

  screen.fillRect(105, 195, 210, 22, ST77XX_BLACK);
  screen.setTextColor(colorText);
  screen.setCursor(110, 198);
  screen.print(colorName);
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

float readDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);
  float distance = echoTime * 0.0343 / 2.0;

  return distance;
}

// 在 TFT 上显示错误并停止程序
void stopProgram(String message) {
  Serial.println(message);

  screen.fillScreen(ST77XX_BLACK);
  screen.setTextColor(ST77XX_RED);
  screen.setTextSize(2);
  screen.setCursor(65, 105);
  screen.print(message);

  while (true) {
    delay(1000);
  }
}
