# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验使用力敏传感器的数值做阈值判断。
  - 当压力值超过设定阈值时，程序输出高电平让蜂鸣器响。
  - 有源蜂鸣器和无源蜂鸣器不同，不需要输出不同频率，直接控制高低电平即可。
- 本实验新增的术语、模块或函数：
  - `analogRead()`
  - `digitalWrite()`
  - 阈值判断
  - `FORCE_LIMIT`
- 开始本实验前需要特别提醒的点：
  - 力敏传感器信号线要接到程序定义的 `GPIO 15`。
  - 蜂鸣器控制端要接到程序定义的 `GPIO 21`。
  - 建议打开 `115200` 波特率串口监视器观察压力数值。

### 2. 实验1接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 力敏传感器信号端 | `GPIO 15` | 信号输出接 `GPIO 15` |
| 力敏传感器电源 | `3.3V` | 模块电源正极接 `3.3V` |
| 力敏传感器地线 | `GND` | 模块地线接 `GND` |
| 有源蜂鸣器控制端 | `GPIO 21` | 控制引脚接 `GPIO 21` |
| 有源蜂鸣器电源 | `VCC/GND` | 按模块标注接电源并共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：读取力敏传感器数值，压力超过设定值时启动有源蜂鸣器报警。
  使用方法：力敏输出接 GPIO 15、蜂鸣器接 GPIO 21，打开 115200 串口后按压力敏片。
  运行结果：串口显示压力值；数值超过 1200 时蜂鸣器鸣叫，松开后停止。
*/

// 力敏传感器的分压输出连接 GPIO 15
#define PIN_FORCE 15

// 有源蜂鸣器模块的控制端连接 GPIO 21
#define PIN_BUZZER 21

// 压力超过这个数值时报警
// 可以根据串口显示的数值进行修改
#define FORCE_LIMIT 1200

void setup() {
  // 启动串口通信，方便观察压力数值
  Serial.begin(115200);

  // 蜂鸣器引脚设置为输出模式
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  // 读取力敏传感器，范围通常是 0～4095
  int force = analogRead(PIN_FORCE);

  // 在串口监视器中观察压力数值
  Serial.println(force);

  // 压力超过设定值，蜂鸣器响
  if (force > FORCE_LIMIT) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    // 没有超过阈值，蜂鸣器停止
    digitalWrite(PIN_BUZZER, LOW);
  }

  // 每隔 100 毫秒读取一次
  delay(100);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中启动串口并把蜂鸣器引脚设置为输出。
  2. 在 `loop()` 中持续读取力敏传感器数值。
  3. 把读取到的压力值打印到串口监视器。
  4. 判断压力值是否超过 `FORCE_LIMIT`。
  5. 超过阈值时启动蜂鸣器，否则关闭蜂鸣器。
- `setup()` 的作用：
  - 初始化串口和蜂鸣器输出引脚。
- `loop()` 的作用：
  - 持续读取压力数值并根据阈值控制蜂鸣器。
- 数据输入、处理、输出的执行过程：
  - 输入：力敏传感器输出的模拟量。
  - 处理：程序用阈值判断当前压力是否达到报警条件。
  - 输出：串口显示压力值，蜂鸣器发声或停止。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `analogRead(PIN_FORCE)`
  - `Serial.println(force)`
  - `if (force > FORCE_LIMIT)`
  - `digitalWrite(PIN_BUZZER, HIGH/LOW)`
- 每个函数的作用、写法、参数和返回值：
  - `analogRead(PIN_FORCE)`
    - 函数作用：读取力敏传感器所在引脚的模拟量数值。
    - 参数1：`PIN_FORCE`，表示力敏传感器连接的模拟输入引脚。
    - 返回值：返回当前读取到的模拟值，常见范围为 `0~4095`。
  - `Serial.println(force)`
    - 函数作用：把压力值输出到串口监视器，并自动换行。
    - 参数1：`force`，表示当前读取到的压力值。
    - 返回值：无返回值。
  - `digitalWrite(PIN_BUZZER, HIGH)`
    - 函数作用：向蜂鸣器控制引脚输出高电平。
    - 参数1：`PIN_BUZZER`，表示蜂鸣器连接引脚。
    - 参数2：`HIGH`，表示输出高电平。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `if (force > FORCE_LIMIT)`：这是最典型的阈值判断，只要数值超过设定值就触发报警。
