/*
  程序功能：使用 MPU6050 内部 DMP 解算姿态，并在 OLED 显示 Yaw、Pitch、Roll。
  使用方法：OLED 和 MPU6050 共用 GPIO 8/9；上电校准时保持模块静止，完成后转动模块。
  运行结果：OLED 实时显示 Y、P、R 三个姿态角；初始化失败时显示错误信息。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire 管理 I2C；I2Cdev 和 MotionApps20 提供 MPU6050 DMP 功能；
// GFX 与 SSD1306 负责 OLED 绘图和显示。
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED 与 MPU6050 共用同一组 I2C 引脚，通过不同地址区分设备。
#define PIN_SDA 8
#define PIN_SCL 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

// OLED 地址为 0x3C，MPU6050 常用地址为 0x68，两者可以挂在同一总线上。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MPU6050 mpu;

// dmpReady 是“DMP 是否已经初始化完成”的标志，避免未就绪时读取数据。
bool dmpReady = false;
// FIFO 数据包先进入缓冲区，再由 DMP 函数逐步解算。
uint8_t fifoBuffer[64];
// 四元数和重力向量是计算欧拉姿态角所需的中间结果。
Quaternion quaternion;
VectorFloat gravity;
// ypr 依次保存弧度制的 Yaw、Pitch、Roll。
float ypr[3];

// 单独保存角度制结果，便于在 loop() 中直接显示。
float yaw = 0;
float pitch = 0;
float roll = 0;

void setup() {
  Serial.begin(115200);

  // 两个设备共用总线时统一使用 400 kHz，可提高姿态数据读取速度。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  // OLED 必须先初始化成功，才能显示后续校准状态和错误信息。
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
  // 前面的 print() 只改缓冲区，调用 display() 后学生才会看到校准提示。
  display.display();

  // initialize() 配置芯片，testConnection() 再确认 I2C 通信是否正常。
  mpu.initialize();

  if (!mpu.testConnection()) {
    showMpuError("CONNECT ERROR");
  }

  // dmpInitialize() 返回 0 才表示 DMP 固件与内部设置初始化成功。
  byte dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    showMpuError("DMP ERROR");
  }

  // 先把示例偏移量设为 0，再用库函数进行现场校准。
  // 校准期间必须保持模块静止、水平，否则零点会被错误动作污染。
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);

  // 完成校准后才开启 DMP，并把就绪标志设为 true。
  mpu.setDMPEnabled(true);
  dmpReady = true;
}

void loop() {
  // FIFO 暂时没有完整数据包时先返回，避免显示一组无效角度。
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  // 动态数字长度会变化，每次先清缓冲区可防止旧字符残留。
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

  // 一次性提交整帧缓冲区，使三个角度同时更新。
  display.display();
  delay(100);
}

// 从 DMP 的 FIFO 中读取一组姿态角
bool readDmpAngles() {
  // 函数用 true/false 告诉 loop() 本轮是否取得有效数据。
  if (dmpReady == false) {
    return false;
  }

  // 只在 FIFO 中存在一包完整数据时继续解算。
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) == false) {
    return false;
  }

  // 解算顺序：原始数据包 → 四元数 → 重力向量 → Y/P/R。
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  // DMP 给出弧度值，乘以 180/PI 转成课堂更直观的角度值。
  yaw = ypr[0] * 180.0 / PI;
  pitch = ypr[1] * 180.0 / PI;
  roll = ypr[2] * 180.0 / PI;

  return true;
}

// 在 OLED 上显示错误并停止程序
void showMpuError(String message) {
  // 错误画面覆盖校准提示，让学生能直接看到失败原因。
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("MPU6050 ERROR");
  display.println(message);
  display.display();

  // 硬件初始化失败时继续运行没有意义，因此停在错误画面等待排查。
  while (true) {
    delay(1000);
  }
}
