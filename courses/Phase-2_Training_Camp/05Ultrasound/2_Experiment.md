# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验的核心是先让超声波模块完成一次测量，再把测量结果显示到串口监视器。
  - `TRIG` 需要短暂高电平脉冲，`ECHO` 返回高电平持续时间。
  - `pulseIn()` 返回的时间单位是微秒，不是毫秒。
- 本实验新增的术语、模块或函数：
  - `delayMicroseconds()`
  - `pulseIn()`
  - `measureDistance()`
  - `echoTime`
- 开始本实验前需要特别提醒的点：
  - `TRIG` 接 `GPIO 12`。
  - `ECHO` 经分压后接 `GPIO 13`。
  - 串口监视器波特率为 `115200`。

### 2. 实验1接线
- 接线表：

| HC-SR04 | ESP32-S3 / 电路 | 说明 |
| --- | --- | --- |
| `VCC` | `5V` | 按模块标注供电 |
| `TRIG` | `GPIO 12` | 发送触发脉冲 |
| `ECHO` | `GPIO 13` | 必须先经过分压后再接入 |
| `GND` | `GND` | 与开发板共地 |

- `ECHO` 分压说明：
  - `HC-SR04 ECHO -> 1kΩ -> GPIO 13`
  - `GPIO 13 -> 2kΩ -> GND`

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 GPIO 12 和 GPIO 13 进行超声波测距，并在串口显示距离。
  使用方法：TRIG 接 GPIO 12、ECHO 经分压后接 GPIO 13，打开 115200 串口并移动障碍物。
  运行结果：串口持续显示障碍物距离，单位为 cm；无有效回波时显示测量失败。
*/

// HC-SR04 超声波模块引脚
#define PIN_TRIG 12
#define PIN_ECHO 13

void setup() {
  // 启动串口，方便显示测量结果
  Serial.begin(115200);

  // TRIG 负责输出触发脉冲，ECHO 负责输入回波信号
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // TRIG 默认保持低电平
  digitalWrite(PIN_TRIG, LOW);
}

void loop() {
  // 进行一次超声波测距
  float distance = measureDistance();

  if (distance == 0) {
    Serial.println("未收到回波");
  } else {
    Serial.print("距离：");
    Serial.print(distance, 1);
    Serial.println(" cm");
  }

  // 两次测量之间留出时间，避免上一束回波影响下一次测量
  delay(200);
}

float measureDistance() {
  // 先产生一个至少 10 微秒的触发脉冲
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 测量 ECHO 高电平持续时间，30 ms 后仍无回波就退出
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);

  // 声速约为 0.0343 cm/us；声音走了一个来回，所以除以 2
  float distance = echoTime * 0.0343 / 2.0;
  return distance;
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 在 `setup()` 中初始化串口和超声波模块引脚。
  2. 在 `loop()` 中调用 `measureDistance()` 进行一次测距。
  3. `measureDistance()` 先发送 `10us` 触发脉冲。
  4. 再用 `pulseIn()` 测量 `ECHO` 高电平持续时间。
  5. 用声速公式把回波时间换算为厘米距离。
  6. 把结果显示到串口监视器。
- `setup()` 的作用：
  - 初始化串口、`TRIG` 引脚和 `ECHO` 引脚。
- `loop()` 的作用：
  - 持续执行测距并显示结果。
- 数据输入、处理、输出的执行过程：
  - 输入：`ECHO` 返回的高电平持续时间。
  - 处理：程序把时间换算成距离值。
  - 输出：串口显示距离，或者提示“未收到回波”。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `delayMicroseconds(10)`
  - `pulseIn(PIN_ECHO, HIGH, 30000)`
  - `float distance = echoTime * 0.0343 / 2.0`
  - `measureDistance()`