- 为什么这样写：
  - 力敏传感器的数值本身是连续变化的，但蜂鸣器报警是“开/关”现象，所以最适合使用阈值判断。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验不是简单亮灭，而是达到压力阈值后执行完整的呼吸灯过程。
  - `PWM` 通过输出不同数值让 LED 呈现不同亮度。
  - `for` 循环适合让亮度按固定步长逐渐变化。
- 本实验新增的术语、模块或函数：
  - `analogWrite()`
  - `for`
  - 呼吸灯
  - `FORCE_LIMIT`
- 开始本实验前需要特别提醒的点：
  - 本实验中力敏传感器接 `GPIO 15`，LED 接 `GPIO 2`。
  - 只有压力超过阈值时才会看到呼吸灯效果。

### 2. 实验2接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 力敏传感器信号端 | `GPIO 15` | 信号输出接 `GPIO 15` |
| 力敏传感器电源 | `3.3V` | 模块电源正极接 `3.3V` |
| 力敏传感器地线 | `GND` | 模块地线接 `GND` |
| LED | `GPIO 2` | 若用外接 LED，需串联限流电阻 |
| LED 地线 | `GND` | 负极接 `GND` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：检测压力大小，超过设定值后启动 LED 呼吸灯效果。
  使用方法：力敏输出接 GPIO 15、LED 接 GPIO 2，上传程序后按压力敏片。
  运行结果：压力值超过 1200 时 LED 产生呼吸效果，压力不足时 LED 熄灭。
*/

// 力敏传感器连接 GPIO 15
#define PIN_FORCE 15

// 板载 LED_ON 连接 GPIO 2
#define PIN_LED 2

// 超过这个数值时启动呼吸灯
#define FORCE_LIMIT 1200

