# 第二模块 实验

## 实验1

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验先学习最基础的单边沿计数方式，只使用编码器 A 相的上升沿。
  - 中断函数中只做“快速判断方向并修改计数”的工作，不做复杂输出。
  - 串口输出放在 `loop()` 中完成，避免 ISR 过长。
- 本实验新增的术语、模块或函数：
  - `attachInterrupt()`
  - `digitalPinToInterrupt()`
  - `IRAM_ATTR`
  - `noInterrupts()`
  - `interrupts()`
- 开始本实验前需要特别提醒的点：
  - 编码器 A 相接 `GPIO 6`，B 相接 `GPIO 7`。
  - 本实验采用 `INPUT_PULLUP` 输入方式。
  - 若转动方向和计数正负号相反，可以交换 A/B 接线，或交换程序中的加减号。

### 2. 实验1接线
- 接线表：

| 编码器引脚 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `A` | `GPIO 6` | 被中断检测的主信号 |
| `B` | `GPIO 7` | 用于判断方向 |
| `VCC` | 按模块额定电压 | 注意逻辑兼容 |
| `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用最基础的 RISING 上升沿中断统计编码器 A 相脉冲，并通过 B 相判断方向。
  使用方法：编码器 A、B 相接 GPIO 6、7，上传后打开 115200 串口并缓慢正反转动编码器。
  运行结果：A 相每出现一次上升沿，Count 增加或减少 1；停止转动后计数保持不变。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 定义编码器 A 相、B 相连接到的引脚
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

// 这个变量会在中断函数中被修改，所以要使用 volatile
volatile long encoderCount = 0;

// 中断函数没有参数，也没有返回值
void IRAM_ATTR encoderRising() {
  // A 相上升沿到来时，读取 B 相当前电平判断旋转方向
  if (digitalRead(ENCODER_B_PIN) == HIGH) {
    // 若 B 相为高，记作正转，计数加 1
    encoderCount = encoderCount + 1;
  } else {
    // 若 B 相为低，记作反转，计数减 1
    encoderCount = encoderCount - 1;
  }
}

void setup() {
  // 初始化串口，方便把计数结果显示到串口监视器
  Serial.begin(115200);

  // A 相和 B 相都设置为上拉输入模式
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // RISING 表示只在 A 相从 LOW 变成 HIGH 时触发中断函数
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    encoderRising,
    RISING
  );
}

void loop() {
  // 复制共享变量前先短暂关闭中断，防止复制一半时被中断修改
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  // 在主循环中输出计数结果，而不是在中断函数里输出
  Serial.print("RISING Count:");
  Serial.println(countCopy);

  // 每 200ms 显示一次结果，便于观察
  delay(200);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义编码器引脚和共享计数变量。
  2. 在 `setup()` 中初始化串口和输入引脚。
  3. 使用 `attachInterrupt()` 把 A 相上升沿和中断函数绑定起来。
  4. 每当 A 相出现上升沿时，中断函数立即执行，并读取 B 相判断方向。
  5. `loop()` 周期性安全复制计数值，再通过串口输出。
- `setup()` 的作用：
  - 完成串口初始化、编码器输入配置和中断绑定。
- `loop()` 的作用：
  - 把中断里不断变化的计数值复制出来并显示。
- 数据输入、处理、输出的执行过程：
  - 输入：编码器 A 相边沿信号和 B 相方向信号。
  - 处理：中断函数在上升沿到来时判断方向并修改 `encoderCount`。
  - 输出：串口显示最新计数值。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), encoderRising, RISING)`
  - `digitalRead(ENCODER_B_PIN)`
  - `noInterrupts()`
  - `interrupts()`
  - `volatile long encoderCount`