- 每个函数的作用、写法、参数和返回值：
  - `delayMicroseconds(10)`
    - 函数作用：让程序暂停指定的微秒数。
    - 参数1：`10`，表示延时 `10us`。
    - 返回值：无返回值。
  - `pulseIn(PIN_ECHO, HIGH, 30000)`
    - 函数作用：测量 `PIN_ECHO` 引脚上高电平持续了多长时间。
    - 参数1：`PIN_ECHO`，要测量的引脚。
    - 参数2：`HIGH`，表示测量高电平脉冲。
    - 参数3：`30000`，最长等待 `30000us`，超时则返回 `0`。
    - 返回值：返回高电平持续时间，单位微秒。
  - `measureDistance()`
    - 函数作用：完成一次完整的超声波触发、测量和距离计算。
    - 参数：无。
    - 返回值：返回测得的距离值，单位厘米。
- 关键代码片段分析：
  - `echoTime * 0.0343 / 2.0` 中的 `/ 2.0` 不能漏，因为超声波走的是“来回”两段路程。
- 为什么这样写：
  - 把测距过程单独封装成函数后，后面做倒车雷达时可以直接复用同一套测量逻辑。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验在测距基础上增加了有源蜂鸣器报警。
  - 距离不是简单的“有/无”两种状态，而是分成安全、注意、较近、危险多个区间。
  - `beepOnce()` 用来完成一次“响一下，再安静一会儿”的节奏控制。
- 本实验新增的术语、模块或函数：
  - `beepOnce()`
  - `soundTime`
  - `quietTime`
  - 多段 `if`
- 开始本实验前需要特别提醒的点：
  - 超声波模块接线与实验1相同。
  - 有源蜂鸣器控制端接 `GPIO 21`。
  - 如果模块为低电平触发，响停逻辑可能需要反过来。

### 2. 实验2接线
- 接线表：

| 元件 | ESP32-S3 / 电路 | 说明 |
| --- | --- | --- |
| HC-SR04 `VCC` | `5V` | 按模块标注供电 |
| HC-SR04 `TRIG` | `GPIO 12` | 发送触发脉冲 |
| HC-SR04 `ECHO` | `GPIO 13` | 必须先经过分压后再接入 |
| HC-SR04 `GND` | `GND` | 与开发板共地 |
| 有源蜂鸣器 `IN` | `GPIO 21` | 控制响和停 |
| 有源蜂鸣器 `GND` | `GND` | 与开发板共地 |
| 有源蜂鸣器 `VCC` | 按模块标注 | 接 `3.3V` 或 `5V` |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用超声波模块测量障碍物距离，距离越近有源蜂鸣器报警越急促。
  使用方法：TRIG 接 GPIO 12、ECHO 经分压接 GPIO 13、蜂鸣器接 GPIO 21，然后移动障碍物。
  运行结果：串口显示距离；障碍物靠近时蜂鸣器鸣叫间隔缩短，进入危险距离后持续报警。
*/

// HC-SR04 超声波模块引脚
#define PIN_TRIG 12
#define PIN_ECHO 13

// 有源蜂鸣器模块的控制端连接 GPIO 21
#define PIN_BUZZER 21

void setup() {
  // 启动串口，显示当前测量结果和报警状态
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // 默认关闭触发引脚和蜂鸣器
  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  // 进行一次距离测量
  float distance = measureDistance();

  if (distance == 0) {
    Serial.println("未收到回波，蜂鸣器关闭");
    digitalWrite(PIN_BUZZER, LOW);
    delay(200);
  } else {
    Serial.print("距离：");
    Serial.print(distance, 1);
    Serial.print(" cm，");

    // 按距离区间决定报警节奏
    if (distance <= 20) {
      Serial.println("危险，连续报警");
      digitalWrite(PIN_BUZZER, HIGH);
      delay(100);
    } else if (distance <= 50) {
      Serial.println("较近，快速报警");
      beepOnce(100, 150);
    } else if (distance <= 100) {
      Serial.println("注意，慢速报警");
      beepOnce(100, 600);
    } else {
      Serial.println("安全，蜂鸣器关闭");
      digitalWrite(PIN_BUZZER, LOW);
      delay(200);
    }
  }
}

float measureDistance() {
  // 发送一次 10us 触发脉冲
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 读取回波高电平时间
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);

  // 换算为厘米距离
  float distance = echoTime * 0.0343 / 2.0;
  return distance;
}

