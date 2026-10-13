# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验先只验证 TFT 接线、方向和基础绘图是否正常。
  - 这节使用的是软件 SPI 构造方式，所以构造函数里要直接写 `CS`、`DC`、`MOSI`、`SCK`、`RST`。
  - 基础画面一次绘制完成后，主循环可以为空。
- 本实验新增的术语、模块或函数：
  - `screen.init()`
  - `setRotation()`
  - `fillScreen()`
  - `drawRect()`
  - `drawFastHLine()`
  - `fillRect()`
- 开始本实验前需要特别提醒的点：
  - TFT 的 `SDA/SCL` 是 SPI 的 `MOSI/SCK`，不是 I2C。
  - `TFT_CS_PIN = 47`，`TFT_DC_PIN = 21`，`TFT_SDA_PIN = 20`，`TFT_SCL_PIN = 19`。

### 2. 实验1接线
- 接线表：

| TFT 引脚 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `CS` | `GPIO 47` | 片选 |
| `DC` | `GPIO 21` | 数据/命令选择 |
| `RST` | 不接 | 程序中设为 `-1` |
| `SDA` | `GPIO 20` | SPI `MOSI` |
| `SCL` | `GPIO 19` | SPI `SCK` |
| `GND` | `GND` | 共地 |
| `VCC` | 按模块标注 | 模块供电 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 Adafruit_ST7789 库初始化 TFT，并显示文字、线条和色块。
  使用方法：TFT 的 CS、DC、SDA、SCL 分别接 GPIO 47、21、20、19，上传后直接观察屏幕。
  运行结果：横向 320×240 屏幕显示标题、边框和红绿蓝三个色块。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;

// TFT 丝印 SDA、SCL 在 SPI 模式下分别是 MOSI、SCK
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;

const uint16_t TFT_NATIVE_WIDTH = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

// 使用软件 SPI 创建 ST7789 屏幕对象
Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

void setup() {
  // 按照屏幕原生尺寸初始化，再旋转为横屏
  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);
  screen.fillScreen(ST77XX_BLACK);

  // 先画出白色边框
  screen.drawRect(4, 4, 312, 232, ST77XX_WHITE);

  // 显示标题
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(52, 18);
  screen.print("QINGCHAO ST7789");

  // 画一条蓝色分隔线
  screen.drawFastHLine(5, 46, 310, ST77XX_BLUE);

  // 显示红、绿、蓝三个基础色块
  screen.fillRect(25, 75, 75, 75, ST77XX_RED);
  screen.fillRect(122, 75, 75, 75, ST77XX_GREEN);
  screen.fillRect(219, 75, 75, 75, ST77XX_BLUE);

  // 底部显示状态提示
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(68, 180);
  screen.print("320 x 240 READY");
}

