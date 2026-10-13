# ESP32 + Arduino 入门教程（Day1 下）

> 面向对象：初学者  
> 目标：能读懂 Arduino/C 代码结构，理解 ESP32 常用硬件概念，并完成“按钮控制 LED（带消抖）”与“旋钮控制舵机”。

---

## 1. C 语言基础（面向 Arduino）

### 1.1 语法要素

#### 1.1.1 数据类型：int、String、void

- `int`
  - 含义：整数类型（有正有负）。
  - ESP32（Arduino）中通常占用 4 字节（32 位）。
  - 适用场景：计数、状态值、引脚编号、角度、时间差（毫秒）等。
- `String`
  - 含义：Arduino 提供的字符串类（可自动拼接、比较等）。
  - 占用空间：对象本身 + 堆内存（长度越大越占内存）。
  - 适用场景：串口命令、提示文本；注意不要在超长循环里无节制拼接，避免内存碎片化。
- `void`
  - 含义：函数返回类型为“无返回值”。
  - 占用空间：不是变量类型，不表示“占用多少字节”，而是告诉编译器“这个函数不返回数据”。
  - 适用场景：只做动作、不需要返回结果的函数，例如点灯、打印提示等。

**动手实践 1（截图串口输出）**

1. 新建工程并烧录下列代码。
2. 打开串口监视器，波特率 115200，截图输出。

**文件：`m1_types_demo.ino`**

```cpp
void setup() {
  Serial.begin(115200);                 // 初始化串口（必须先做）

  int a = 123;                          // int：整数变量
  String msg = "Hello";                 // String：字符串对象

  Serial.print("a=");                   // 打印文本（不换行）
  Serial.println(a);                    // 打印整数并换行
  Serial.print("msg=");                 // 打印标签
  Serial.println(msg);                  // 打印字符串并换行
}

void loop() {
  // loop() 里留空也可以，程序仍会反复执行 loop()
}
```

---

#### 1.1.2 控制关键字：return、if、while、for

##### (1) `return`

- 作用：结束函数，并可返回一个值给调用者。
- 常见错误：函数声明返回 `int`，却忘了 `return` 返回整数。

**示例：带返回值的函数**

```cpp
int addTwo(int x) {          // 返回 int 类型结果
  return x + 2;              // return：把结果返回给调用者
}
```

##### (2) `if`（条件判断）

语法骨架：

```cpp
if (条件) {
  // 条件为真执行
} else {
  // 条件为假执行
}
```

典型错误：
- 把比较写成赋值：`if (x = 1)`（错误习惯），正确是 `if (x == 1)`
- 忘记大括号导致逻辑和预期不一致（建议课堂统一加 `{}`）

##### (3) `while`（条件循环）

语法骨架：

```cpp
while (条件) {
  // 条件为真会一直循环
}
```

典型错误：
- 条件永远为真导致死循环（程序“卡住”）
- 忘记更新循环条件相关变量

##### (4) `for`（计数循环）

语法骨架：

```cpp
for (初始化; 条件; 更新) {
  // 循环体
}
```

典型错误：
- `i++` 写成 `i--` 导致循环不结束
- `i < N` 与 `i <= N` 边界写错导致多/少执行一次

**动手实践 2（截图串口输出）**

要求：串口依次输出 0 到 4。

**文件：`m2_for_demo.ino`**

```cpp
void setup() {
  Serial.begin(115200);                 // 初始化串口

  for (int i = 0; i < 5; i++) {         // for：从 0 到 4
    Serial.print("i=");                 // 打印标签
    Serial.println(i);                  // 打印 i 并换行
    delay(200);                         // 稍等，便于观察
  }
}

void loop() {
}
```

---

### 1.2 Arduino 程序框架

#### 1.2.1 setup() 与 loop() 的生命周期

- `setup()`
  - 程序上电或复位后执行一次。
  - 常见初始化任务：
    - `Serial.begin(...)` 串口初始化
    - `pinMode(...)` 引脚模式初始化
    - 初始化库（例如舵机、WiFi、传感器等）
