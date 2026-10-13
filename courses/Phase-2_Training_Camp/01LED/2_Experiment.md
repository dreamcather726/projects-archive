# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 这是本课的基础语法预备实验，重点不在硬件现象，而在理解程序结构。
  - 串口监视器可以帮助我们观察程序运行结果。
  - `int`、`float`、`char`、`bool` 是最基础的数据类型。
- 本实验新增的术语、模块或函数：
  - `Serial.begin()`
  - `Serial.println()`
  - `if`
  - `while`
  - `for`
- 开始本实验前需要特别提醒的点：
  - 上传后要打开波特率为 `115200` 的串口监视器。
  - 本实验不用额外接外设。

### 2. 实验1接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| USB 数据线 | USB 接口 | 连接电脑下载程序并打开串口监视器 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：演示 Arduino 的 setup()、loop() 和串口输出等基础语法。
  使用方法：上传程序后，打开波特率为 115200 的串口监视器，无需发送数据。
  运行结果：串口每隔 3 秒显示变量、if 判断、while 循环和 for 循环的示例结果。
*/

// setup 只在开发板启动时运行一次
void setup() {
  // 启动串口，115200 是通信速度
  Serial.begin(115200);
}

// loop 会一直重复运行
void loop() {
  // int 用来保存整数
  int age = 12;

  // float 用来保存小数
  float voltage = 3.3;

  // char 用来保存一个字符，字符使用单引号
  char grade = 'A';

  // bool 只有 true 和 false 两种值
  bool ready = true;

  // println 输出内容后会自动换行
  Serial.println("ESP32-S3 start");
  Serial.println(age);
  Serial.println(voltage);
  Serial.println(grade);

  // if 判断条件是否成立
  if (ready == true) {
    Serial.println("Ready!");
  }

  // while 会在条件成立时重复运行
  int count = 0;
  while (count < 3) {
    Serial.println("while");
    count = count + 1;
  }

  // for 把循环需要的三个部分写在同一行
  for (int i = 0; i < 3; i = i + 1) {
    Serial.println("for");
  }

  // 等待 3 秒，避免串口输出得太快
  delay(3000);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中启动串口。
  2. 在 `loop()` 中定义变量。
  3. 用 `Serial.println()` 输出变量值。
  4. 演示 `if`、`while`、`for` 的运行过程。
  5. 延时 3 秒后再次重复。
- `setup()` 的作用：
  - 启动串口通信。
- `loop()` 的作用：
  - 不断重复执行变量定义、条件判断、循环和串口输出。
- 数据输入、处理、输出的执行过程：
  - 输入：本实验无外部输入。
  - 处理：程序内部完成变量计算和循环判断。
  - 输出：把结果打印到串口监视器。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Serial.begin(115200)`
  - `Serial.println(...)`
  - `if (ready == true)`
  - `while (count < 3)`
  - `for (int i = 0; i < 3; i = i + 1)`
- 每个函数的作用、写法、参数和返回值：
  - `Serial.begin(115200)`
    - 函数作用：启动串口通信。
    - 参数1：`115200`，表示串口波特率。
    - 返回值：无返回值。
  - `Serial.println(age)`
    - 函数作用：向串口监视器输出内容，并自动换行。
    - 参数1：可以是字符串、数字或变量。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `if (ready == true)`：只有条件成立时，才执行花括号中的语句。
  - `while (count < 3)`：条件成立时会反复执行。
  - `for (int i = 0; i < 3; i = i + 1)`：适合做固定次数循环。
- 为什么这样写：
  - 这是后续所有硬件实验的基础程序认知入口。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 想让 LED 亮，必须先把引脚设置为输出模式。
  - `HIGH` 表示高电平，`LOW` 表示低电平。
  - `delay(1000)` 表示延时 `1000ms`。
- 本实验新增的术语、模块或函数：
  - `pinMode()`
  - `digitalWrite()`
  - `delay()`
- 开始本实验前需要特别提醒的点：
  - LED 必须串联限流电阻。
  - 先检查 `GPIO 4` 是否和代码一致。

### 2. 实验2接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 普通 LED | `GPIO 4` | `GPIO 4 -> 220Ω 电阻 -> LED 正极` |
| 普通 LED | `GND` | `LED 负极 -> GND` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：控制普通 LED 周期性点亮和熄灭，实现闪烁效果。
*/

// 把 LED 所连接的引脚编号保存到常量中，后面反复使用
const int LED_PIN = 4;

void setup() {
  // 把 LED 引脚设置为输出模式，只有输出模式才能控制亮灭
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // 输出高电平，LED 点亮
  digitalWrite(LED_PIN, HIGH);

  // 保持点亮 1000 毫秒，也就是 1 秒
  delay(1000);

  // 输出低电平，LED 熄灭
  digitalWrite(LED_PIN, LOW);

  // 保持熄灭 1000 毫秒
  delay(1000);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义 LED 引脚。
  2. 在 `setup()` 中把 LED 引脚设置为输出。
  3. 在 `loop()` 中先点亮 LED，再延时 `1` 秒。
  4. 然后熄灭 LED，再延时 `1` 秒。
  5. 重复执行，形成闪烁现象。
- `setup()` 的作用：
  - 负责初始化 LED 引脚模式。
- `loop()` 的作用：
  - 负责反复执行亮灭控制。
- 数据输入、处理、输出的执行过程：
  - 本实验没有外部输入，程序直接按固定时间输出高低电平。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `pinMode(LED_PIN, OUTPUT)`
  - `digitalWrite(LED_PIN, HIGH)`
  - `digitalWrite(LED_PIN, LOW)`
  - `delay(1000)`
- 每个函数的作用、写法、参数和返回值：
  - `pinMode(LED_PIN, OUTPUT)`：设置引脚为输出模式。
  - `digitalWrite(LED_PIN, HIGH)`：输出高电平点亮 LED。
  - `digitalWrite(LED_PIN, LOW)`：输出低电平熄灭 LED。
  - `delay(1000)`：暂停 1000 毫秒。
- 关键代码片段分析：
  - 先输出 `HIGH` 再输出 `LOW`，中间用 `delay()` 控制时间。
- 为什么这样写：
  - 这是最基础的 GPIO 输出实验。

---

## 实验3

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 按键可以作为输入设备。
  - `INPUT_PULLUP` 表示内部上拉输入模式。
  - 在这种写法下，按下时读到的是 `LOW`。
- 本实验新增的术语、模块或函数：
  - `digitalRead()`
  - `INPUT_PULLUP`
  - 消抖
- 开始本实验前需要特别提醒的点：
  - 按键一端接 `GPIO 5`，另一端接 `GND`。

### 2. 实验3接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 普通 LED | `GPIO 4` | `GPIO 4 -> 220Ω 电阻 -> LED 正极` |
| 普通 LED | `GND` | `LED 负极 -> GND` |
| 按键 | `GPIO 5` | 按键一端接 `GPIO 5` |
| 按键 | `GND` | 按键另一端接 `GND` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：读取按键状态，每按一次按键切换一次 LED 的亮灭状态。
*/

// LED 接在 GPIO 4
const int LED_PIN = 4;

// 按键接在 GPIO 5
const int BUTTON_PIN = 5;

// 保存 LED 当前状态，false 表示灭，true 表示亮
bool ledState = false;

// 记录按键上一次的状态，开始时默认松开
int lastButton = HIGH;

void setup() {
  // LED 负责输出电平，所以设置为输出模式
  pinMode(LED_PIN, OUTPUT);

  // 按键使用内部上拉输入模式，松开时读到 HIGH，按下时读到 LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 程序启动时先让 LED 熄灭
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // 每次循环都读取按键结果，并把结果直接写给 LED
  digitalWrite(LED_PIN, button_read());
}

int button_read() {
  // 读取当前按键电平
  int buttonState = digitalRead(BUTTON_PIN);

  // 简单延时消抖，减少机械按键抖动带来的误判
  delay(50);

  // 只有当按键从“松开”变成“按下”时，才切换一次 LED 状态
  if (lastButton == HIGH && buttonState == LOW) {
    // 取反当前 LED 状态：亮变灭，灭变亮
    ledState = !ledState;
  }

  // 更新“上一次状态”，为下一次判断做准备
  lastButton = buttonState;

  // 返回当前 LED 状态给主循环
  return ledState;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义 LED 引脚、按键引脚和状态变量。
  2. 在 `setup()` 中配置 LED 为输出，按键为内部上拉输入。
  3. 在 `loop()` 中不断调用 `button_read()`。
  4. 检测到按键由松开变为按下时，切换 `ledState`。
  5. 把新的状态写给 LED。
- `setup()` 的作用：
  - 初始化输入输出模式。
- `loop()` 的作用：
  - 持续扫描按键并更新 LED。
- 数据输入、处理、输出的执行过程：
  - 输入：按键状态。
  - 处理：判断是否为一次有效按下。
  - 输出：根据 `ledState` 控制 LED 亮灭。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `pinMode(BUTTON_PIN, INPUT_PULLUP)`
  - `digitalRead(BUTTON_PIN)`
  - `button_read()`
- 每个函数的作用、写法、参数和返回值：
  - `pinMode(BUTTON_PIN, INPUT_PULLUP)`：把按键引脚设置为内部上拉输入。
  - `digitalRead(BUTTON_PIN)`：读取按键当前电平。
  - `button_read()`：完成读取按键、消抖和切换 LED 状态的完整过程，返回当前 LED 状态。
- 关键代码片段分析：
  - `if (lastButton == HIGH && buttonState == LOW)`：只在按下瞬间切换一次状态。
- 为什么这样写：
  - 这个实验把输入和输出结合起来。

---

## 实验4

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - PWM 可以通过改变占空比让 LED 呈现不同亮度。
  - `for` 循环适合控制亮度逐步增加和逐步减少。
- 本实验新增的术语、模块或函数：
  - `analogWrite()`
  - PWM
  - 占空比
- 开始本实验前需要特别提醒的点：
  - 呼吸灯依赖 `0~255` 的亮度变化。

### 2. 实验4接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 普通 LED | `GPIO 4` | `GPIO 4 -> 220Ω 电阻 -> LED 正极` |
| 普通 LED | `GND` | `LED 负极 -> GND` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 PWM 逐渐改变 LED 亮度，实现呼吸灯效果。
*/

// LED 连接在 GPIO 4
const int LED_PIN = 4;

void setup() {
  // 把 LED 引脚设为输出模式
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // light 从 0 逐步增加到 255，LED 会越来越亮
  for (int light = 0; light <= 255; light = light + 5) {
    // 输出当前 PWM 数值
    analogWrite(LED_PIN, light);

    // 延时越长，亮度变化越慢
    delay(30);
  }

  // light 从 255 逐步减小到 0，LED 会越来越暗
  for (int light = 255; light >= 0; light = light - 5) {
    // 继续输出 PWM 数值，但这次是逐步减小
    analogWrite(LED_PIN, light);

    // 每一步都短暂停留一下，形成平滑的呼吸效果
    delay(30);
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化 LED 引脚为输出。
  2. 用第一个 `for` 循环让亮度从 `0` 增加到 `255`。
  3. 用第二个 `for` 循环让亮度从 `255` 减少到 `0`。
  4. 持续循环，形成呼吸灯效果。
- `setup()` 的作用：
  - 初始化 LED 引脚。
- `loop()` 的作用：
  - 反复执行“变亮”和“变暗”的过程。
- 数据输入、处理、输出的执行过程：
  - 输入：无外部输入。
  - 处理：程序修改变量 `light` 的值。
  - 输出：把 `light` 作为 PWM 值写到 LED 引脚。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `analogWrite(LED_PIN, light)`
  - `for (int light = 0; light <= 255; light = light + 5)`
  - `for (int light = 255; light >= 0; light = light - 5)`
- 每个函数的作用、写法、参数和返回值：
  - `analogWrite(LED_PIN, light)`：输出 PWM 数值，控制 LED 亮度。
  - 两个 `for` 循环：分别控制变亮和变暗过程。
- 关键代码片段分析：
  - 前一个循环负责变亮，后一个循环负责变暗。
- 为什么这样写：
  - 这个实验帮助学生理解 PWM 对亮度的影响。

---

## 实验5

### 1. 实验5预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - RGB 灯本质上是红、绿、蓝三个 LED 组合在一起。
  - 三种颜色的亮度不同，混合后可以得到不同颜色。
- 本实验新增的术语、模块或函数：
  - `setColor()`
  - 混色
  - 公共阴极
- 开始本实验前需要特别提醒的点：
  - RGB 公共端必须接 `GND`。

### 2. 实验5接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| RGB 红色脚 | `GPIO 6` | 串联 `220Ω` 电阻 |
| RGB 绿色脚 | `GPIO 7` | 串联 `220Ω` 电阻 |
| RGB 蓝色脚 | `GPIO 8` | 串联 `220Ω` 电阻 |
| RGB 公共端 | `GND` | 公共阴极接地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：分别控制 RGB 灯的红、绿、蓝通道，显示不同颜色。
*/

// 定义 RGB 三个颜色通道所连接的引脚
const int RED_PIN = 6;
const int GREEN_PIN = 7;
const int BLUE_PIN = 8;

void setup() {
  // 把红、绿、蓝三个引脚都设置为输出模式
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}

void loop() {
  // 红色最亮，绿色和蓝色关闭，显示红色
  setColor(255, 0, 0);
  delay(1000);

  // 绿色最亮，显示绿色
  setColor(0, 255, 0);
  delay(1000);

  // 蓝色最亮，显示蓝色
  setColor(0, 0, 255);
  delay(1000);

  // 三种颜色同时最亮，混合成白色
  setColor(255, 255, 255);
  delay(1000);

  // 三种颜色都关闭，RGB 灯熄灭
  setColor(0, 0, 0);
  delay(1000);
}

void setColor(int red, int green, int blue) {
  // 把红色亮度写到红色引脚
  analogWrite(RED_PIN, red);

  // 把绿色亮度写到绿色引脚
  analogWrite(GREEN_PIN, green);

  // 把蓝色亮度写到蓝色引脚
  analogWrite(BLUE_PIN, blue);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义 RGB 三个颜色通道的引脚。
  2. 在 `setup()` 中全部设置为输出。
  3. 在 `loop()` 中依次调用 `setColor()` 显示不同颜色。
  4. 最后关闭全部颜色并重复。
- `setup()` 的作用：
  - 初始化三路颜色引脚。
- `loop()` 的作用：
  - 反复展示不同颜色效果。
- 数据输入、处理、输出的执行过程：
  - 输入：无外部输入。
  - 处理：程序预设好每种颜色的三通道亮度值。
  - 输出：把三个 PWM 值分别写到红、绿、蓝引脚。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `setColor(int red, int green, int blue)`
  - `analogWrite(RED_PIN, red)`
- 每个函数的作用、写法、参数和返回值：
  - `setColor(int red, int green, int blue)`：同时设置红、绿、蓝三路亮度。
    - 参数1：`red`，红色亮度。
    - 参数2：`green`，绿色亮度。
    - 参数3：`blue`，蓝色亮度。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `setColor(255, 255, 255)`：三种颜色同时最亮，显示白色。
- 为什么这样写：
  - 这个实验帮助学生理解“复杂颜色也是多个简单输出的组合”。

---

## 实验6

### 1. 实验6预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - `WS2812` 内部带控制芯片，通常只需要一根数据线。
  - 使用前要导入 `Adafruit_NeoPixel` 库。
  - 设置颜色后必须调用 `show()` 才会刷新。
- 本实验新增的术语、模块或函数：
  - `#include <Adafruit_NeoPixel.h>`
  - `strip.begin()`
  - `strip.fill()`
  - `strip.show()`
- 开始本实验前需要特别提醒的点：
  - 数据引脚要和程序中的定义一致。
  - 模块必须与开发板共地。

### 2. 实验6接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| WS2812 数据端 | `GPIO 48` | `GPIO 48 -> DIN` |
| WS2812 电源 | `5V/3.3V` | 按模块说明供电 |
| WS2812 地线 | `GND` | 与开发板共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 Adafruit_NeoPixel 库控制 WS2812 彩灯显示不同颜色。
*/

// 导入 WS2812 控制库
#include <Adafruit_NeoPixel.h>

// WS2812 数据输入端连接 GPIO 48
const int LED_PIN = 48;

// 当前示例只控制 1 颗灯珠
const int LED_COUNT = 1;

// 创建灯带对象
// NEO_GRB 表示颜色顺序，NEO_KHZ800 表示通信速度
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // 启动灯带对象
  strip.begin();

  // 清除内存中的颜色数据
  strip.clear();

  // 把“清除颜色”的结果真正发送给灯珠
  strip.show();
}