void loop() {
  // 基础画面保持不变，主循环不需要重复刷新
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 创建 ST7789 屏幕对象。
  2. 先按原生尺寸初始化屏幕。
  3. 旋转为横屏模式。
  4. 清屏并绘制边框、标题、分隔线和色块。
  5. 显示底部状态提示文字。
- `setup()` 的作用：
  - 完成屏幕初始化和首屏静态界面绘制。
- `loop()` 的作用：
  - 本实验没有动态刷新内容。
- 数据输入、处理、输出的执行过程：
  - 输入：无外部传感器输入。
  - 处理：把要显示的文字和图形直接绘制到屏幕。
  - 输出：TFT 显示基础彩色界面。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Adafruit_ST7789 screen(...)`
  - `screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT)`
  - `screen.setRotation(TFT_ROTATION)`
  - `screen.fillScreen(ST77XX_BLACK)`
  - `screen.fillRect(...)`
- 每个函数的作用、写法、参数和返回值：
  - `Adafruit_ST7789 screen(...)`
    - 作用：创建 ST7789 屏幕对象。
    - 参数依次为：`CS`、`DC`、`MOSI`、`SCK`、`RST`。
  - `screen.init(width, height)`
    - 函数作用：按照屏幕原生尺寸初始化屏幕。
    - 参数1：原生宽度。
    - 参数2：原生高度。
    - 返回值：无返回值。
  - `screen.setRotation(3)`
    - 函数作用：设置屏幕显示方向。
    - 参数1：方向编号。
    - 返回值：无返回值。
  - `screen.fillRect(x, y, w, h, color)`
    - 函数作用：用指定颜色填充一个矩形区域。
    - 参数1~参数4：矩形位置和大小。
    - 参数5：颜色值。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `screen.init(240, 320)` 后再 `setRotation(3)`，说明“初始化尺寸”和“最终显示方向”是两件事。
- 为什么这样写：
  - 先用基础程序确认接线、方向、颜色都正常，能减少后面整合实验的排错难度。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验会同时使用 TFT、`MPU6050`、红外、超声波和 RGB。
  - `MPU6050` 仍然走 I2C，而 TFT 走 SPI，两组总线是并行存在的。
  - 为了减少闪烁，固定标题和标签只画一次，动态数值区域单独更新。
- 本实验新增的术语、模块或函数：
  - `drawStaticScreen()`
  - `updateScreen()`
  - `stopProgram()`
  - `colorText`
  - 局部刷新
- 开始本实验前需要特别提醒的点：
  - `MPU6050` 校准时保持静止。
  - 超声波 `ECHO` 必须分压。
  - `GPIO 21` 用于 TFT `DC`，不能再接其他冲突器件。

### 2. 实验2接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| TFT `CS` | `GPIO 47` | 片选 |
| TFT `DC` | `GPIO 21` | 数据/命令选择 |
| TFT `SDA(MOSI)` | `GPIO 20` | SPI 数据线 |
| TFT `SCL(SCK)` | `GPIO 19` | SPI 时钟线 |
| `MPU6050 SDA` | `GPIO 8` | I2C 数据线 |
| `MPU6050 SCL` | `GPIO 9` | I2C 时钟线 |
| 红外 `OUT` | `GPIO 40` | 数字输入 |
| 超声波 `TRIG` | `GPIO 12` | 触发引脚 |
| 超声波 `ECHO` | `GPIO 13` | 分压后接入 |
| RGB `DIN` | `GPIO 48` | WS2812 数据引脚 |
| 所有 `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
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

  // 初始化红外、超声波引脚
  pinMode(PIN_IR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // I2C 供 MPU6050 使用
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  // 初始化 TFT 屏幕
  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextColor(ST77XX_YELLOW);
  screen.setTextSize(2);
  screen.setCursor(55, 100);
  screen.print("DMP CALIBRATING");

  // 初始化 MPU6050 并检查连接
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

  // 初始化 RGB 灯
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

  // 根据距离区间设置 RGB 颜色和屏幕文字颜色
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

  // 只更新动态区域
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

// 读取一次超声波距离
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
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化红外、超声波、I2C、TFT、`MPU6050` 和 RGB。
  2. 在屏幕上显示校准提示并完成 DMP 初始化。
  3. 画出固定标题和标签。
  4. 在循环中读取姿态角、红外状态和超声波距离。
  5. 根据距离区间决定 RGB 颜色和颜色文字。
  6. 只刷新动态数值区域。
- `setup()` 的作用：
  - 初始化全部模块，并完成静态界面绘制。
- `loop()` 的作用：
  - 周期性采集多路传感器数据并更新显示。
- 数据输入、处理、输出的执行过程：
  - 输入：`MPU6050` 姿态角、红外状态、超声波距离。
  - 处理：把距离值转换成 RGB 颜色和显示文字。
  - 输出：TFT 显示综合信息，RGB 灯同步反馈。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `drawStaticScreen()`
  - `updateScreen(...)`
  - `screen.fillRect(...)`
  - `stopProgram(String message)`
  - `uint16_t colorText`
- 每个函数的作用、写法、参数和返回值：
  - `drawStaticScreen()`
    - 函数作用：绘制标题和固定标签。
    - 参数：无。
    - 返回值：无返回值。
  - `updateScreen(bool irDetected, float distance, String colorName, uint16_t colorText)`
    - 函数作用：刷新动态数值区域。
    - 参数1：红外状态。
    - 参数2：距离值。
    - 参数3：颜色名称。
    - 参数4：颜色文字值。
    - 返回值：无返回值。
  - `screen.fillRect(x, y, w, h, ST77XX_BLACK)`
    - 函数作用：先用黑色擦除旧数值区域，再写入新数值。
    - 参数1~参数4：矩形位置和大小。
    - 参数5：填充颜色。
    - 返回值：无返回值。
  - `stopProgram(String message)`
    - 函数作用：在 TFT 上显示错误并停止程序。
    - 参数1：错误信息。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `固定标签只绘制一次 + 局部 fillRect 擦除重绘` 是这章最值得学生带走的界面刷新思路，比每次整屏清空再重画更稳定。
- 为什么这样写：
  - TFT 彩屏信息量更大，局部刷新思路是从“能显示”走向“显示得更像产品界面”的关键一步。

---

## 总结
- 实验目标：
  - 学会初始化 ST7789 TFT 并显示基础图形和文字。
  - 学会区分 TFT 的 SPI 接线和传感器的 I2C 接线。
  - 学会把 `MPU6050`、红外、超声波和 RGB 信息整合显示到 TFT。
  - 学会使用局部刷新减少彩屏闪烁。
- 单词：
  - `screen`：屏幕
  - `color`：颜色
  - `rotation`：旋转
  - `dashboard`：仪表盘
  - `static`：静态的
  - `update`：更新
  - `integrated`：整合的
  - `error`：错误
  - `distance`：距离
  - `detected`：检测到
  - `clear`：无遮挡 / 清除
  - `width`：宽度
  - `height`：高度
  - `refresh`：刷新
  - `native`：原生的
- 接线：
  - `TFT CS -> GPIO 47`
  - `TFT DC -> GPIO 21`
  - `TFT SDA(MOSI) -> GPIO 20`
  - `TFT SCL(SCK) -> GPIO 19`
  - `MPU6050 SDA -> GPIO 8`
  - `MPU6050 SCL -> GPIO 9`
  - `IR OUT -> GPIO 40`
  - `TRIG -> GPIO 12`
  - `ECHO -> GPIO 13(分压后接入)`
  - `RGB DIN -> GPIO 48`
  - 所有模块 `GND` 必须共地
- 关键函数和参数：
  - `screen.init(240, 320)`
  - `screen.setRotation(3)`
  - `screen.fillScreen()`
  - `screen.drawRect()`
  - `screen.fillRect()`
  - `drawStaticScreen()`
  - `updateScreen()`
  - `mpu.dmpInitialize()`
  - `pulseIn(PIN_ECHO, HIGH, 30000)`
  - `rgb.setPixelColor()`
- 关键程序：
  - TFT 基础显示程序
  - TFT 综合仪表盘程序
- 实验现象：
  - TFT 会显示标题、色块和传感器信息。
  - RGB 灯会随距离变化改变颜色。
  - 红外、姿态角和距离都会实时更新。
- 本课重点：
  - TFT 走 SPI，`MPU6050` 走 I2C，两组总线不要混淆。
  - `setRotation(3)` 决定了横屏布局。
  - 局部刷新是减少彩屏闪烁的重要方法。