- `loop()`
  - `setup()` 执行完后，`loop()` 会不断重复执行。
  - 常见任务：
    - 读取按钮/传感器
    - 控制 LED/电机
    - 周期打印调试信息

**动手实践 3（截图串口输出）**

要求：看到 `setup` 只打印一次，`loop` 每秒打印一次。

**文件：`m3_setup_loop_demo.ino`**

```cpp
void setup() {
  Serial.begin(115200);                 // 串口初始化
  Serial.println("setup: run once");    // setup 只执行一次
}

void loop() {
  Serial.println("loop: running");      // loop 反复执行
  delay(1000);                          // 每秒一次
}
```

---

#### 1.2.2 函数定义与调用（参数、返回值、作用域）

要点：
- 函数由“返回类型 + 函数名 + 参数列表 + 函数体”组成。
- 参数：函数的输入。
- 返回值：函数的输出（`void` 表示没有输出）。
- 作用域：在 `{}` 内声明的变量通常只在该块内有效。

**动手实践 4（截图串口输出）**

要求：调用自定义函数，打印两次不同的结果。

**文件：`m4_function_demo.ino`**

```cpp
int clampInt(int v, int minV, int maxV) {     // 自定义函数：返回 int
  if (v < minV) {                             // if：判断是否小于最小值
    return minV;                              // return：返回最小值
  }
  if (v > maxV) {                             // 判断是否大于最大值
    return maxV;                              // 返回最大值
  }
  return v;                                   // 返回原值
}

void setup() {
  Serial.begin(115200);                        // 串口初始化

  int a = clampInt(300, 0, 255);              // 调用函数：将 300 限制到 255
  int b = clampInt(-5, 0, 255);               // 将 -5 限制到 0

  Serial.print("a=");                         // 输出标签
  Serial.println(a);                          // 输出 a
  Serial.print("b=");                         // 输出标签
  Serial.println(b);                          // 输出 b
}

void loop() {
}
```

---

### 1.3 输入输出与调试（Serial）

#### 1.3.1 Serial.print() / Serial.println()

常见用法：
- `Serial.print("text");`：打印文本，不换行
- `Serial.println("text");`：打印文本并换行
- `Serial.print(value);`：打印整数/浮点/字符等
- `Serial.print(value, HEX);`：按十六进制打印（调试常用）

ESP32 上常用的格式化输出（更像 C 语言的 printf）：
- `Serial.printf("x=%d\n", x);`

**动手实践 5（截图串口输出）**

要求：同一个数字分别以十进制与十六进制输出。

**文件：`m5_serial_format_demo.ino`**

```cpp
void setup() {
  Serial.begin(115200);                 // 初始化串口

  int x = 255;                          // int：255

  Serial.print("DEC: ");                // 十进制标签
  Serial.println(x);                    // 十进制输出

  Serial.print("HEX: ");                // 十六进制标签
  Serial.println(x, HEX);               // 十六进制输出（FF）

  Serial.printf("printf: x=%d\n", x);   // printf 风格输出（ESP32 常用）
}

void loop() {
}
```

---

#### 1.3.2 转义字符：\n、\t

- `\n`：换行（new line）
- `\t`：制表符（tab），用于对齐输出

注意：
- `Serial.println()` 自带换行效果；但当你用 `Serial.print()` 拼接输出时，`\n` 很方便。

**动手实践 6（截图串口输出）**

要求：输出一张“对齐的小表格”。

**文件：`m6_escape_demo.ino`**

```cpp
void setup() {
  Serial.begin(115200);                          // 初始化串口

  Serial.print("Name\tScore\n");                 // \t 对齐，\n 换行
  Serial.print("A\t");                           // 第一行名字
  Serial.print(95);                              // 第一行分数
  Serial.print("\n");                            // 手动换行
  Serial.print("B\t");                           // 第二行名字
  Serial.print(88);                              // 第二行分数
  Serial.print("\n");                            // 手动换行
}

void loop() {
}
```

