# 第二模块 实验

> 安全提醒：舵机必须使用合适的独立电源，独立电源 GND 要与 ESP32 GND 相连。第一次上电前请老师检查接线。

## 实验1 串口控制舵机

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 串口命令由字母 `g` 和角度组成，例如 `g90` 表示“转到 90°”。
  - 程序会把角度限制在 `0°～180°`，再换算成 LEDC 的 Duty 数值。
- 本实验新增的术语、模块或函数：
  - `ledcAttachChannel()`：把引脚、频率、分辨率和 LEDC 通道连接起来。
  - `ledcWriteChannel()`：向指定 LEDC 通道写入 Duty。
  - `map()`：把一个范围中的数换算到另一个范围。
  - `constrain()`：把数值关在规定范围内。
- 开始本实验前需要特别提醒的点：
  - 串口设为 `115200`，行结尾选择 `Newline`。
  - 先不装机械负载，发送 `g90` 检查舵机是否正常。

### 2. 实验1接线
- 接线表：

| 舵机线路 | 连接位置 | 说明 |
| --- | --- | --- |
| 信号线 | `GPIO 4` | 只传送控制信号 |
| 电源线 | 独立舵机电源正极 | 不接 GPIO |
| GND | 独立电源 GND | 同时与 ESP32 GND 共地 |
| USB 数据线 | ESP32 与电脑 | 上传程序和发送命令 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 ESP32 LEDC 产生 50Hz 舵机信号，通过串口命令设置 0～180°目标角度。
  使用方法：舵机信号接 GPIO4，串口设为 115200 和 Newline，发送 g0、g90 或 g180。
  运行结果：舵机转到指定目标角度，串口显示 Angle 和对应的 14 位 Duty 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

const int SERVO_FREQUENCY = 50;
const int SERVO_RESOLUTION = 14;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  ledcAttachChannel(
    SERVO_PIN,
    SERVO_FREQUENCY,
    SERVO_RESOLUTION,
    SERVO_CHANNEL
  );

  setServoAngle(90);
  Serial.println("命令：g0、g90、g180");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("g")) {
      int angle = command.substring(1).toInt();
      setServoAngle(angle);
    } else {
      Serial.println("格式错误，请输入 g0～g180");
    }
  }
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  int duty = map(angle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);

  Serial.print("Angle:");
  Serial.print(angle);
  Serial.print("  Duty:");
  Serial.println(duty);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 打开串口，并把 `GPIO 4` 连接到 `50Hz`、`14` 位的 LEDC 通道。
  2. 开机先让舵机去往 `90°`。
  3. 主循环等待串口命令。
  4. 收到 `g90` 时，取出字母后面的 `90`。
  5. 把角度限制、换算，再写入 LEDC 通道。
- `setup()` 的作用：完成串口和舵机信号初始化，只运行一次。
- `loop()` 的作用：反复检查有没有新命令。
- 数据输入、处理、输出的执行过程：
  - 输入：串口命令，例如 `g45`。
  - 处理：取出 `45`，限制到安全角度，再映射成 Duty。
  - 输出：舵机转动，串口显示 `Angle` 和 `Duty`。

### 5. 核心函数及代码讲解
- `ledcAttachChannel(pin, frequency, resolution, channel)`：设置输出引脚、频率、分辨率和通道；本实验是 `GPIO 4`、`50Hz`、`14` 位、通道 `0`。
- `constrain(angle, 0, 180)`：小于 `0` 就按 `0` 处理，大于 `180` 就按 `180` 处理，返回限制后的数值。
- `map(angle, 0, 180, 410, 2048)`：把角度从 `0～180` 换算到 Duty 的 `410～2048`。
- `ledcWriteChannel(channel, duty)`：把 Duty 写入通道，无返回值。
- `command.substring(1).toInt()`：跳过第一个字母 `g`，把后面的文字变成整数。
- 为什么这样写：把“读取命令”和“控制舵机”分开，程序更像两块积木，后面接按键或屏幕时还能继续使用 `setServoAngle()`。