void loop() {
  // 把所有灯珠设置为红色
  strip.fill(strip.Color(255, 0, 0));

  // 刷新显示，灯珠真正变成红色
  strip.show();
  delay(1000);

  // 把所有灯珠设置为绿色
  strip.fill(strip.Color(0, 255, 0));
  strip.show();
  delay(1000);

  // 把所有灯珠设置为蓝色
  strip.fill(strip.Color(0, 0, 255));
  strip.show();
  delay(1000);

  // 清除所有颜色，灯珠熄灭
  strip.clear();
  strip.show();
  delay(1000);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 导入 `Adafruit_NeoPixel` 库并创建灯带对象。
  2. 在 `setup()` 中初始化灯带并清空颜色。
  3. 在 `loop()` 中依次填充红、绿、蓝三种颜色。
  4. 每次设置颜色后调用 `show()` 刷新。
  5. 最后清空颜色并继续循环。
- `setup()` 的作用：
  - 初始化 `WS2812` 对象。
- `loop()` 的作用：
  - 反复切换灯珠颜色。
- 数据输入、处理、输出的执行过程：
  - 输入：无外部输入。
  - 处理：程序生成颜色数据。
  - 输出：通过数据线把颜色发送给 `WS2812`。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Adafruit_NeoPixel strip(...)`
  - `strip.begin()`
  - `strip.fill(strip.Color(...))`
  - `strip.show()`
- 每个函数的作用、写法、参数和返回值：
  - `Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800)`：创建灯带对象。
  - `strip.begin()`：启动灯带对象。
  - `strip.fill(strip.Color(255, 0, 0))`：把全部灯珠设置为指定颜色。
  - `strip.show()`：把内存中的颜色真正刷新到灯珠上。
- 关键代码片段分析：
  - `strip.fill(...); strip.show();`：先写入颜色，再刷新显示。
- 为什么这样写：
  - 这个实验帮助学生理解普通 RGB 灯和可编程彩灯的区别。

---

## 总结
- 实验目标：
  - 学会基础语法与串口输出。
  - 学会用 GPIO 输出控制 LED。
  - 学会用 GPIO 输入读取按键。
  - 学会用 PWM 改变 LED 亮度。
  - 学会 RGB 灯颜色控制。
  - 学会 `WS2812` 基础显示控制。
- 单词：
  - `setup`：初始化
  - `loop`：循环
  - `serial`：串口
  - `print`：打印 / 输出
  - `println`：打印并换行
  - `pin`：引脚
  - `mode`：模式
  - `button`：按键
  - `blink`：闪烁
  - `input`：输入
  - `output`：输出
  - `digital`：数字的 / 数字电平的
  - `analog`：模拟的
  - `read`：读取
  - `write`：写入 / 输出
  - `light`：灯光 / 亮度
  - `red`：红色
  - `green`：绿色
  - `blue`：蓝色
  - `high`：高
  - `low`：低
  - `delay`：延时
  - `pullup`：上拉
  - `state`：状态
  - `value`：数值 / 值
- 关键函数和参数：
  - `Serial.begin(115200)`
  - `Serial.println(...)`
  - `pinMode(pin, OUTPUT)`
  - `pinMode(pin, INPUT_PULLUP)`
  - `digitalWrite(pin, HIGH/LOW)`
  - `digitalRead(pin)`
  - `analogWrite(pin, value)`
  - `strip.fill(strip.Color(r, g, b))`
  - `strip.show()`
- 关键程序：
  - 基础语法串口程序
  - LED 闪烁程序
  - 按键切换程序
  - 呼吸灯程序
  - RGB 灯颜色程序
  - WS2812 彩灯程序
- 本课重点：
  - 理解 `setup()` 和 `loop()` 的分工。
  - 理解按键按下读到 `LOW` 的原因。
  - 理解 PWM 亮度变化和 RGB / WS2812 颜色控制的差异。