---

### 1.4 注释风格（要求：示例代码关键行必须中文注释）

- 单行注释：`// 说明`
- 多行注释：`/* 多行说明 */`

建议规范：
- 关键行必须解释“为什么做”和“做了什么”
- 不写空洞注释（例如“定义变量”），要写出变量用途

**动手实践 7**

要求：在你之前写过的任意一个示例里，补齐中文注释并提交截图。

---

## 2. 库函数与示例

### 2.1 ESP32Servo 库

#### 2.1.1 安装步骤（Arduino IDE）

1. Arduino IDE → Library Manager（库管理器）
2. 搜索：`ESP32Servo`
3. 安装完成后，记录版本号（提交作业时写在截图或文本里）

**动手实践 8**

要求：安装完成后截图库管理器界面（能看到库名与版本号）。

---

#### 2.1.2 常用 API（核心语义）

以下语义参考 ESP32Servo 项目说明（attach/write/read/detach 等）：
- `int attach(int pin)`
  - 作用：把舵机对象绑定到某个 GPIO 输出 PWM。
  - 返回：通道号；失败返回 0。
  - 建议引脚：常见推荐包括 2、4、12-19、21-23、25-27、32-33（不同开发板可能有差异）。
- `int attach(int pin, int min, int max)`
  - `min/max` 单位为微秒（us），常见 500–2500us 范围。
  - 当 `min < 500` 会被强制到 500；`max > 2500` 会被强制到 2500。
- `void write(int value)`
  - `value < 500`：当作角度（0–180 度）处理并限制范围。
  - `value >= 500`：当作微秒脉宽处理（会按 min/max 限制）。
- `int read()`
  - 返回：最后一次写入的角度（0–180）。
- `void detach()`
  - 作用：停止输出 PWM，释放通道，舵机不再保持控制。

参考链接（课堂可由老师提供）：  
- ESP32Servo 项目说明与头文件（attach 返回值与参数范围等）【来源：GitHub】  
  - https://github.com/amanenk/ESP32Servo  
  - https://github.com/jkb-git/ESP32Servo/blob/master/src/ESP32_Servo.h

**动手实践 8.5（截图串口输出）**

要求：接好舵机后，烧录下列代码，截图串口输出（至少 3 行）。

**文件：`m8_5_servo_api_demo.ino`**

```cpp
#include <ESP32Servo.h>                  // 引入舵机库

#define SERVO_PIN 18                     // 舵机信号线接 GPIO18（示例）

Servo s;                                  // 创建舵机对象

void setup() {
  Serial.begin(115200);                   // 初始化串口

  int ch = s.attach(SERVO_PIN);           // 绑定舵机到引脚
  if (ch == 0) {                          // 失败返回 0
    Serial.println("ERR: attach failed"); // 打印错误
    return;                               // return：结束 setup
  }

  Serial.print("OK: channel=");           // 打印通道号
  Serial.println(ch);                     // 输出通道号并换行

  s.write(0);                             // 写入角度 0 度
  Serial.print("read angle=");            // 打印 read 标签
  Serial.println(s.read());               // read：读取最后写入角度

  delay(800);                             // 等待舵机转动完成

  s.write(90);                            // 写入角度 90 度
  Serial.print("read angle=");            // 再次读取
  Serial.println(s.read());               // 输出角度

  s.detach();                             // detach：释放 PWM 输出
  Serial.println("OK: detached");         // 提示已释放
}

void loop() {
}
```

---

#### 2.1.3 示例：旋钮控制舵机（完整源码）

硬件建议：
- 舵机建议使用独立 5V 供电（不要直接用 ESP32 的 3V3 引脚硬拉舵机电源）。
- 舵机 GND 必须与 ESP32 GND 共地。
- 旋钮（电位器）接 ADC1 引脚（例如 GPIO34），避免 ADC2 与 WiFi 冲突。

【接线占位符】  
- `[此处放：电位器 + 舵机接线图（Fritzing 或照片）]`

