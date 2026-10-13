/*
  程序功能：使用 DMP 解算 MPU6050 姿态，并把姿态、红外、距离和 RGB 状态显示在 OLED。
  使用方法：按课程引脚连接全部模块；上电校准时保持 MPU6050 静止，之后移动和遮挡传感器。
  运行结果：OLED 同时显示 Y/P/R、红外状态、超声波距离和 RGB 颜色，彩灯随距离变色。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40
#define PIN_TRIG 12
#define PIN_ECHO 13
#define PIN_RGB 48

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
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
  display.println("DMP Calibrating...");
  display.display();

  mpu.initialize();

  if (!mpu.testConnection()) {
    stopProgram("MPU6050 CONNECT ERROR");
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
}

void loop() {
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  bool irDetected = digitalRead(PIN_IR) == LOW;
  float distance = readDistance();
  String colorName;

  if (distance == 0) {
    rgb.setPixelColor(0, rgb.Color(0, 0, 0));
    colorName = "OFF";
  } else if (distance <= 20) {
    rgb.setPixelColor(0, rgb.Color(30, 0, 0));
    colorName = "RED";
  } else if (distance <= 50) {
    rgb.setPixelColor(0, rgb.Color(30, 20, 0));
    colorName = "YELLOW";
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 30, 0));
    colorName = "GREEN";
  }

  rgb.show();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print("Y:");
  display.print(yaw, 0);
  display.print(" P:");
  display.print(pitch, 0);
  display.print(" R:");
  display.println(roll, 0);

  display.print("IR:");

  if (irDetected) {
    display.println("YES");
  } else {
    display.println("NO");
  }

  display.print("Distance:");

  if (distance == 0) {
    display.println("NONE");
  } else {
    display.print(distance, 1);
    display.println("cm");
  }

  display.print("RGB:");
  display.println(colorName);

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

// 显示错误信息并停止程序
void stopProgram(String message) {
  Serial.println(message);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(message);
  display.display();

  while (true) {
    delay(1000);
  }
}