void beepOnce(int soundTime, int quietTime) {
  // 蜂鸣器响一小段时间
  digitalWrite(PIN_BUZZER, HIGH);
  delay(soundTime);

  // 蜂鸣器停止，再等待一段时间
  digitalWrite(PIN_BUZZER, LOW);
  delay(quietTime);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 初始化串口、超声波模块和有源蜂鸣器引脚。
  2. 每次循环先测量当前距离。
  3. 如果测量失败，就关闭蜂鸣器。
  4. 如果测量成功，就按距离区间判断当前状态。
  5. 安全区关闭蜂鸣器，注意区慢速报警，较近区快速报警，危险区连续报警。
- `setup()` 的作用：
  - 初始化所有引脚和串口。
- `loop()` 的作用：
  - 持续测距并根据距离变化调整报警节奏。
- 数据输入、处理、输出的执行过程：
  - 输入：超声波模块回波时间。
  - 处理：先换算距离，再按区间判断报警级别。
  - 输出：串口显示当前距离和状态；蜂鸣器发出不同节奏报警。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `if (distance <= 20)`
  - `beepOnce(100, 150)`
  - `beepOnce(100, 600)`
  - `digitalWrite(PIN_BUZZER, HIGH)`
- 每个函数的作用、写法、参数和返回值：
  - `beepOnce(int soundTime, int quietTime)`
    - 函数作用：让蜂鸣器响一段时间，再停止一段时间。
    - 参数1：`soundTime`，蜂鸣器发声时间，单位毫秒。
    - 参数2：`quietTime`，蜂鸣器静音时间，单位毫秒。
    - 返回值：无返回值。
  - `digitalWrite(PIN_BUZZER, HIGH)`
    - 函数作用：让有源蜂鸣器发声。
    - 参数1：蜂鸣器控制引脚。
    - 参数2：`HIGH`，表示输出高电平。
    - 返回值：无返回值。
- 关键代码片段分析：
  - 多段 `if` 把距离划分为 `<=20`、`<=50`、`<=100` 和更远几个区间，这是倒车雷达逻辑的核心。
- 为什么这样写：
  - 倒车雷达不是单纯测出一个数，而是要把这个数转成“人容易听懂的风险提示”，所以需要做距离分级。

---

## 总结
- 实验目标：
  - 学会使用超声波模块测量距离。
  - 学会使用 `delayMicroseconds()` 和 `pulseIn()` 完成一次完整测量。
  - 学会根据回波时间计算距离。
  - 学会把距离划分为多个报警区间。
  - 学会用有源蜂鸣器实现倒车雷达报警。
- 单词：
  - `ultrasonic`：超声波的
  - `distance`：距离
  - `measure`：测量
  - `echo`：回波
  - `trigger`：触发
  - `pulse`：脉冲
  - `microseconds`：微秒
  - `radar`：雷达
  - `buzzer`：蜂鸣器
  - `danger`：危险
  - `safe`：安全
  - `time`：时间
  - `sound`：声音
  - `quiet`：安静 / 静音间隔
  - `high`：高
  - `low`：低
- 接线：
  - `TRIG -> GPIO 12`
  - `ECHO -> 分压后接 GPIO 13`
  - 蜂鸣器 `IN -> GPIO 21`
  - 超声波模块和蜂鸣器都要与开发板共地
- 关键函数和参数：
  - `delayMicroseconds(us)`
  - `pulseIn(pin, HIGH, timeout)`
  - `measureDistance()`
  - `beepOnce(soundTime, quietTime)`
  - `digitalWrite(pin, HIGH/LOW)`
- 关键程序：
  - 超声波串口测距程序
  - 超声波倒车雷达程序
- 实验现象：
  - 串口会显示实时距离。
  - 无回波时会提示测量失败。
  - 距离越近，报警越急促；极近距离会连续报警。
- 本课重点：
  - `ECHO` 信号测到的是时间，不是直接的距离值。
  - 距离公式必须除以 `2`。
  - `HC-SR04` 的 `ECHO` 对 `ESP32-S3` 来说通常需要降压处理。