---

## 实验2 TFT 显示舵机状态

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - TFT 画面分成不变的标题、边框，以及会变化的角度、Duty、进度条。
  - 更新数字前先用黑色矩形擦掉旧数字，可以避免文字叠在一起。
- 本实验新增的术语、模块或函数：
  - `Adafruit_ST7789`、`fillRect()`、`drawRect()`、`setCursor()`。
  - `currentAngle`：保存当前命令角度。
- 开始本实验前需要特别提醒的点：
  - 先安装 `Adafruit GFX Library` 和 `Adafruit ST7735 and ST7789 Library`。
  - TFT 和舵机同时工作时更要注意供电稳定与共地。

### 2. 实验2接线
- 接线表：

| 元件 | 引脚 | ESP32-S3 |
| --- | --- | --- |
| 舵机 | 信号 | `GPIO 4` |
| TFT | CS | `GPIO 47` |
| TFT | DC | `GPIO 21` |
| TFT | MOSI / SDA | `GPIO 20` |
| TFT | SCK / SCL | `GPIO 19` |
| TFT | RST | `-1`，本程序不单独连接 |
| 所有模块 | GND | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：通过串口控制舵机，并在 ST7789 TFT 上显示目标角度、Duty 和进度条。
  使用方法：舵机信号接 GPIO4、TFT 按固定引脚连接；串口发送 g0、g90 或 g180。
  运行结果：舵机转到目标位置，TFT 数字和角度进度条同步更新。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

const int SERVO_FREQUENCY = 50;
const int SERVO_RESOLUTION = 14;

const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;

Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

int currentAngle = 90;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  ledcAttachChannel(SERVO_PIN, 50, 14, SERVO_CHANNEL);

  screen.init(240, 320);
  screen.setRotation(3);
  screen.setTextWrap(false);
  drawStaticScreen();

  setServoAngle(currentAngle);
  Serial.println("命令：g0、g90、g180");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("g")) {
      int angle = command.substring(1).toInt();
      setServoAngle(angle);
    }
  }
}

void setServoAngle(int angle) {
  currentAngle = constrain(angle, 0, 180);
  int duty = map(currentAngle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);
  updateScreen(currentAngle, duty);
}

void drawStaticScreen() {
  screen.fillScreen(ST77XX_BLACK);
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(65, 15);
  screen.print("SERVO MONITOR");
  screen.drawFastHLine(0, 45, 320, ST77XX_BLUE);

  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(25, 72);
  screen.print("Angle:");
  screen.setCursor(25, 112);
  screen.print("Duty:");
  screen.drawRect(25, 165, 270, 30, ST77XX_WHITE);
  screen.setTextSize(1);
  screen.setCursor(25, 205);
  screen.print("0 deg");
  screen.setCursor(262, 205);
  screen.print("180");
}

void updateScreen(int angle, int duty) {
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_YELLOW);

  screen.fillRect(130, 68, 170, 25, ST77XX_BLACK);
  screen.setCursor(140, 72);
  screen.print(angle);
  screen.print(" deg");

  screen.fillRect(130, 108, 170, 25, ST77XX_BLACK);
  screen.setCursor(140, 112);
  screen.print(duty);

  screen.fillRect(26, 166, 268, 28, ST77XX_BLACK);
  int barWidth = map(angle, 0, 180, 0, 268);
  screen.fillRect(26, 166, barWidth, 28, ST77XX_GREEN);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：初始化串口和舵机；初始化 TFT；画固定界面；读取角度命令；控制舵机；更新数字和进度条。
- `setup()` 的作用：准备硬件，并把不会变化的画面画好。
- `loop()` 的作用：等待并解析新的 `g角度` 命令。
- 数据输入、处理、输出的执行过程：串口角度 → 限制并换算 → 写入舵机信号 → 更新 TFT。