**文件：`m8_servo_knob.ino`**

```cpp
#include <ESP32Servo.h>                 // 引入舵机库（必须）

#define SERVO_PIN 18                    // 舵机信号线接到 GPIO18（示例）
#define POT_PIN 34                      // 电位器中间脚接 GPIO34（ADC1）

Servo myServo;                          // 创建舵机对象

int clampInt(int v, int minV, int maxV) {
  if (v < minV) {                        // 小于最小值就夹到最小
    return minV;                         // 返回 minV
  }
  if (v > maxV) {                        // 大于最大值就夹到最大
    return maxV;                         // 返回 maxV
  }
  return v;                              // 返回原值
}

void setup() {
  Serial.begin(115200);                  // 串口调试输出

  int ch = myServo.attach(SERVO_PIN);    // 绑定舵机到引脚，返回通道号
  if (ch == 0) {                         // 0 表示失败（做防御性处理）
    Serial.println("ERR: servo attach failed"); // 输出错误提示
  } else {
    Serial.print("OK: servo channel=");  // 输出成功信息
    Serial.println(ch);                  // 打印通道号
  }

  myServo.write(90);                     // 初始化角度到中位（90 度）
}

void loop() {
  int raw = analogRead(POT_PIN);         // 读取电位器 ADC 原始值（通常 0~4095）
  raw = clampInt(raw, 0, 4095);          // 防止异常值（防御性处理）

  int angle = map(raw, 0, 4095, 0, 180); // 将 ADC 映射到角度 0~180
  angle = clampInt(angle, 0, 180);       // 再次限制范围，避免越界

  myServo.write(angle);                  // 设置舵机角度

  Serial.print("raw=");                  // 打印原始 ADC
  Serial.print(raw);                     // 输出 raw
  Serial.print("\tangle=");              // 用 \t 对齐输出
  Serial.print(angle);                   // 输出角度
  Serial.print("\n");                    // 换行

  delay(50);                             // 稍等，降低刷屏频率
}
```

**动手实践 9（截图串口输出）**

要求：
1. 旋转电位器，串口输出的 `raw` 与 `angle` 会变化。
2. 截图一张，能看到至少 5 行输出。

---

## 3. ESP32 硬件原理图认知（看得懂“关键引脚”）

### 3.1 最小系统

#### 3.1.1 电源引脚：3V3、GND

- `3V3`
  - 提供 3.3V 电源给 ESP32 核心与部分外设。
  - 不建议用它直接给舵机/电机供电（电流波动大、易复位）。
- `GND`
  - 地线，所有电路必须“共地”才有同一参考电平。
- 去耦电容（概念性认知）
  - 电源旁的电容用于抑制瞬时电流波动，减少复位与干扰。

**动手实践 10**

要求：在你的开发板上找到 `3V3` 与 `GND` 丝印位置，并拍照标注（或截图教材图）。

---

#### 3.1.2 启动模式：GPIO0、GPIO2、EN（重点：下载/运行）

常见现象：
- 需要烧录时，按住 BOOT 再点上传（很多板子 BOOT 对应 GPIO0）。

常用对应表（课堂记住“最重要的一条”即可）：

| 引脚/信号 | 下载模式（烧录） | 运行模式（正常执行） | 说明 |
|---|---|---|---|
| GPIO0 | 低电平 | 高电平 | GPIO0=低 → 进入下载 |
| EN | 高电平 | 高电平 | EN 低会复位/关闭芯片 |
| GPIO2 | 通常保持默认 | 通常保持默认 | 尽量不要在启动阶段强拉该脚 |

**动手实践 11**

要求：当遇到“连接失败”时，尝试按住 BOOT 再上传，并记录是否成功（文字即可）。

---

### 3.2 GPIO 架构（认识引脚“名字”与“能力”）

#### 3.2.1 引脚编号规则

- 物理引脚：开发板上实际的针脚位置（“脚位”）。
- GPIO 号：Arduino 代码里使用的编号，例如 `GPIO18` 常写成 `18`。
- RTC 编号：部分低功耗相关引脚会有 RTC IO 名称（进阶内容，课堂先认识术语即可）。

