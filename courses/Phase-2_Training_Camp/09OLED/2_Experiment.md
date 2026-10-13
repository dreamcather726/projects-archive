# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验先不接入其他传感器，只学习 OLED 的基础显示流程。
  - OLED 的显示内容先写入缓冲区，最后用 `display.display()` 一次刷新到屏幕。
  - `setTextSize()`、`setCursor()` 会直接影响文字大小和位置。
- 本实验新增的术语、模块或函数：
  - `display.begin()`
  - `clearDisplay()`
  - `setTextColor()`
  - `setTextSize()`
  - `setCursor()`
  - `display.display()`
- 开始本实验前需要特别提醒的点：
  - OLED 地址默认使用 `0x3C`。
  - `OLED_RESET` 统一设置为 `-1`。

### 2. 实验1接线
- 接线表：

| OLED | ESP32-S3 | 说明 |
| --- | --- | --- |
| `VCC` | `3.3V` 或按模块标注 | 屏幕供电 |
| `GND` | `GND` | 与开发板共地 |
| `SDA` | `GPIO 8` | I2C 数据线 |
| `SCL` | `GPIO 9` | I2C 时钟线 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：初始化 I2C OLED 屏幕，并显示基础文字和图形。
  使用方法：OLED 的 SDA 接 GPIO 8、SCL 接 GPIO 9，确认地址为 0x3C 后上传程序。
  运行结果：128×64 OLED 显示示例文字、矩形、直线和圆形等基础内容。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

// 创建 OLED 显示对象
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // 启动 I2C 总线
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  // 初始化 OLED，失败时停在这里
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }

  // 清空缓冲区，准备绘制新内容
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 显示大号标题
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("ESP32-S3");

  // 显示较小的说明文字
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println("I2C OLED");
  display.println("SDA:8  SCL:9");

  // 绘图完成后必须调用 display() 才会真正显示
  display.display();
}

void loop() {
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 启动串口和 I2C。
  2. 初始化 OLED。
  3. 清空显示缓冲区。
  4. 设置文字颜色、大小和光标位置。
  5. 把标题和说明文字写入缓冲区。
  6. 调用 `display.display()` 把内容刷新到屏幕。
- `setup()` 的作用：
  - 完成 OLED 初始化和首屏内容绘制。
- `loop()` 的作用：
  - 本实验没有循环刷新内容。
- 数据输入、处理、输出的执行过程：
  - 输入：无外部传感器输入。
  - 处理：构造显示内容并放入缓冲区。
  - 输出：OLED 显示文字内容。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)`
  - `display.clearDisplay()`
  - `display.setTextSize(2)`
  - `display.setCursor(0, 0)`
  - `display.display()`
- 每个函数的作用、写法、参数和返回值：
  - `display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)`
    - 函数作用：初始化 OLED 屏幕。
    - 参数1：供电方式。
    - 参数2：OLED 的 I2C 地址，本课为 `0x3C`。
    - 返回值：返回 `true` 或 `false`。
  - `display.setTextSize(2)`
    - 函数作用：设置后续文字的放大倍数。
    - 参数1：文字大小倍数。
    - 返回值：无返回值。
  - `display.display()`
    - 函数作用：把缓冲区中的内容真正刷新到屏幕上。
    - 参数：无。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `display.display()` 是 OLED 课程里最关键的一行，前面的 `print()`、`println()` 只是把内容写进缓冲区。
- 为什么这样写：
  - 先掌握最基础的 OLED 刷新流程，后面所有组合实验都在这个基础上增加传感器数据。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验让 OLED 和 `MPU6050` 共用同一组 I2C 引脚。
  - `MPU6050` 姿态角来自 DMP 解算结果，不是直接读寄存器就能得到。
  - 初始化时要先在 OLED 上显示校准提示，再进行 DMP 初始化和校准。
- 本实验新增的术语、模块或函数：
  - `dmpInitialize()`
  - `CalibrateAccel()`
  - `CalibrateGyro()`
  - `readDmpAngles()`
  - `showMpuError()`
- 开始本实验前需要特别提醒的点：
  - `MPU6050` 地址默认是 `0x68`。
  - 校准时要保持模块静止、水平放置。

### 2. 实验2接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| OLED `SDA` | `GPIO 8` | 与 `MPU6050 SDA` 并联 |
| OLED `SCL` | `GPIO 9` | 与 `MPU6050 SCL` 并联 |
| OLED `GND` | `GND` | 共地 |
| OLED `VCC` | 按模块标注 | OLED 供电 |
| `MPU6050 SDA` | `GPIO 8` | I2C 数据线 |
| `MPU6050 SCL` | `GPIO 9` | I2C 时钟线 |
| `MPU6050 GND` | `GND` | 共地 |
| `MPU6050 VCC` | `3.3V` | 模块供电 |
| `MPU6050 AD0` | `GND` | 默认地址 `0x68` |

### 3. 完整程序及注释讲解

```cpp
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

  // 先初始化 OLED，用来显示状态提示
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

  // 初始化 MPU6050 并检查连接
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

  // 打开 DMP
  mpu.setDMPEnabled(true);
  dmpReady = true;
}