- 每个函数的作用、写法、参数和返回值：
  - `attachInterrupt(interrupt, isr, mode)`
    - 函数作用：把某个引脚对应的外部中断和中断函数绑定起来。
    - 参数1：中断编号，通常用 `digitalPinToInterrupt(pin)` 获得。
    - 参数2：中断发生时要执行的函数名。
    - 参数3：触发模式，如 `RISING`、`FALLING`、`CHANGE`。
    - 返回值：无返回值。
  - `digitalPinToInterrupt(pin)`
    - 函数作用：把普通 GPIO 引脚号转换成外部中断号。
    - 参数1：GPIO 引脚号。
    - 返回值：该引脚对应的中断编号。
  - `digitalRead(pin)`
    - 函数作用：读取指定数字引脚当前的逻辑电平。
    - 参数1：要读取的引脚号。
    - 返回值：`HIGH` 或 `LOW`。
  - `noInterrupts()`
    - 函数作用：临时关闭中断。
    - 参数：无。
    - 返回值：无返回值。
  - `interrupts()`
    - 函数作用：重新开启中断。
    - 参数：无。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `if (digitalRead(ENCODER_B_PIN) == HIGH)` 说明本实验不是单纯计数，而是“在 A 相边沿发生时，顺手读取 B 相电平来判断方向”。
  - `noInterrupts(); long countCopy = encoderCount; interrupts();` 是中断共享变量最基础、最重要的安全读取模板。
- 为什么这样写：
  - 先从最简单的上升沿中断开始，能帮助学生清楚理解“边沿触发”和“方向判断”的最小闭环。

---

## 实验2

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验结构和实验 1 很像，只是把触发方式改成下降沿。
  - 下降沿到来时，B 相电平和上升沿实验中的判断条件不一样。
  - 通过对比上升沿和下降沿程序，学生可以更直观理解触发方式的区别。
- 本实验新增的术语、模块或函数：
  - `FALLING`
  - `encoderFalling()`
- 开始本实验前需要特别提醒的点：
  - 不要同时在一个引脚上配置多种中断模式。
  - 本实验上传时应只保留下降沿程序。

### 2. 实验2接线
- 接线表：

| 编码器引脚 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `A` | `GPIO 6` | 下降沿触发中断 |
| `B` | `GPIO 7` | 用于判断方向 |
| `VCC` | 按模块额定电压 | 注意逻辑兼容 |
| `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 FALLING 下降沿中断统计编码器 A 相脉冲，并通过 B 相判断方向。
  使用方法：编码器 A、B 相接 GPIO 6、7，上传后打开 115200 串口并缓慢正反转动编码器。
  运行结果：A 相每出现一次下降沿，Count 增加或减少 1；停止转动后计数保持不变。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 定义编码器 A 相和 B 相引脚
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

// 中断和主循环共同访问的变量要写成 volatile
volatile long encoderCount = 0;

// FALLING 只会在 A 相从 HIGH 变成 LOW 时触发
void IRAM_ATTR encoderFalling() {
  // 下降沿发生时，读取 B 相电平判断方向
  if (digitalRead(ENCODER_B_PIN) == LOW) {
    // 若 B 相为低，记作正转
    encoderCount = encoderCount + 1;
  } else {
    // 否则记作反转
    encoderCount = encoderCount - 1;
  }
}

void setup() {
  // 打开串口监视器输出
  Serial.begin(115200);

  // 配置两个编码器引脚为上拉输入
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // 把 A 相引脚、下降沿模式和中断函数绑定起来
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    encoderFalling,
    FALLING
  );
}

void loop() {
  // 为了安全读取共享变量，先关闭中断再复制
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  // 在串口中显示当前计数
  Serial.print("FALLING Count:");
  Serial.println(countCopy);

  // 加入适当延时，让输出更稳定、更易读
  delay(200);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义编码器引脚和共享变量。
  2. 初始化串口和输入引脚。
  3. 把 A 相下降沿和 `encoderFalling()` 绑定。
  4. 每次下降沿到来时，根据 B 相电平判断方向并修改计数。
  5. 主循环复制计数并打印。
- `setup()` 的作用：
  - 完成串口初始化和中断模式设置。
- `loop()` 的作用：
  - 负责把计数结果输出到串口监视器。
- 数据输入、处理、输出的执行过程：
  - 输入：A 相下降沿、B 相电平。
  - 处理：在 ISR 中根据 B 相判断加减。
  - 输出：串口显示 `FALLING Count`。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `attachInterrupt(..., encoderFalling, FALLING)`
  - `if (digitalRead(ENCODER_B_PIN) == LOW)`
- 每个函数的作用、写法、参数和返回值：
  - `FALLING`
    - 含义：当引脚从高电平变为低电平时触发中断。
    - 作用：让程序只统计下降沿。
  - `encoderFalling()`
    - 函数作用：作为中断函数，在下降沿发生时执行。
    - 参数：无。
    - 返回值：无返回值。
  - `Serial.print(data)` / `Serial.println(data)`
    - 函数作用：把数据输出到串口监视器。
    - 参数1：要输出的数据。
    - 返回值：输出的字节数。
- 关键代码片段分析：
  - 上升沿实验中是“B 为高记正转”，而下降沿实验中变成“B 为低记正转”，这正是相位关系带来的差异。
- 为什么这样写：
  - 让学生明确理解：不同的中断模式不仅触发时机不同，方向判断条件也要跟着变化。

---

## 实验3

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验使用 `CHANGE`，即 A 相只要发生电平变化就进入中断。
  - 进入 ISR 后，先读 A 相当前电平，才能判断刚才发生的是上升沿还是下降沿。
  - 2 倍频只对 A 相做中断配置，B 相仍然只用于方向判断。
- 本实验新增的术语、模块或函数：
  - `CHANGE`
  - `risingCount`
  - `fallingCount`
  - `countDualEdge()`
  - 2 倍频
- 开始本实验前需要特别提醒的点：
  - 双边沿统计时不能简单写成“B 高加 1、B 低减 1”，否则同一方向的上升沿和下降沿可能相互抵消。
  - 进入中断后，A 相若为 `HIGH`，说明刚发生上升沿；A 相若为 `LOW`，说明刚发生下降沿。

### 2. 实验3接线
- 接线表：

| 编码器引脚 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `A` | `GPIO 6` | `CHANGE` 双边沿中断 |
| `B` | `GPIO 7` | 方向判断参考信号 |
| `VCC` | 按模块额定电压 | 注意逻辑兼容 |
| `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用 CHANGE 同时统计编码器 A 相的上升沿和下降沿，实现双边沿2倍频计数。
  使用方法：编码器 A、B 相接 GPIO 6、7，上传后打开 115200 串口并正反转动编码器。
  运行结果：串口显示总计数、上升沿次数和下降沿次数；每个 A 相周期计数2次。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 定义编码器 A 相和 B 相引脚
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