**动手实践 12**

要求：在你的 ESP32 开发板引脚图里找到 GPIO18、GPIO15、GPIO34，并标注它们的用途（输出/输入/ADC）。

---

#### 3.2.2 驱动能力（安全认知）

- ESP32 GPIO 是 3.3V 逻辑，不能直接接 5V 信号。
- LED 必须串联电阻，避免引脚过流。
- 输出模式：
  - 推挽输出（常见）：`pinMode(pin, OUTPUT);`
  - 开漏输出（进阶）：用于需要外部上拉的场景（例如 I2C）

**动手实践 13**

要求：用万用表测量 GPIO 输出高电平约为 3.3V（没有万用表可跳过）。

---

### 3.3 数字与模拟信号

#### 3.3.1 数字信号（HIGH/LOW）与消抖

- 数字读取：`digitalRead(pin)` → `HIGH` 或 `LOW`
- 数字输出：`digitalWrite(pin, HIGH/LOW)`
- 消抖（debounce）
  - 按钮在按下/松开瞬间会抖动，导致一次按压被读成多次。
  - 解决：硬件 RC 或软件延时/状态机（本教程用软件 50ms 消抖）。
- 内部上拉/下拉
  - `INPUT_PULLUP`：内部上拉（常用）
  - `INPUT_PULLDOWN`：内部下拉（ESP32 支持）

**动手实践 14**

要求：写一段程序读取按钮（先不消抖），观察串口是否会出现“抖动”（同一次按压出现多次变化）。

---

#### 3.3.2 模拟信号（ADC）与测量范围

- ADC1/ADC2
  - ADC1：通常更稳定，建议优先用（例如 GPIO32–GPIO39 区域常见 ADC1 引脚）。
  - ADC2：在启用 WiFi 等功能时可能受影响（课堂先记住“优先 ADC1”）。
- 分辨率（常见）：12-bit
  - 原始读数通常是 0–4095。
- 衰减档位（概念性认知）
  - 衰减越大，可测电压范围越大。
  - 常见范围（不同芯片/校准会有差异，课堂只记“近似”）：
    - 0 dB：约 0–1.1V
    - 2.5 dB：约 0–1.5V
    - 6 dB：约 0–2.2V
    - 11 dB：约 0–3.3V（近似）

**动手实践 15（截图串口输出）**

要求：用电位器接 ADC1，引脚读数随旋转变化，并截图。

---

#### 3.3.3 DAC（GPIO25、GPIO26）

- DAC：数模转换，可输出“类模拟电压”（本质是模拟输出通道）。
- 常见通道：GPIO25、GPIO26。
- 分辨率：常见为 8-bit（0–255）。
- 课堂建议：先理解概念即可，后续项目再用。

---

## 4. 按钮控制 LED（完整项目：带 50ms 软件消抖）

### 4.1 需求

- 按下按钮点亮 LED，松开熄灭。
- 加入 50ms 软件消抖。
- 串口输出调试信息（状态变化时打印）。

---

### 4.2 硬件连接

要求连接（按题目固定）：
- 按钮：一端接 GPIO15，另一端经 10 kΩ 下拉到 GND
- LED：正极经 220 Ω 电阻接 GPIO18，负极接 GND

【Fritzing 示意图占位符】  
![Fritzing：按钮控制 LED（占位符）](./images/day1_button_led_fritzing.png)

【文字电路图（备用）】

```
按钮输入（外部下拉）：
GPIO15 ----o  o---- 3V3
         按钮
GPIO15 ----[10k]---- GND

LED 输出：
GPIO18 ----[220Ω]----|>|---- GND
                     LED
```

**动手实践 16**

要求：接线完成后拍照（能清晰看到 GPIO15、GPIO18、10k、220Ω）。

---

### 4.3 软件实现（digitalRead/digitalWrite/millis + 状态机）