void loop() {
  // 没有取到完整姿态数据时先等待
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  // 刷新 OLED 显示姿态角
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
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化 I2C、OLED 和 `MPU6050`。
  2. 在 OLED 上显示校准提示。
  3. 完成 DMP 初始化和传感器校准。
  4. 在循环中读取一组姿态角。
  5. 把 `Yaw/Pitch/Roll` 刷新到 OLED。
- `setup()` 的作用：
  - 初始化 OLED、`MPU6050` 和 DMP，并完成校准。
- `loop()` 的作用：
  - 持续读取姿态角并刷新显示。
- 数据输入、处理、输出的执行过程：
  - 输入：`MPU6050` 的 FIFO 姿态数据包。
  - 处理：DMP 数据包 -> 四元数 -> 重力向量 -> `Yaw/Pitch/Roll`。
  - 输出：OLED 显示三轴姿态角。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `mpu.dmpInitialize()`
  - `mpu.CalibrateAccel(6)`
  - `mpu.CalibrateGyro(6)`
  - `readDmpAngles()`
  - `showMpuError(String message)`
- 每个函数的作用、写法、参数和返回值：
  - `readDmpAngles()`
    - 函数作用：从 DMP 中读取一组完整姿态角并保存到全局变量。
    - 参数：无。
    - 返回值：`true` 表示读取成功，`false` 表示暂时没有新数据。
  - `showMpuError(String message)`
    - 函数作用：在 OLED 上显示错误信息并停止程序。
    - 参数1：错误文字。
    - 返回值：无返回值。
  - `mpu.dmpGetYawPitchRoll(ypr, &quaternion, &gravity)`
    - 函数作用：根据四元数和重力向量计算姿态角。
    - 参数1：保存结果的数组。
    - 参数2：四元数变量地址。
    - 参数3：重力向量变量地址。
    - 返回值：无返回值。
- 关键代码片段分析：
  - 先在 OLED 上显示 `"Calibrating..."`，再开始 DMP 校准，能让学生知道“屏幕也可以用来显示程序状态”，不只是显示最终结果。
- 为什么这样写：
  - OLED 和姿态传感器组合后，学生可以直接看到姿态角变化，观察效果比单纯看串口更直观。

---

## 实验3

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 红外模块这节只读取数字状态，不读取模拟值。
  - 常见红外避障模块检测到物体时输出 `LOW`，但不同模块也可能相反。
  - OLED 这节用来显示“检测到 / 未检测到”两种状态。
- 本实验新增的术语、模块或函数：
  - `digitalRead()`
  - `detected`
  - `DETECTED`
  - `CLEAR`
- 开始本实验前需要特别提醒的点：
  - 本例默认红外检测到障碍物时输出 `LOW`。
  - 若实物相反，需要把判断条件改为 `== HIGH`。

### 2. 实验3接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| OLED `SDA` | `GPIO 8` | I2C 数据线 |
| OLED `SCL` | `GPIO 9` | I2C 时钟线 |
| OLED `GND` | `GND` | 共地 |
| 红外模块 `OUT` | `GPIO 40` | 数字输出 |
| 红外模块 `GND` | `GND` | 共地 |
| 红外模块 `VCC` | 按模块标注 | 模块供电 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：读取红外模块的数字状态，并实时显示在 OLED 屏幕上。
  使用方法：OLED 接 GPIO 8/9，红外模块数字输出接 GPIO 40，用物体遮挡或移开红外探头。
  运行结果：OLED 根据红外输入状态切换显示 DETECTED 和 CLEAR。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_IR 40

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  // 红外模块数字引脚设为输入
  pinMode(PIN_IR, INPUT);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  // 常见红外避障模块检测到物体时输出 LOW
  bool detected = digitalRead(PIN_IR) == LOW;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("IR SENSOR");

  display.setTextSize(2);
  display.setCursor(0, 24);

  if (detected) {
    display.println("DETECTED");
  } else {
    display.println("CLEAR");
  }

  display.display();
  delay(100);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化红外输入引脚、I2C 和 OLED。
  2. 在循环中读取红外模块状态。
  3. 根据状态决定显示 `DETECTED` 或 `CLEAR`。
  4. 刷新 OLED。
- `setup()` 的作用：
  - 初始化 OLED 和红外输入引脚。
- `loop()` 的作用：
  - 周期性读取红外状态并刷新显示。
- 数据输入、处理、输出的执行过程：
  - 输入：红外模块数字电平。
  - 处理：把 `LOW/HIGH` 翻译为“检测到 / 未检测到”。
  - 输出：OLED 显示英文状态。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `pinMode(PIN_IR, INPUT)`
  - `digitalRead(PIN_IR) == LOW`
  - `display.println("DETECTED")`
- 每个函数的作用、写法、参数和返回值：
  - `digitalRead(PIN_IR)`
    - 函数作用：读取红外模块输出引脚的当前电平。
    - 参数1：输入引脚号。
    - 返回值：返回 `HIGH` 或 `LOW`。
  - `pinMode(PIN_IR, INPUT)`
    - 函数作用：把红外模块引脚配置为输入模式。
    - 参数1：引脚号。
    - 参数2：输入模式。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `bool detected = digitalRead(PIN_IR) == LOW` 这一句把“硬件电平”翻译成“是否检测到”，是代码可读性更好的写法。
- 为什么这样写：
  - 课堂上比起直接显示 `0` 和 `1`，直接显示 `DETECTED` 和 `CLEAR` 更符合学生直觉。

---

## 实验4

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验用超声波模块测距，并把结果显示到 OLED。
  - 若没有收到有效回波，程序会把距离视作 `0`，OLED 上显示 `NO ECHO`。
  - 经典 `5V HC-SR04` 的 `ECHO` 要先分压。
- 本实验新增的术语、模块或函数：
  - `delayMicroseconds()`
  - `pulseIn()`
  - `readDistance()`
  - `NO ECHO`
- 开始本实验前需要特别提醒的点：
  - `TRIG = GPIO 12`
  - `ECHO = GPIO 13`
  - `ECHO` 必须按前面超声波章节的方法分压。

### 2. 实验4接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| OLED `SDA` | `GPIO 8` | I2C 数据线 |
| OLED `SCL` | `GPIO 9` | I2C 时钟线 |
| 超声波 `TRIG` | `GPIO 12` | 触发引脚 |
| 超声波 `ECHO` | `GPIO 13` | 分压后接入 |
| 超声波 `GND` | `GND` | 共地 |
| 超声波 `VCC` | 按模块标注 | 模块供电 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用超声波模块测量距离，并实时显示在 OLED 屏幕上。
  使用方法：OLED 接 GPIO 8/9，TRIG 接 GPIO 12、ECHO 经分压接 GPIO 13，然后移动障碍物。
  运行结果：OLED 实时显示距离和 cm 单位；无有效回波时显示 NO ECHO。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_TRIG 12
#define PIN_ECHO 13

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }
}