// 这三个变量都可能在中断中被修改，所以都要使用 volatile
volatile long encoderCount = 0;
volatile long risingCount = 0;
volatile long fallingCount = 0;

// CHANGE 表示 A 相无论上升沿还是下降沿都会进入这个中断函数
void IRAM_ATTR countDualEdge() {
  // 先读取当前 A 相和 B 相电平
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 如果此时 A 相是高电平，说明刚刚发生的是上升沿
  if (levelA == HIGH) {
    risingCount = risingCount + 1;
  } else {
    // 否则说明刚刚发生的是下降沿
    fallingCount = fallingCount + 1;
  }

  // 上升沿时，B 相为高记正转，为低记反转
  if (levelA == HIGH) {
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 下降沿时，同一正转方向下，B 相应为低电平
  if (levelA == LOW) {
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  // 初始化串口
  Serial.begin(115200);

  // 把编码器 A、B 两相都设置为上拉输入
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // 只给 A 相配置 CHANGE 中断，实现基础 2 倍频
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    countDualEdge,
    CHANGE
  );
}

void loop() {
  // 安全复制三个共享变量
  noInterrupts();
  long countCopy = encoderCount;
  long risingCopy = risingCount;
  long fallingCopy = fallingCount;
  interrupts();

  // 在串口中同时显示总计数、上升沿次数和下降沿次数
  Serial.print("2X Count:");
  Serial.print(countCopy);
  Serial.print("  RISING:");
  Serial.print(risingCopy);
  Serial.print("  FALLING:");
  Serial.println(fallingCopy);

  // 每 200ms 输出一次
  delay(200);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义 A/B 相引脚和三个共享变量。
  2. 初始化串口与输入引脚。
  3. 把 A 相 `CHANGE` 和 `countDualEdge()` 绑定。
  4. 每当 A 相发生边沿变化，中断函数读取 A/B 两相电平。
  5. 先判断这是上升沿还是下降沿，再根据 B 相判断方向并修改总计数。
  6. 主循环复制三个变量并输出。
- `setup()` 的作用：
  - 配置输入引脚并启用 A 相 `CHANGE` 中断。
- `loop()` 的作用：
  - 安全读取共享数据，显示双边沿计数结果。
- 数据输入、处理、输出的执行过程：
  - 输入：A 相边沿变化，B 相逻辑电平。
  - 处理：区分上升沿 / 下降沿，并完成方向加减计数。
  - 输出：串口显示总计数、上升沿数、下降沿数。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `attachInterrupt(..., countDualEdge, CHANGE)`
  - `if (levelA == HIGH)`
  - `volatile long risingCount`
  - `volatile long fallingCount`
- 每个函数的作用、写法、参数和返回值：
  - `CHANGE`
    - 含义：引脚电平发生变化时触发中断。
    - 作用：同时捕获上升沿和下降沿。
  - `countDualEdge()`
    - 函数作用：在双边沿触发时区分边沿类型并统计方向。
    - 参数：无。
    - 返回值：无返回值。
  - `digitalRead(ENCODER_A_PIN)`
    - 函数作用：读取 A 相当前电平，用来判断刚才到来的是哪一种边沿。
    - 参数1：要读取的 A 相引脚号。
    - 返回值：`HIGH` 或 `LOW`。
- 关键代码片段分析：
  - `if (levelA == HIGH)` 和 `if (levelA == LOW)` 是本实验的核心，因为同样的 B 相电平在“上升沿”和“下降沿”中代表的方向含义不同。
  - `risingCount`、`fallingCount` 的存在能帮助学生验证：`CHANGE` 的确把两种边沿都统计到了。
- 为什么这样写：
  - 这一步把“单边沿计数”升级成“基础 2 倍频计数”，是后面距离和测速计算的基础。

---

## 实验4

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验在实验 3 的 2 倍频基础上，进一步把计数换算成距离和速度。
  - 距离计算依赖“每圈计数”和“车轮周长”。
  - 速度计算依赖“这一段时间内计数变化量”和“经过的时间”。
  - TFT 界面采用“固定标签只画一次，动态区域局部刷新”的写法，减少闪烁。
- 本实验新增的术语、模块或函数：
  - `millis()`
  - `drawStaticScreen()`
  - `updateScreen()`
  - `fillRect()`
  - `COUNTS_PER_REVOLUTION`
  - `WHEEL_DIAMETER_MM`
- 开始本实验前需要特别提醒的点：
  - 本程序默认“单边沿每圈 430 次”，2 倍频后每圈为 `860` 次。
  - 若实际编码器或车轮参数不同，要先重新标定。
  - TFT 接线为 `CS=47`、`DC=21`、`MOSI=20`、`SCK=19`、`RST=-1`。

### 2. 实验4接线
- 接线表：

| 模块 | ESP32-S3 | 说明 |
| --- | --- | --- |
| 编码器 `A` | `GPIO 6` | `CHANGE` 2 倍频输入 |
| 编码器 `B` | `GPIO 7` | 方向判断 |
| 编码器 `VCC` | 按模块额定电压 | 注意逻辑兼容 |
| 编码器 `GND` | `GND` | 必须共地 |
| TFT `CS` | `GPIO 47` | 片选 |
| TFT `DC` | `GPIO 21` | 数据/命令选择 |
| TFT `SDA(MOSI)` | `GPIO 20` | SPI 数据线 |
| TFT `SCL(SCK)` | `GPIO 19` | SPI 时钟线 |
| TFT `RST` | 不接 | 程序中设为 `-1` |
| TFT `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用编码器 2 倍频中断计数，在 ST7789 TFT 上显示计数、距离和速度。
  使用方法：编码器 A/B 接 GPIO 6/7并连接TFT；程序已按60mm轮径、单边沿每圈430次设置。
  运行结果：TFT 每 200ms 更新一次 Count、Distance(cm) 和 Speed(cm/s)，反转时数值为负。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 引入 TFT 显示需要的库文件
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// 定义编码器 A 相和 B 相引脚
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

// 实测：车轮转一圈时，单边沿计数约为 430 次
const int SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430;

// 当前程序采用 2 倍频统计，所以倍频系数为 2
const int ENCODER_MULTIPLY = 2;

// 每圈总计数 = 单边沿每圈计数 × 倍频系数
const int COUNTS_PER_REVOLUTION =
  SINGLE_EDGE_COUNTS_PER_REVOLUTION * ENCODER_MULTIPLY;

// 车轮直径，单位为毫米
const float WHEEL_DIAMETER_MM = 60.0;

// 定义 TFT 屏幕接线引脚
const int8_t TFT_CS_PIN = 47;
const int8_t TFT_DC_PIN = 21;
const int8_t TFT_RST_PIN = -1;
const int8_t TFT_SDA_PIN = 20;
const int8_t TFT_SCL_PIN = 19;

// 定义屏幕原生宽高和横屏方向
const uint16_t TFT_NATIVE_WIDTH = 240;
const uint16_t TFT_NATIVE_HEIGHT = 320;
const uint8_t TFT_ROTATION = 3;

// 使用软件 SPI 方式创建 ST7789 屏幕对象
Adafruit_ST7789 screen(
  TFT_CS_PIN,
  TFT_DC_PIN,
  TFT_SDA_PIN,
  TFT_SCL_PIN,
  TFT_RST_PIN
);

// 中断函数和主循环共享的计数变量
volatile long encoderCount = 0;

// 用于速度计算的上一次计数和上一次时刻
long lastCount = 0;
unsigned long lastMeasureTime = 0;

// 使用 A 相双边沿实现 2 倍频
void IRAM_ATTR countEncoder2x() {
  // 读取当前 A 相和 B 相电平
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 若此时 A 相为高，说明刚刚出现的是上升沿
  if (levelA == HIGH) {
    // 上升沿时，B 相为高记正转
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 若此时 A 相为低，说明刚刚出现的是下降沿
  if (levelA == LOW) {
    // 下降沿时，B 相为低记同一个正转方向
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  // 编码器两相都设置为上拉输入
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // 只给 A 相配置 CHANGE 中断，实现 2 倍频计数
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    countEncoder2x,
    CHANGE
  );

  // 初始化 TFT 屏幕，并设置为横屏
  screen.init(TFT_NATIVE_WIDTH, TFT_NATIVE_HEIGHT);
  screen.setRotation(TFT_ROTATION);
  screen.setTextWrap(false);

  // 绘制固定标题和标签
  drawStaticScreen();

  // 记录初始测速时间
  lastMeasureTime = millis();
}

void loop() {
  // 获取当前时间
  unsigned long currentTime = millis();

  // 每隔 200ms 刷新一次数据
  if (currentTime - lastMeasureTime >= 200) {
    // 安全复制编码器总计数
    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    // 计算这段时间内经过了多久，以及计数变化了多少
    unsigned long timeChange = currentTime - lastMeasureTime;
    long countChange = currentCount - lastCount;

    // 车轮周长 = π × 直径，毫米转厘米要除以 10
    float wheelLengthCm = PI * WHEEL_DIAMETER_MM / 10.0;

    // 累计距离 = 当前总计数 × 每次计数对应的位移
    float distanceCm = currentCount * wheelLengthCm
                     / COUNTS_PER_REVOLUTION;

    // 速度 = 本周期内距离变化 ÷ 时间变化
    float speedCmPerSecond = countChange * wheelLengthCm
                           / COUNTS_PER_REVOLUTION
                           * 1000.0 / timeChange;

    // 把最新数据更新到屏幕上
    updateScreen(currentCount, distanceCm, speedCmPerSecond);

    // 保存本次数据，作为下次测速的“上一次状态”
    lastCount = currentCount;
    lastMeasureTime = currentTime;
  }
}

void drawStaticScreen() {
  // 清屏为黑色背景
  screen.fillScreen(ST77XX_BLACK);

  // 设置标题文字大小和颜色
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_CYAN);
  screen.setCursor(30, 12);
  screen.print("ENCODER 2X MONITOR");

  // 画一条分隔线
  screen.drawFastHLine(0, 40, 320, ST77XX_BLUE);

  // 显示固定标签
  screen.setTextColor(ST77XX_WHITE);
  screen.setCursor(12, 62);
  screen.print("Count:");
  screen.setCursor(12, 112);
  screen.print("Distance:");
  screen.setCursor(12, 162);
  screen.print("Speed:");

  // 底部补充单位说明
  screen.setTextSize(1);
  screen.setCursor(12, 220);
  screen.print("Distance: cm    Speed: cm/s");
}

void updateScreen(long countValue, float distanceCm, float speedCmPerSecond) {
  // 设置动态数据显示的文字大小和颜色
  screen.setTextSize(2);
  screen.setTextColor(ST77XX_YELLOW);

  // 先擦除旧的 Count 数据区域，再显示新的 Count
  screen.fillRect(135, 58, 180, 24, ST77XX_BLACK);
  screen.setCursor(140, 62);
  screen.print(countValue);

  // 先擦除旧的 Distance 数据区域，再显示新的距离值
  screen.fillRect(135, 108, 180, 24, ST77XX_BLACK);
  screen.setCursor(140, 112);
  screen.print(distanceCm, 2);

  // 先擦除旧的 Speed 数据区域，再显示新的速度值
  screen.fillRect(135, 158, 180, 24, ST77XX_BLACK);
  screen.setCursor(140, 162);
  screen.print(speedCmPerSecond, 2);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 引入 TFT 库并定义编码器、屏幕、参数常量。
  2. 在 `setup()` 中初始化编码器输入、中断和 TFT 屏幕。
  3. 绘制静态界面，包括标题、分隔线和固定标签。
  4. `loop()` 中每隔 200ms 复制一次计数值。
  5. 根据计数变化量和时间变化量计算累计距离与当前速度。
  6. 使用局部刷新方式更新屏幕中的数值区域。
- `setup()` 的作用：
  - 配置 2 倍频中断，初始化 TFT，并绘制固定界面。
- `loop()` 的作用：
  - 周期性完成“取样、计算、显示”三个动作。
- 数据输入、处理、输出的执行过程：
  - 输入：编码器 A/B 两相信号和 `millis()` 时间信息。
  - 处理：中断函数完成计数，主循环完成距离、速度换算。
  - 输出：TFT 显示当前总计数、距离和速度。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `millis()`
  - `drawStaticScreen()`
  - `updateScreen(...)`
  - `screen.fillRect(...)`
  - `screen.setRotation(TFT_ROTATION)`
- 每个函数的作用、写法、参数和返回值：
  - `millis()`
    - 函数作用：返回程序开始运行到当前的毫秒数。
    - 参数：无。
    - 返回值：`unsigned long` 类型毫秒值。
  - `drawStaticScreen()`
    - 函数作用：绘制不会频繁改变的固定界面内容。
    - 参数：无。
    - 返回值：无返回值。
  - `updateScreen(long countValue, float distanceCm, float speedCmPerSecond)`
    - 函数作用：刷新计数、距离和速度数值区域。
    - 参数1：当前总计数。
    - 参数2：累计距离，单位厘米。
    - 参数3：当前速度，单位厘米每秒。
    - 返回值：无返回值。
  - `screen.fillRect(x, y, w, h, color)`
    - 函数作用：用指定颜色填充矩形区域，常用于“先擦后写”。
    - 参数1：矩形左上角 `x` 坐标。
    - 参数2：矩形左上角 `y` 坐标。
    - 参数3：矩形宽度。
    - 参数4：矩形高度。
    - 参数5：填充颜色。
    - 返回值：无返回值。
  - `screen.setRotation(rotation)`
    - 函数作用：设置屏幕显示方向。
    - 参数1：方向编号。
    - 返回值：无返回值。
- 关键代码片段分析：
  - `float distanceCm = currentCount * wheelLengthCm / COUNTS_PER_REVOLUTION;` 把抽象的“脉冲数”转换成了实际可理解的“距离”。
  - `float speedCmPerSecond = countChange * wheelLengthCm / COUNTS_PER_REVOLUTION * 1000.0 / timeChange;` 表示速度不是直接读出来的，而是“距离变化 ÷ 时间变化”。
  - `fillRect() + print()` 是 TFT 局部刷新常见写法，比整屏重绘更稳定。
- 为什么这样写：
  - 这个实验把“中断计数”提升为“运动量测量”，让编码器真正和小车里程、测速等实际任务连接起来。

---

## 实验5

### 1. 实验5预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 本实验属于扩展内容，学习目标是理解 4 倍频思路，不要求所有学生都完全掌握。
  - 4 倍频会同时对 A 相和 B 相配置 `CHANGE` 中断。
  - 程序不再只看一个时刻的 B 相高低，而是比较“上一次 AB 状态”和“当前 AB 状态”的转移关系。
  - 常见正转状态序列是 `00 -> 01 -> 11 -> 10 -> 00`，反转则相反。
- 本实验新增的术语、模块或函数：
  - `lastEncoderState`
  - `currentState`
  - `forward`
  - `reverse`
  - 4 倍频
- 开始本实验前需要特别提醒的点：
  - 4 倍频更容易受到抖动和非法跳变影响。
  - 基础测速、TFT 显示和后续 PID 控制一般仍以 2 倍频方案为主。

### 2. 实验5接线
- 接线表：

| 编码器引脚 | ESP32-S3 | 说明 |
| --- | --- | --- |
| `A` | `GPIO 6` | `CHANGE` 中断输入 |
| `B` | `GPIO 7` | `CHANGE` 中断输入 |
| `VCC` | 按模块额定电压 | 注意逻辑兼容 |
| `GND` | `GND` | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：扩展学习编码器 4 倍频，对 A、B 两相的全部边沿进行计数。
  使用方法：完成 2 倍频实验后再使用；编码器 A、B 相接 GPIO 6、7并打开115200串口。
  运行结果：每个完整 A/B 周期最多计数 4 次，串口显示扩展实验的 4X Count。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 定义编码器 A 相和 B 相引脚
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

// 共享计数变量
volatile long encoderCount = 0;

// 记录上一次的 AB 组合状态，00、01、10、11 会被编码成 0、1、2、3
volatile int lastEncoderState = 0;

// 扩展内容：A 相和 B 相的每次边沿变化都会进入这个中断函数
void IRAM_ATTR countEncoder4x() {
  // 读取当前 A 相和 B 相电平
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 用 2 位状态编码当前 AB 组合
  int currentState = levelA * 2 + levelB;

  // 判断是否符合正转状态序列
  bool forward =
    (lastEncoderState == 0 && currentState == 1) ||
    (lastEncoderState == 1 && currentState == 3) ||
    (lastEncoderState == 3 && currentState == 2) ||
    (lastEncoderState == 2 && currentState == 0);

  // 判断是否符合反转状态序列
  bool reverse =
    (lastEncoderState == 0 && currentState == 2) ||
    (lastEncoderState == 2 && currentState == 3) ||
    (lastEncoderState == 3 && currentState == 1) ||
    (lastEncoderState == 1 && currentState == 0);

  // 若匹配正转，则计数加 1
  if (forward) {
    encoderCount = encoderCount + 1;
  } else if (reverse) {
    // 若匹配反转，则计数减 1
    encoderCount = encoderCount - 1;
  }

  // 保存本次状态，供下一次边沿比较使用
  lastEncoderState = currentState;
}

void setup() {
  // 初始化串口
  Serial.begin(115200);

  // 设置编码器两相为上拉输入
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  // 先记录上电时的初始 AB 状态
  lastEncoderState = digitalRead(ENCODER_A_PIN) * 2
                   + digitalRead(ENCODER_B_PIN);

  // A 相和 B 相都使用 CHANGE 中断，实现 4 倍频
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    countEncoder4x,
    CHANGE
  );
  attachInterrupt(
    digitalPinToInterrupt(ENCODER_B_PIN),
    countEncoder4x,
    CHANGE
  );

  // 串口提示当前程序属于扩展实验
  Serial.println("这是 4 倍频扩展实验");
}

void loop() {
  // 安全复制共享变量
  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  // 显示 4 倍频计数结果
  Serial.print("4X Count:");
  Serial.println(countCopy);

  // 每 200ms 输出一次
  delay(200);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 定义 A/B 相引脚、总计数和上一次状态变量。
  2. 在 `setup()` 中初始化串口、输入模式和初始状态。
  3. 给 A 相、B 相都绑定 `CHANGE` 中断。
  4. 每次任一相发生边沿变化时，读取新的 AB 组合状态。
  5. 比较“旧状态 -> 新状态”属于正转还是反转。
  6. 在主循环中输出 4 倍频计数值。
- `setup()` 的作用：
  - 初始化编码器输入，记录上电初始状态，并启用 A/B 两相中断。
- `loop()` 的作用：
  - 安全读取当前总计数并输出到串口。
- 数据输入、处理、输出的执行过程：
  - 输入：A/B 两相的每一次边沿变化。
  - 处理：把 AB 电平编码成状态，并比较状态转移方向。
  - 输出：串口显示 `4X Count`。

### 5. 核心函数及代码讲解
- 本实验新出现的关键函数/语句：
  - `attachInterrupt(... ENCODER_A_PIN ..., CHANGE)`
  - `attachInterrupt(... ENCODER_B_PIN ..., CHANGE)`
  - `int currentState = levelA * 2 + levelB`
  - `bool forward = ...`
  - `bool reverse = ...`
- 每个函数的作用、写法、参数和返回值：
  - `currentState = levelA * 2 + levelB`
    - 作用：把 A/B 两相组合成一个 2 位状态值。
    - 结果范围：`0`、`1`、`2`、`3`，分别对应 `00`、`01`、`10`、`11`。
  - `forward`
    - 作用：判断当前状态转移是否符合正转顺序。
    - 类型：布尔变量，结果为 `true` 或 `false`。
  - `reverse`
    - 作用：判断当前状态转移是否符合反转顺序。
    - 类型：布尔变量，结果为 `true` 或 `false`。
- 关键代码片段分析：
  - `00 -> 01 -> 11 -> 10 -> 00` 这类状态转移判断，是 4 倍频和 2 倍频最大的思维差异。
  - 4 倍频不再只关注“这一瞬间 B 相是高还是低”，而是关注“AB 状态如何变化”。
- 为什么这样写：
  - 让学生接触更完整的编码器解码思想，为后续更高精度计数、运动控制和抗抖动处理做铺垫。

---

## 总结
- 实验目标：
  - 学会使用上升沿、下降沿和双边沿中断统计编码器脉冲。
  - 学会利用 B 相判断正转和反转。
  - 学会把编码器计数转换为距离与速度。
  - 学会把运动数据显示到 TFT 屏幕上。
  - 了解 4 倍频的基本思想和状态转移判断方法。
- 单词：
  - `interrupt`：中断
  - `encoder`：编码器
  - `rising`：上升的
  - `falling`：下降的
  - `change`：变化
  - `count`：计数
  - `pulse`：脉冲
  - `edge`：边沿
  - `signal`：信号
  - `forward`：正向 / 正转
  - `reverse`：反向 / 反转
  - `shared`：共享的
  - `copy`：复制
  - `distance`：距离
  - `speed`：速度
  - `measure`：测量
  - `wheel`：车轮
  - `diameter`：直径
  - `rotation`：旋转
  - `monitor`：监视器 / 监测界面
  - `volatile`：易变的
  - `attach`：附加 / 绑定
  - `digital`：数字的
  - `read`：读取
  - `print`：打印 / 输出
- 接线：
  - 编码器 `A -> GPIO 6`
  - 编码器 `B -> GPIO 7`
  - 编码器 `VCC -> 按模块额定电压`
  - 编码器 `GND -> GND`
  - TFT `CS -> GPIO 47`
  - TFT `DC -> GPIO 21`
  - TFT `SDA(MOSI) -> GPIO 20`
  - TFT `SCL(SCK) -> GPIO 19`
  - TFT `RST -> 不接，代码中为 -1`
  - 所有模块 `GND` 必须共地
- 关键函数和参数：
  - `attachInterrupt(digitalPinToInterrupt(pin), isr, mode)`
  - `RISING`
  - `FALLING`
  - `CHANGE`
  - `digitalRead(pin)`
  - `noInterrupts()`
  - `interrupts()`
  - `millis()`
  - `screen.init(240, 320)`
  - `screen.setRotation(3)`
  - `screen.fillRect(x, y, w, h, color)`
  - `SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430`
  - `COUNTS_PER_REVOLUTION = 860`
  - `WHEEL_DIAMETER_MM = 60.0`
- 关键程序：
  - 上升沿中断计数程序
  - 下降沿中断计数程序
  - 双边沿 2 倍频计数程序
  - 编码器 TFT 距离速度显示程序
  - 4 倍频扩展程序
- 实验现象：
  - 不同中断模式下，同样转一圈，计数结果不同。
  - 2 倍频计数通常约为单边沿计数的 2 倍。
  - TFT 会实时显示计数、距离和速度，并随转动方向变化而显示正负值。
  - 4 倍频程序在同样一圈下通常会得到更多计数。
- 本课重点：
  - 中断函数要短，只做计数和少量判断，不做耗时工作。
  - 中断共享变量必须重视 `volatile` 和安全复制。
  - 倍频方式变化后，每圈计数参数也要跟着修改。
  - 距离和速度本质上都来自“计数换算”。