实现思路（课堂口令）：
1. 读取原始按钮值。
2. 如果原始值变化，记录变化时间。
3. 变化后持续稳定超过 50ms，才认为“按键真的变了”。
4. 用稳定后的按键状态控制 LED，并在变化时打印日志。

**文件：`p1_button_led_debounce.ino`**

```cpp
#define BUTTON_PIN 15                    // 按钮输入引脚：GPIO15
#define LED_PIN 18                       // LED 输出引脚：GPIO18

const unsigned long DEBOUNCE_MS = 50;    // 消抖时间：50ms

int lastRaw = LOW;                       // 上一次原始读数（未消抖）
int stableState = LOW;                   // 当前稳定状态（已消抖）
unsigned long lastChangeMs = 0;          // 最近一次原始变化的时间

void setup() {
  Serial.begin(115200);                  // 串口调试输出

  pinMode(BUTTON_PIN, INPUT);            // 外部 10k 下拉：输入模式即可
  pinMode(LED_PIN, OUTPUT);              // LED 引脚输出
  digitalWrite(LED_PIN, LOW);            // 默认先关灯

  Serial.println("Ready: press button to control LED"); // 启动提示
}

void loop() {
  int raw = digitalRead(BUTTON_PIN);     // 读取按钮原始电平（HIGH/LOW）
  unsigned long now = millis();          // 当前时间（毫秒）

  if (raw != lastRaw) {                  // 发现原始读数发生变化
    lastRaw = raw;                       // 记录新的原始读数
    lastChangeMs = now;                  // 记录变化发生的时间
  }

  if ((now - lastChangeMs) >= DEBOUNCE_MS) { // 原始读数稳定超过 50ms
    if (stableState != raw) {                // 稳定状态需要更新
      stableState = raw;                     // 更新稳定状态

      if (stableState == HIGH) {             // 按下按钮（GPIO15 变 HIGH）
        digitalWrite(LED_PIN, HIGH);         // 点亮 LED
        Serial.println("OK: button=HIGH, LED=ON"); // 打印状态
      } else {                               // 松开按钮（GPIO15 变 LOW）
        digitalWrite(LED_PIN, LOW);          // 熄灭 LED
        Serial.println("OK: button=LOW, LED=OFF"); // 打印状态
      }
    }
  }
}
```

**动手实践 17（截图串口输出）**

要求：
- 连续按压与松开按钮，截图能看到至少 6 行状态变化输出。

---

### 4.4 测试验收

#### 4.4.1 功能测试

- 连续快速按压 20 次：
  - LED 行为正确（按下亮、松开灭）
  - 串口输出不出现“一次按压触发多次变化”的误触发（明显减少抖动）

#### 4.4.2 电气测试（有设备则做，没有可跳过）

- 万用表：测 GPIO15 按下/松开时电平变化（LOW/HIGH）。
- 示波器：观察 LED 端上升/下降沿（目标 < 1 µs）。

---

## 5. 常见问题（不少于 5 条）

1. 看不到 COM 口
   - 检查数据线是否可传数据
   - 安装对应串口驱动后重插
   - 更换 USB 口
2. 串口监视器乱码
   - 代码 `Serial.begin(115200);` 与监视器波特率必须一致
3. 上传失败（连接失败/超时）
   - 检查 Board 是否选对、Port 是否选对
   - 尝试按住 BOOT 再上传
4. LED 不亮
   - 检查 LED 极性是否接反
   - 检查是否串联电阻
   - 检查 LED 是否接到正确 GPIO（GPIO18）与 GND
5. 按钮按一次却触发多次
   - 说明消抖不足或接线浮空
   - 确认 10k 下拉电阻已连接到 GND
   - 确认程序使用 `millis()` 消抖逻辑，并把 `DEBOUNCE_MS` 设为 50
6. 舵机抖动或 ESP32 复位
   - 舵机不要用 3V3 供电，改用独立 5V
   - 舵机与 ESP32 必须共地（GND 连接）
   - 串口输出观察是否反复重启（复位现象）