void loop() {
  float distance = readDistance();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ULTRASONIC");

  display.setTextSize(2);
  display.setCursor(0, 24);

  if (distance == 0) {
    display.println("NO ECHO");
  } else {
    display.print(distance, 1);
    display.println(" cm");
  }

  display.display();
  delay(200);
}

float readDistance() {
  // 发送 10us 触发脉冲
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 测量回波高电平持续时间
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);
  float distance = echoTime * 0.0343 / 2.0;

  return distance;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化超声波引脚、I2C 和 OLED。
  2. 在循环中调用 `readDistance()` 测距。
  3. 若距离为 `0`，显示 `NO ECHO`。
  4. 若测距成功，显示距离数值和单位。
  5. 刷新 OLED。
- `setup()` 的作用：
  - 初始化 OLED 和超声波测距引脚。
- `loop()` 的作用：
  - 周期性测距并刷新显示。
- 数据输入、处理、输出的执行过程：
  - 输入：超声波回波时间。
  - 处理：按公式换算为厘米距离。
  - 输出：OLED 显示距离或无回波状态。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `delayMicroseconds(10)`
  - `pulseIn(PIN_ECHO, HIGH, 30000)`
  - `float distance = echoTime * 0.0343 / 2.0`
- 每个函数的作用、写法、参数和返回值：
  - `pulseIn(PIN_ECHO, HIGH, 30000)`
    - 函数作用：测量 `ECHO` 引脚上的高电平持续时间。
    - 参数1：测量引脚。
    - 参数2：要测量的电平类型。
    - 参数3：超时时间，单位微秒。
    - 返回值：返回脉冲持续时间，单位微秒。
  - `readDistance()`
    - 函数作用：完成一次完整超声波测距。
    - 参数：无。
    - 返回值：返回距离值。