### 5. 核心函数及代码讲解
- `screen.init(240, 320)`：按 `240×320` 分辨率初始化屏幕。
- `drawStaticScreen()`：只画一次标题、标签和边框，减少重复刷新。
- `updateScreen(angle, duty)`：只更新会变化的区域。
- `fillRect(x, y, width, height, color)`：画实心矩形；先画黑色就是“用背景色擦除”。
- `barWidth = map(angle, 0, 180, 0, 268)`：角度越大，绿色进度条越长。
- 为什么这样写：屏幕只刷新必要的小区域，画面更稳定，程序也不用每次重画整页。

---

## 实验3 按键控制舵机

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - `INPUT_PULLUP` 模式下，没按按键是 `HIGH`，按下后是 `LOW`。
  - 机械按键按下一次时可能产生多次快速跳变，程序要做消抖。
- 本实验新增的术语、模块或函数：
  - `lastButtonState`：上一次读取到的按键状态。
  - `lastDebounceTime`：上一次有效按键发生的时间。
  - `ANGLE_STEP`：每次增加的角度步长。
- 开始本实验前需要特别提醒的点：
  - 按键一端接 `GPIO 5`，另一端接 `GND`，程序使用芯片内部上拉功能。
  - 观察一次按下是否只增加 `15°`。

### 2. 实验3接线
- 接线表：