void setup() {
  // 把 LED 引脚设置为输出模式
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  // 读取当前压力值
  int force = analogRead(PIN_FORCE);

  // 压力超过阈值后，执行一次完整的呼吸灯效果
  if (force > FORCE_LIMIT) {
    // PWM 从 0 增加到 255，LED 逐渐变亮
    for (int light = 0; light <= 255; light = light + 5) {
      analogWrite(PIN_LED, light);
      delay(20);
    }

    // PWM 从 255 减少到 0，LED 逐渐变暗
    for (int light = 255; light >= 0; light = light - 5) {
      analogWrite(PIN_LED, light);
      delay(20);
    }
  } else {
    // 没有达到设定压力时关闭 LED
    analogWrite(PIN_LED, 0);
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中把 LED 引脚设置为输出。
  2. 在 `loop()` 中持续读取力敏传感器数值。
  3. 判断压力是否超过 `FORCE_LIMIT`。
  4. 超过阈值时，通过两个 `for` 循环完成“变亮”和“变暗”过程。
  5. 未达到阈值时，把 LED 亮度设置为 `0`。
- `setup()` 的作用：
  - 初始化 LED 输出引脚。
- `loop()` 的作用：
  - 持续检测压力大小，并决定是否执行呼吸灯。
- 数据输入、处理、输出的执行过程：
  - 输入：力敏传感器的模拟量。
  - 处理：程序先做阈值判断，再计算不同阶段的 PWM 亮度值。
  - 输出：向 LED 引脚输出不同的 PWM 数值。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `analogRead(PIN_FORCE)`
  - `analogWrite(PIN_LED, light)`
  - `for (int light = 0; light <= 255; light = light + 5)`
  - `for (int light = 255; light >= 0; light = light - 5)`
- 每个函数的作用、写法、参数和返回值：
  - `analogWrite(PIN_LED, light)`
    - 函数作用：向 LED 引脚输出 PWM 数值，控制亮度。
    - 参数1：`PIN_LED`，表示 LED 所连接的输出引脚。
    - 参数2：`light`，表示当前要输出的亮度值，常见范围是 `0~255`。
    - 返回值：无返回值。
  - `for (int light = 0; light <= 255; light = light + 5)`
    - 语句作用：让亮度值按固定步长不断增加。
    - 初始值：`light = 0`
    - 条件：`light <= 255`
    - 变化：`light = light + 5`
- 关键代码片段分析：
  - 两个 `for` 循环分别完成“渐亮”和“渐暗”，组合起来就是呼吸灯。
- 为什么这样写：
  - 力敏传感器负责决定“是否触发”，而呼吸灯本身是一个连续变化过程，所以程序分成“判断条件”和“执行效果”两部分。

---

## 实验3

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验不是简单比较阈值，而是要把压力值转换成亮度值。
  - 力敏传感器原始值范围和 LED 亮度范围不同，必须做范围转换。
  - `constrain()` 先限制范围，`map()` 再进行映射，是很常见的处理流程。
- 本实验新增的术语、模块或函数：
  - `constrain()`
  - `map()`
  - `Serial.print()`
  - `FORCE_MAX`
- 开始本实验前需要特别提醒的点：
  - 本实验程序中力敏传感器接 `GPIO 1`，和前两个实验不同。
  - 如果不先限制范围，映射后的亮度值可能不稳定。

### 2. 实验3接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 力敏传感器信号端 | `GPIO 1` | 信号输出接 `GPIO 1` |
| 力敏传感器电源 | `3.3V` | 模块电源正极接 `3.3V` |
| 力敏传感器地线 | `GND` | 模块地线接 `GND` |
| LED | `GPIO 2` | 若用外接 LED，需串联限流电阻 |
| LED 地线 | `GND` | 负极接 `GND` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：把力敏传感器的压力值转换成 LED 亮度，实现压力越大灯越亮。
  使用方法：力敏输出接 GPIO 1、LED 接 GPIO 2，打开 115200 串口后改变按压力度。
  运行结果：LED 随压力增大而变亮，串口同时显示 force 和 light 数值。
*/

// 力敏传感器连接 GPIO 1
#define PIN_FORCE 1

// 板载 LED_ON 连接 GPIO 2
#define PIN_LED 2

// 达到这个压力数值时，LED 为最亮
#define FORCE_MAX 3000

void setup() {
  // 启动串口，方便同时观察原始压力值和映射后的亮度值
  Serial.begin(115200);

  // 把 LED 引脚设置为输出模式
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  // 读取当前压力值
  int force = analogRead(PIN_FORCE);

  // 防止压力数值超过设定范围
  force = constrain(force, 0, FORCE_MAX);

  // 把压力转换为 0～255 的亮度
  int light = map(force, 0, FORCE_MAX, 0, 255);

  // 输出 PWM，控制 LED 亮度
  analogWrite(PIN_LED, light);

  // 串口打印原始压力值和映射后的亮度值
  Serial.print("force: ");
  Serial.print(force);
  Serial.print("  light: ");
  Serial.println(light);

  // 稍作延时，避免刷新过快
  delay(50);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中启动串口并初始化 LED 引脚。
  2. 在 `loop()` 中读取力敏传感器原始值。
  3. 用 `constrain()` 把压力值限制在 `0~FORCE_MAX` 范围内。
  4. 用 `map()` 把压力值映射成 `0~255` 的亮度值。
  5. 把映射后的亮度值写入 LED，并通过串口显示。
- `setup()` 的作用：
  - 初始化串口和 LED 输出引脚。
- `loop()` 的作用：
  - 持续读取压力值并转换成 LED 亮度。
- 数据输入、处理、输出的执行过程：
  - 输入：力敏传感器输出的模拟量。
  - 处理：先限制范围，再做数值映射。
  - 输出：LED 亮度变化，串口显示原始值和转换结果。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `constrain(force, 0, FORCE_MAX)`
  - `map(force, 0, FORCE_MAX, 0, 255)`
  - `analogWrite(PIN_LED, light)`
  - `Serial.print(...)`
- 每个函数的作用、写法、参数和返回值：
  - `constrain(force, 0, FORCE_MAX)`
    - 函数作用：把数值限制在指定范围内。
    - 参数1：`force`，表示当前读取到的压力值。
    - 参数2：`0`，表示允许的最小值。
    - 参数3：`FORCE_MAX`，表示允许的最大值。
    - 返回值：返回限制后的数值。
  - `map(force, 0, FORCE_MAX, 0, 255)`
    - 函数作用：把一个范围的数值映射到另一个范围。
    - 参数1：`force`，表示要映射的输入值。
    - 参数2、参数3：输入范围的最小值和最大值。
    - 参数4、参数5：输出范围的最小值和最大值。
    - 返回值：返回映射后的整数值。
  - `Serial.print("force: ")`
    - 函数作用：向串口连续输出内容，但不换行。
    - 参数1：要输出的字符串或变量。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `force = constrain(force, 0, FORCE_MAX); int light = map(force, 0, FORCE_MAX, 0, 255);`
    - 先限幅，再映射，是把传感器原始值转换成可控亮度值的标准做法。
- 为什么这样写：
  - 传感器原始值和 LED 亮度值不在同一范围，必须通过数据处理把“测到的量”变成“能控制的量”。

---

## 实验4

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验把连续变化的压力值分成多个等级，每个等级对应一种颜色。
  - `WS2812` 和普通 RGB 灯不同，它需要通过库函数发送颜色数据。
  - 设置颜色后必须调用 `show()`，否则灯珠不会真正改变颜色。
- 本实验新增的术语、模块或函数：
  - `#include <Adafruit_NeoPixel.h>`
  - `setBrightness()`
  - `setPixelColor()`
  - `show()`
- 开始本实验前需要特别提醒的点：
  - 力敏传感器接 `GPIO 15`，`WS2812` 数据端接 `GPIO 48`。
  - 程序使用 `Adafruit NeoPixel` 库，运行前要先安装。

### 2. 实验4接线
- 接线表：

| 元件 | ESP32-S3 引脚 | 接法 |
| --- | --- | --- |
| 力敏传感器信号端 | `GPIO 15` | 信号输出接 `GPIO 15` |
| 力敏传感器电源 | `3.3V` | 模块电源正极接 `3.3V` |
| 力敏传感器地线 | `GND` | 模块地线接 `GND` |
| WS2812 数据端 | `GPIO 48` | 数据输入端接 `GPIO 48` |
| WS2812 电源 | `5V/3.3V` | 按模块说明供电 |
| WS2812 地线 | `GND` | 与开发板共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：根据力敏传感器测得的压力大小改变 WS2812 彩灯颜色。
  使用方法：力敏输出接 GPIO 15、WS2812 接 GPIO 48，打开 115200 串口后逐渐加压。
  运行结果：无压力时灯灭，轻按显示蓝色，中等压力显示绿色，重按显示红色。
*/

// 导入板载 WS2812 彩灯控制库
#include <Adafruit_NeoPixel.h>

// 力敏传感器连接 GPIO 15
#define PIN_FORCE 15

// 板载 WS2812 连接 GPIO 48
#define PIN_RGB 48

// 板上只有一颗 WS2812 灯珠
#define RGB_COUNT 1

// 创建 WS2812 对象，指定灯珠数量、数据引脚和通信格式
Adafruit_NeoPixel rgb(RGB_COUNT, PIN_RGB, NEO_GRB + NEO_KHZ800);

void setup() {
  // 启动串口，方便观察当前压力数值
  Serial.begin(115200);

  // 启动彩灯对象
  rgb.begin();

  // 限制彩灯整体亮度，避免过亮
  rgb.setBrightness(50);

  // 清空颜色缓存并立即刷新到灯珠
  rgb.clear();
  rgb.show();
}

void loop() {
  // 读取当前压力值
  int force = analogRead(PIN_FORCE);
  Serial.println(force);

  // 没有明显压力时关闭彩灯
  if (force < 300) {
    rgb.clear();
  }

  // 轻按显示蓝色
  else if (force < 1200) {
    rgb.setPixelColor(0, rgb.Color(0, 0, 255));
  }

  // 中等压力显示绿色
  else if (force < 2400) {
    rgb.setPixelColor(0, rgb.Color(0, 255, 0));
  }

  // 重按显示红色
  else {
    rgb.setPixelColor(0, rgb.Color(255, 0, 0));
  }

  // 把颜色数据发送给 WS2812
  rgb.show();
  delay(50);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中启动串口并初始化 `WS2812` 对象。
  2. 在 `loop()` 中持续读取力敏传感器的压力值。
  3. 用多段条件判断把压力值划分为不同区间。
  4. 为不同压力区间设置不同颜色。
  5. 调用 `show()` 把颜色真正发送给灯珠。
- `setup()` 的作用：
  - 初始化串口、`WS2812` 对象和默认亮度。
- `loop()` 的作用：
  - 持续检测压力，并根据压力等级切换颜色。
- 数据输入、处理、输出的执行过程：
  - 输入：力敏传感器模拟量。
  - 处理：程序使用多段条件判断进行压力分级。
  - 输出：`WS2812` 显示不同颜色，串口输出压力值。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `Adafruit_NeoPixel rgb(...)`
  - `rgb.setBrightness(50)`
  - `rgb.setPixelColor(0, rgb.Color(...))`
  - `rgb.show()`
- 每个函数的作用、写法、参数和返回值：
  - `Adafruit_NeoPixel rgb(RGB_COUNT, PIN_RGB, NEO_GRB + NEO_KHZ800)`
    - 函数作用：创建一个 `WS2812` 控制对象。
    - 参数1：`RGB_COUNT`，表示灯珠数量。
    - 参数2：`PIN_RGB`，表示数据引脚编号。
    - 参数3：`NEO_GRB + NEO_KHZ800`，表示颜色顺序和通信格式。
    - 返回值：创建出一个可操作的彩灯对象。
  - `rgb.setBrightness(50)`
    - 函数作用：设置彩灯整体亮度。
    - 参数1：`50`，表示亮度值。
    - 返回值：无返回值。
  - `rgb.setPixelColor(0, rgb.Color(0, 255, 0))`
    - 函数作用：给指定编号的灯珠设置颜色。
    - 参数1：`0`，表示第 1 颗灯珠。
    - 参数2：`rgb.Color(0, 255, 0)`，表示颜色数据。
    - 返回值：无返回值。
  - `rgb.show()`
    - 函数作用：把缓存中的颜色真正刷新到灯珠。
    - 参数：无。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `if (force < 300) ... else if (force < 1200) ... else if (force < 2400) ... else ...`
    - 这一段把连续的压力数值划分成多个等级，是“模拟量转离散状态”的典型例子。
- 为什么这样写：
  - `WS2812` 更适合用颜色表达多个等级，所以这里不直接映射亮度，而是采用分段判断方式表达“无压力、轻压、中压、重压”。

---

## 总结
- 实验目标：
  - 学会使用力敏传感器读取压力数值。
  - 学会用阈值判断控制蜂鸣器报警。
  - 学会用 `PWM` 实现压力触发的呼吸灯效果。
  - 学会用 `map()` 和 `constrain()` 实现压力到亮度的转换。
  - 学会用 `WS2812` 颜色变化表达压力等级。
- 单词：
  - `force`：压力 / 受力
  - `sensor`：传感器
  - `buzzer`：蜂鸣器
  - `limit`：限制值 / 阈值
  - `max`：最大值
  - `light`：灯光 / 亮度
  - `analog`：模拟的
  - `read`：读取
  - `digital`：数字的 / 数字电平的
  - `write`：写入 / 输出
  - `brightness`：亮度
  - `color`：颜色
  - `red`：红色
  - `green`：绿色
  - `blue`：蓝色
  - `clear`：清除
  - `show`：显示 / 刷新显示
  - `map`：映射
  - `constrain`：限制范围
  - `value`：数值 / 值
- 接线：
  - 力敏传感器按实验要求分别接到 `GPIO 15` 或 `GPIO 1`。
  - 蜂鸣器接 `GPIO 21`。
  - LED 接 `GPIO 2`。
  - `WS2812` 数据端接 `GPIO 48`。
- 关键函数和参数：
  - `analogRead(pin)`
  - `digitalWrite(pin, HIGH/LOW)`
  - `analogWrite(pin, value)`
  - `constrain(x, min, max)`
  - `map(value, fromLow, fromHigh, toLow, toHigh)`
  - `rgb.setBrightness(value)`
  - `rgb.setPixelColor(index, rgb.Color(r, g, b))`
  - `rgb.show()`
- 关键程序：
  - 力敏蜂鸣器报警程序
  - 力敏呼吸灯程序
  - 力敏亮度映射程序
  - 力敏彩灯颜色程序
- 实验现象：
  - 按压力度越大，串口数值越高。
  - 超过阈值时，蜂鸣器会报警。
  - 达到阈值后，LED 会执行一次呼吸变化。
  - 压力越大，LED 越亮。
  - 无压力、轻压、中压、重压时，彩灯显示不同颜色。
- 本课重点：
  - 力敏传感器是模拟量输入，读取值是连续变化的。
  - 阈值判断适合做报警类实验，映射适合做亮度变化类实验，分段判断适合做等级显示类实验。