- 关键代码片段分析：
  - `distance == 0` 在这套课程代码里用来表示“没有收到有效回波”，所以显示时不能直接打印成 `0.0 cm`。
- 为什么这样写：
  - OLED 比串口更适合做距离显示界面，学生可以直接把它理解为简易测距仪表盘。

---

## 实验5

### 1. 实验5预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验用 `WS2812` 显示颜色，并在 OLED 上同步显示颜色名称。
  - `rgb.show()` 必须调用后，颜色设置才会真正生效。
  - 这节使用较低亮度值，避免灯太亮。
- 本实验新增的术语、模块或函数：
  - `Adafruit_NeoPixel`
  - `setPixelColor()`
  - `Color()`
  - `rgb.show()`
  - `colorNumber`
- 开始本实验前需要特别提醒的点：
  - `PIN_RGB = GPIO 48`
  - 如果该引脚和其他外设冲突，要按当前实验重新接线。

### 2. 实验5接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| OLED `SDA` | `GPIO 8` | I2C 数据线 |
| OLED `SCL` | `GPIO 9` | I2C 时钟线 |
| WS2812 `DIN` | `GPIO 48` | 数据输入 |
| WS2812 `GND` | `GND` | 共地 |
| WS2812 `VCC` | 按模块标注 | 供电 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：控制 WS2812 彩灯循环显示颜色，并在 OLED 上显示当前 RGB 数值。
  使用方法：OLED 接 GPIO 8/9、WS2812 数据端接 GPIO 48，上传程序后直接观察。
  运行结果：彩灯循环变色，OLED 同步显示当前颜色名称或 RGB 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define PIN_SDA 8
#define PIN_SCL 9
#define PIN_RGB 48

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel rgb(1, PIN_RGB, NEO_GRB + NEO_KHZ800);

int colorNumber = 0;

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED 初始化失败");

    while (true) {
      delay(1000);
    }
  }

  // 初始化 WS2812，并先清空灯光
  rgb.begin();
  rgb.clear();
  rgb.show();
}