| 元件 | 连接位置 | 说明 |
| --- | --- | --- |
| 舵机信号 | `GPIO 4` | 舵机仍用独立电源 |
| 按键一端 | `GPIO 5` | 使用内部上拉 |
| 按键另一端 | `GND` | 按下时读到 `LOW` |
| 舵机电源 GND | ESP32 `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用按键消抖控制舵机角度，每次有效按键让目标角度增加 15°。
  使用方法：舵机信号接 GPIO4，按键接 GPIO5 和 GND；上传后逐次按下并松开按键。
  运行结果：舵机从 0°开始每次增加 15°，超过 180°后回到 0°，串口显示当前角度。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

const int BUTTON_PIN = 5;
const int ANGLE_STEP = 15;

int currentAngle = 0;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  ledcAttachChannel(SERVO_PIN, 50, 14, SERVO_CHANNEL);
  setServoAngle(currentAngle);
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);
  unsigned long currentTime = millis();

  // 只在松开变为按下，并且距离上次有效动作超过 50ms 时处理
  if (lastButtonState == HIGH && buttonState == LOW
      && currentTime - lastDebounceTime >= 50) {
    lastDebounceTime = currentTime;
    currentAngle = currentAngle + ANGLE_STEP;

    if (currentAngle > 180) {
      currentAngle = 0;
    }

    setServoAngle(currentAngle);
  }

  lastButtonState = buttonState;
}

void setServoAngle(int angle) {
  int duty = map(angle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);

  Serial.print("Angle:");
  Serial.println(angle);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：初始化按键和舵机；读取按键；寻找 `HIGH -> LOW` 的按下瞬间；检查是否超过 `50ms`；角度增加；控制舵机。
- `setup()` 的作用：打开串口，把按键设为内部上拉输入，并让舵机从 `0°` 开始。
- `loop()` 的作用：不停查看按键有没有产生一次新的有效按下。
- 数据输入、处理、输出的执行过程：按键电平 → 边沿与时间判断 → 角度增加或回到零 → 舵机和串口输出。

### 5. 核心函数及代码讲解
- `pinMode(BUTTON_PIN, INPUT_PULLUP)`：启用芯片内部上拉，按键没按时保持高电平。
- `lastButtonState == HIGH && buttonState == LOW`：只抓住“刚刚按下”的瞬间，而不是一直按住时反复增加。
- `currentTime - lastDebounceTime >= 50`：两次有效动作至少间隔 `50ms`，过滤抖动。
- `currentAngle = currentAngle + ANGLE_STEP`：每次加 `15°`。
- 为什么这样写：边沿检测和消抖一起使用，一次按下更接近只执行一次动作。

---

## 实验4 舵机扫描与数据采集

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 程序每 `100ms` 改变一次角度，不使用 `delay()` 阻塞主循环。
  - `angleStep` 为 `5` 时角度增加，为 `-5` 时角度减小。
  - CSV 是用逗号隔开每一列的数据格式。
- 本实验新增的术语、模块或函数：
  - `Time,Angle,Duty`：时间、命令角度、Duty 三列数据。
  - 往返扫描：到 `180°` 后反向，到 `0°` 后再次反向。
- 开始本实验前需要特别提醒的点：
  - 先确认舵机能安全到达两端；若端点嗡鸣，应缩小范围后再自动扫描。
  - 这些数据表示程序发出的命令，不代表舵机轴真实到达的位置。

### 2. 实验4接线
- 接线表与实验1相同：信号 `GPIO 4`，舵机独立供电，所有 GND 共地。
- USB 数据线连接电脑，用 `115200` 波特率读取或保存串口数据。

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：让舵机自动往返扫描，并通过串口输出时间、命令角度和 PWM Duty 数据。
  使用方法：舵机信号接 GPIO4，打开 115200 串口绘图器或串口工具记录 CSV 数据。
  运行结果：舵机在 0～180 度之间往返，串口每 100ms 输出 Time,Angle,Duty。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

int servoAngle = 0;
int angleStep = 5;
unsigned long lastMoveTime = 0;

void setup() {
  Serial.begin(115200);
  ledcAttachChannel(SERVO_PIN, 50, 14, SERVO_CHANNEL);
  setServoAngle(servoAngle);

  Serial.println("Time,Angle,Duty");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMoveTime >= 100) {
    servoAngle = servoAngle + angleStep;

    if (servoAngle >= 180) {
      servoAngle = 180;
      angleStep = -5;
    }

    if (servoAngle <= 0) {
      servoAngle = 0;
      angleStep = 5;
    }

    int duty = setServoAngle(servoAngle);

    Serial.print(currentTime);
    Serial.print(",");
    Serial.print(servoAngle);
    Serial.print(",");
    Serial.println(duty);

    lastMoveTime = currentTime;
  }
}

int setServoAngle(int angle) {
  int duty = map(angle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);
  return duty;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：初始化；每隔 `100ms` 改变角度；到达端点后把步长反向；输出控制信号；打印一行 CSV 数据。
- `setup()` 的作用：准备串口和舵机，并打印表头。
- `loop()` 的作用：用 `millis()` 定时完成往返扫描。
- 数据输入、处理、输出的执行过程：时间 → 计算新角度和方向 → 换算 Duty → 舵机动作并输出 CSV。

### 5. 核心函数及代码讲解
- `angleStep = -5`：负数表示接下来每次都减小 `5°`。
- `if (servoAngle >= 180)` 与 `if (servoAngle <= 0)`：到达两端时修正角度，并掉头。
- `int setServoAngle(int angle)`：函数返回 `int` 类型 Duty，控制舵机的同时把结果交给串口打印。
- `Serial.print(",")`：在三项数据之间放逗号，形成 CSV。
- 为什么这样写：同一份 Duty 既用于控制舵机，又用于记录，避免重复计算或记录错数。

---

## 总结
- 实验目标：会用串口、TFT、按键和自动扫描四种方式控制或观察舵机。
- 单词：`servo` 舵机、`angle` 角度、`duty` Duty 数值、`frequency` 频率、`channel` 通道、`debounce` 消抖、`capture` 采集。
- 接线：舵机信号接 `GPIO 4`；按键接 `GPIO 5` 和 GND；舵机独立供电；所有 GND 共地。
- 关键函数和参数：`ledcAttachChannel(GPIO 4, 50, 14, 0)`、`ledcWriteChannel()`、`map()`、`constrain()`、`millis()`。
- 关键程序：角度换算函数 `setServoAngle()`、串口命令解析、按键边沿消抖、非阻塞往返扫描。
- 本课重点：
  - 舵机控制信号和舵机供电不是一回事。
  - `50Hz` 和安全 Duty 范围是稳定控制的关键。
  - 命令角度不等于真实测量角度。
  - 出现端点嗡鸣、堵转或供电异常时必须立即断电。
