/*
  程序功能：使用 DMP 解算 MPU6050 姿态，并把姿态、红外、距离和 RGB 状态显示在 ST7789 TFT。
  使用方法：按程序引脚连接 TFT 和各模块；上电校准时保持 MPU6050 静止，再操作各传感器。
  运行结果：TFT 显示 Yaw、Pitch、Roll、红外、距离和 RGB，彩灯随距离改变颜色。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// Wire、I2Cdev、MotionApps20 负责 MPU6050；GFX、ST7789、SPI 负责 TFT；
// NeoPixel 负责 WS2812。综合程序同时使用 I2C、SPI 和单总线输出。
#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

// 传感器、I2C 和 WS2812 引脚集中定义，便于按模块分组排查。
#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40
#define PIN_TRIG 12
#define PIN_ECHO 13
#define PIN_RGB 48

// TFT 采用软件 SPI；丝印 SDA/SCL 在这里实际表示 MOSI/SCK。
const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;
// 屏幕先按物理尺寸初始化，再旋转成课堂使用的 320×240 横屏。
const uint16_t TFT_NATIVE_WIDTH = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

// 软件 SPI 构造参数顺序：CS、DC、MOSI、SCK、RST。
Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

// 创建 1 颗 GRB 顺序的 WS2812 对象和 MPU6050 对象。
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);
MPU6050 mpu;

// DMP 就绪标志用于阻止初始化完成前读取 FIFO。
bool dmpReady = false;
// FIFO 数据经过四元数、重力向量计算，最终得到 ypr。
uint8_t fifoBuffer[64];
Quaternion quaternion;
VectorFloat gravity;
float ypr[3];

// 把角度制结果保存在全局变量中，updateScreen() 可以直接访问。
float yaw = 0;
float pitch = 0;
float roll = 0;

void setup() {
  Serial.begin(115200);

  // 配置红外和超声波 GPIO，并让 TRIG 上电时保持低电平。
  pinMode(PIN_IR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // MPU6050 走 I2C，总线与 TFT 的 SPI 引脚互不冲突。
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  // 先初始化 TFT，让漫长的 DMP 校准过程有可见提示。
  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextColor(ST77XX_YELLOW);
  screen.setTextSize(2);
  screen.setCursor(55, 100);
  screen.print("DMP CALIBRATING");

  // initialize() 配置芯片，testConnection() 验证 I2C 通信。
  mpu.initialize();

  if (!mpu.testConnection()) {
    stopProgram("MPU6050 ERROR");
  }

  // 返回 0 表示 DMP 初始化成功，否则显示错误并停止。
  byte dmpStatus = mpu.dmpInitialize();

  if (dmpStatus != 0) {
    stopProgram("DMP INIT ERROR");
  }

  // 校准时保持模块静止、水平，否则 Y/P/R 零点会明显偏移。
  mpu.setXGyroOffset(0);
  mpu.setYGyroOffset(0);
  mpu.setZGyroOffset(0);
  mpu.setXAccelOffset(0);
  mpu.setYAccelOffset(0);
  mpu.setZAccelOffset(0);
  mpu.CalibrateAccel(6);
  mpu.CalibrateGyro(6);

  // 校准完成后启用 DMP，并允许主循环读取数据包。
  mpu.setDMPEnabled(true);
  dmpReady = true;

  // WS2812 初始化后先熄灭，避免上电出现未知灯色。
  rgb.begin();
  rgb.clear();
  rgb.show();

  // 固定标题和标签只绘制一次
  drawStaticScreen();
}

void loop() {
  // 没有取得完整 DMP 数据包时不刷新界面，避免显示无效姿态。
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  // 先采集输入，再进行距离判断，最后统一更新两个输出。
  bool irDetected = digitalRead(PIN_IR) == LOW;
  float distance = readDistance();

  // colorName 用于文字，colorText 用于文字颜色，两者都要与灯光同步。
  String colorName;
  uint16_t colorText;

  // 无回波关闭；近距离红色；中距离黄色；远距离绿色。
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

  // setPixelColor() 只修改颜色数据，show() 才真正发送到灯珠。
  rgb.show();
  // TFT 绘图直接生效，不需要 OLED 那样的统一 display() 提交。
  updateScreen(irDetected, distance, colorName, colorText);
  delay(100);
}

// 固定内容只绘制一次，避免反复清屏造成闪烁
void drawStaticScreen() {
  // 固定界面只画一次：背景、标题、分隔线和六个标签。
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

  // 每一行先用黑色矩形完整覆盖旧值，再在同一区域写入新值。
  // 擦除区域要容得下最长字符串，才能避免长数字变短后留下尾巴。
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

  // 红外状态不仅更换文字，也用红/绿颜色增强视觉反馈。
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

  // 距离 0 表示测距超时，因此显示 NO ECHO 而不是 0.0 cm。
  if (distance == 0) {
    screen.print("NO ECHO");
  } else {
    screen.print(distance, 1);
    screen.print(" cm");
  }

  // RGB 文字使用与灯光相同的语义颜色。
  screen.fillRect(105, 195, 210, 22, ST77XX_BLACK);
  screen.setTextColor(colorText);
  screen.setCursor(110, 198);
  screen.print(colorName);
}

// 从 DMP 的 FIFO 中读取一组姿态角
bool readDmpAngles() {
  // 函数返回 true 才代表本轮得到了一组新的有效姿态角。
  if (dmpReady == false) {
    return false;
  }

  // FIFO 暂时没有完整数据包时返回 false，不重复显示旧数据。
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer) == false) {
    return false;
  }

  // 解算顺序：数据包 → 四元数 → 重力向量 → Y/P/R。
  mpu.dmpGetQuaternion(&quaternion, fifoBuffer);
  mpu.dmpGetGravity(&gravity, &quaternion);
  mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity);

  // DMP 给出弧度值，乘以 180/PI 转成角度。
  yaw = ypr[0] * 180.0 / PI;
  pitch = ypr[1] * 180.0 / PI;
  roll = ypr[2] * 180.0 / PI;

  return true;
}

float readDistance() {
  // 发送 10 微秒高电平脉冲，触发一次超声波测量。
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 最多等待 30 ms；超时返回 0，避免主循环长期阻塞。
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);
  // 声速约 0.0343 cm/us，除以 2 得到传感器到障碍物的单程距离。
  float distance = echoTime * 0.0343 / 2.0;

  return distance;
}

// 在 TFT 上显示错误并停止程序
void stopProgram(String message) {
  // 串口和 TFT 同时显示错误，便于学生从任一渠道排查。
  Serial.println(message);

  screen.fillScreen(ST77XX_BLACK);
  screen.setTextColor(ST77XX_RED);
  screen.setTextSize(2);
  screen.setCursor(65, 105);
  screen.print(message);

  // 初始化失败后继续进入综合流程没有意义，因此停在错误画面。
  while (true) {
    delay(1000);
  }
}