void loop() {
  String colorName;

  // 根据 colorNumber 选择当前显示的颜色
  if (colorNumber == 0) {
    rgb.setPixelColor(0, rgb.Color(30, 0, 0));
    colorName = "RED";
  } else if (colorNumber == 1) {
    rgb.setPixelColor(0, rgb.Color(0, 30, 0));
    colorName = "GREEN";
  } else {
    rgb.setPixelColor(0, rgb.Color(0, 0, 30));
    colorName = "BLUE";
  }

  // 刷新彩灯颜色
  rgb.show();

  // 在 OLED 上显示当前颜色名称
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("RGB COLOR");

  display.setTextSize(2);
  display.setCursor(0, 24);
  display.println(colorName);
  display.display();

  // 让颜色编号按 0 -> 1 -> 2 循环
  colorNumber = colorNumber + 1;

  if (colorNumber >= 3) {
    colorNumber = 0;
  }

  delay(1000);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化 OLED 和 `WS2812`。
  2. 根据 `colorNumber` 选择红、绿、蓝三种颜色之一。
  3. 刷新彩灯显示。
  4. 在 OLED 上显示当前颜色名称。
  5. 让颜色编号循环递增。
- `setup()` 的作用：
  - 初始化 OLED 和 RGB 灯。
- `loop()` 的作用：
  - 周期性切换 RGB 颜色并刷新 OLED。
- 数据输入、处理、输出的执行过程：
  - 输入：无外部传感器输入。
  - 处理：根据 `colorNumber` 选择颜色和文字名称。
  - 输出：WS2812 发光，OLED 显示颜色名称。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `rgb.setPixelColor(0, rgb.Color(30, 0, 0))`
  - `rgb.show()`
  - `String colorName`
- 每个函数的作用、写法、参数和返回值：
  - `rgb.Color(30, 0, 0)`
    - 函数作用：生成一个 RGB 颜色值。
    - 参数1：红色亮度。
    - 参数2：绿色亮度。
    - 参数3：蓝色亮度。
    - 返回值：返回合成后的颜色值。
  - `rgb.setPixelColor(0, color)`
    - 函数作用：设置指定灯珠的颜色。
    - 参数1：灯珠编号。
    - 参数2：颜色值。
    - 返回值：无返回值。
  - `rgb.show()`
    - 函数作用：把设置好的颜色真正发送给灯珠。
    - 参数：无。
    - 返回值：无返回值。
- 关键代码片段分析：
  - 先 `setPixelColor()` 再 `show()`，这组顺序和 OLED 的“先写缓冲区再 `display()`”很像，学生可以对比理解。
- 为什么这样写：
  - OLED 和 RGB 一起使用时，学生能同时看到“文字显示”和“灯光反馈”两种界面输出方式。

---

## 实验6

### 1. 实验6预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验会同时使用 OLED、`MPU6050`、红外、超声波和 RGB。
  - 数据来源多了之后，程序结构更依赖“先采集、再判断、最后集中显示”。
  - 超声波距离不仅要显示在 OLED 上，还要决定 RGB 灯颜色。
- 本实验新增的术语、模块或函数：
  - `stopProgram()`
  - `irDetected`
  - `colorName`
  - 综合状态显示
- 开始本实验前需要特别提醒的点：
  - 所有模块必须共地。
  - `MPU6050` 校准时仍然要保持静止。
  - RGB 颜色规则与距离阈值有关，后续可以自己修改。

### 2. 实验6接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| OLED `SDA` | `GPIO 8` | 与 `MPU6050 SDA` 并联 |
| OLED `SCL` | `GPIO 9` | 与 `MPU6050 SCL` 并联 |
| `MPU6050 SDA` | `GPIO 8` | I2C 数据线 |
| `MPU6050 SCL` | `GPIO 9` | I2C 时钟线 |
| 红外 `OUT` | `GPIO 40` | 数字输入 |
| 超声波 `TRIG` | `GPIO 12` | 触发引脚 |
| 超声波 `ECHO` | `GPIO 13` | 分压后接入 |
| RGB `DIN` | `GPIO 48` | 数据输入 |
| 所有 `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
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

  // 初始化红外、超声波引脚
  pinMode(PIN_IR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // 启动 I2C，总线既服务 OLED 又服务 MPU6050
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  // 初始化 OLED
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

  // 初始化 MPU6050 并检查连接
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

  // 初始化 RGB 灯
  rgb.begin();
  rgb.clear();
  rgb.show();
}

void loop() {
  if (readDmpAngles() == false) {
    delay(10);
    return;
  }

  // 读取其他传感器状态
  bool irDetected = digitalRead(PIN_IR) == LOW;
  float distance = readDistance();
  String colorName;

  // 根据距离决定 RGB 颜色
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

  // 把多种信息集中显示在 OLED 上
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
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化 OLED、`MPU6050`、红外、超声波和 RGB。
  2. 完成 DMP 初始化和校准。
  3. 在循环中先读取姿态角。
  4. 再读取红外状态和超声波距离。
  5. 根据距离决定 RGB 颜色。
  6. 把所有状态集中显示到 OLED 上。
- `setup()` 的作用：
  - 初始化全部模块，并完成姿态校准。
- `loop()` 的作用：
  - 周期性采集多路信息并刷新综合界面。
- 数据输入、处理、输出的执行过程：
  - 输入：`MPU6050` 姿态角、红外状态、超声波距离。
  - 处理：按距离生成 RGB 颜色和颜色名称。
  - 输出：OLED 综合显示，RGB 灯同步反馈。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `bool irDetected = digitalRead(PIN_IR) == LOW`
  - `float distance = readDistance()`
  - `rgb.setPixelColor(0, rgb.Color(...))`
  - `stopProgram(String message)`
- 每个函数的作用、写法、参数和返回值：
  - `stopProgram(String message)`
    - 函数作用：串口和 OLED 同时显示错误信息，然后停止程序。
    - 参数1：错误信息字符串。
    - 返回值：无返回值。
  - `readDistance()`
    - 函数作用：执行一次超声波测距。
    - 参数：无。
    - 返回值：返回距离值。
  - `digitalRead(PIN_IR) == LOW`
    - 作用：把红外电平转换成布尔状态。
- 关键代码片段分析：
  - `if (distance <= 20) ... else if (distance <= 50) ... else ...` 这一段把“测到的数值”变成了“更适合人理解的颜色反馈”。
- 为什么这样写：
  - 这就是一个典型的综合小系统：同一块板子同时读多个传感器、显示多种信息，并做简单联动控制。

---

## 总结
- 实验目标：
  - 学会初始化 OLED 并显示基础文字。
  - 学会在 OLED 上显示 `MPU6050` 姿态角。
  - 学会在 OLED 上显示红外状态和超声波距离。
  - 学会在 OLED 上显示 RGB 颜色信息。
  - 学会完成多模块综合显示程序。
- 单词：
  - `display`：显示
  - `screen`：屏幕
  - `clear`：清除 / 无遮挡
  - `cursor`：光标
  - `text`：文本
  - `color`：颜色
  - `white`：白色
  - `distance`：距离
  - `detected`：检测到
  - `integrated`：整合的
  - `dashboard`：仪表盘
  - `error`：错误
  - `address`：地址
  - `refresh`：刷新
  - `yellow`：黄色
- 接线：
  - `OLED SDA -> GPIO 8`
  - `OLED SCL -> GPIO 9`
  - `MPU6050 SDA -> GPIO 8`
  - `MPU6050 SCL -> GPIO 9`
  - `IR OUT -> GPIO 40`
  - `TRIG -> GPIO 12`
  - `ECHO -> GPIO 13(分压后接入)`
  - `RGB DIN -> GPIO 48`
  - 所有模块 `GND` 必须共地
- 关键函数和参数：
  - `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)`
  - `display.clearDisplay()`
  - `display.setCursor(x, y)`
  - `display.display()`
  - `mpu.dmpInitialize()`
  - `digitalRead(PIN_IR)`
  - `pulseIn(PIN_ECHO, HIGH, 30000)`
  - `rgb.setPixelColor()`
  - `rgb.show()`
- 关键程序：
  - OLED 基础显示程序
  - OLED + `MPU6050`
  - OLED + 红外
  - OLED + 超声波
  - OLED + RGB
  - OLED 综合仪表盘程序
- 实验现象：
  - OLED 会显示文字、状态和传感器数值。
  - RGB 灯会按程序切换颜色。
  - 整合程序里 OLED 和 RGB 会联动变化。
- 本课重点：
  - `display.display()` 是 OLED 刷新的关键。
  - OLED 和 `MPU6050` 可以共用 I2C 总线。
  - 综合实验本质是“多模块输入 + 统一界面输出 + 联动反馈”。
