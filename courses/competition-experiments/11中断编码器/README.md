# ESP32-S3 外部中断与编码器课程

本课程共 3 课时，使用编码器 A 相 GPIO 6、B 相 GPIO 7，并使用青潮智科
ST7789 TFT 方案显示累计距离和速度。

## 学习目标

1. 理解轮询和外部中断的区别。
2. 区分 `RISING`、`FALLING`、`CHANGE`。
3. 理解编码器 A/B 两相的相位关系和旋转方向判断。
4. 掌握以 A 相 `CHANGE` 实现的 2 倍频计数。
5. 了解作为扩展内容的 A/B 两相 4 倍频。
6. 会把计数换算为车轮距离和速度。
7. 会在中断函数外安全读取计数并刷新 TFT。

## 接线

| 编码器 | ESP32-S3 |
|---|---:|
| A 相 | GPIO 6 |
| B 相 | GPIO 7 |
| VCC | 按模块额定电压 |
| GND | GND |

| TFT | ESP32-S3 |
|---|---:|
| CS | GPIO 47 |
| DC | GPIO 21 |
| SDA/MOSI | GPIO 20 |
| SCL/SCK | GPIO 19 |
| RST | 不接，代码为 `-1` |

编码器输出必须与 ESP32-S3 的 3.3V 逻辑兼容。若编码器输出为 5V，先使用合适的
电平转换或分压电路，不能直接输入 GPIO。

## 五个实验与知识讲义

### 0. 最基础的上升沿中断

文件：`00_rising_edge_interrupt/00_rising_edge_interrupt.ino`

只使用 A 相 `RISING`，每次上升沿计数一次，并读取 B 相判断方向。建议先完成这个程序，
再分别学习下降沿和双边沿。

### 1. 下降沿中断

文件：`01_falling_edge_interrupt/01_falling_edge_interrupt.ino`

只使用 A 相 `FALLING`，每次下降沿计数一次。它和上升沿程序结构相同，主要区别是
`attachInterrupt()` 的最后一个参数，以及下降沿到来时用于判断方向的 B 相电平。

### 2. 双边沿2倍频基础实验

文件：`02_dual_edge_count/02_dual_edge_count.ino`

使用 `CHANGE` 同时检测 A 相的上升沿和下降沿，并分别显示 `RISING`、`FALLING`
次数和有方向的总计数。基础课程只使用 A 相中断：

- 上升沿到来时，读取B相；B为高记正转。
- 下降沿到来时，读取B相；B为低才记作同一个正转方向。
- `digitalRead()` 读取的是逻辑高低电平，不是具体的电压数值。

| 检测方式 | 使用的边沿 | 每个完整 A 相周期的计数 |
|---|---|---:|
| 1倍频 | A 相上升沿 | 1 |
| 2倍频 | A 相上升沿+下降沿 | 2 |

2倍频的核心代码：

```cpp
attachInterrupt(
  digitalPinToInterrupt(ENCODER_A_PIN),
  countEncoder2x,
  CHANGE
);
```

它只给 A 相配置中断，B 相只用于判断旋转方向，语法比4倍频更容易理解。

### 3. TFT 显示距离和速度

文件：`03_encoder_tft_distance_speed/03_encoder_tft_distance_speed.ino`

先确认编码器参数：

```cpp
const int SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430;
const int COUNTS_PER_REVOLUTION =
  SINGLE_EDGE_COUNTS_PER_REVOLUTION * 2;
const float WHEEL_DIAMETER_MM = 60.0;
```

当前参数来自实测：车轮直径60mm，车轮转一圈时单边沿计数430次。双边沿会同时统计
上升沿和下降沿，因此每圈计数为 `430×2=860`。

基础程序按2倍频计算：

```text
每圈双边沿计数 = 每圈单边沿计数 × 2 = 430 × 2 = 860
```

### 4. 4 倍频扩展实验

文件：`04_encoder_4x_extension/04_encoder_4x_extension.ino`

4倍频会同时检测 A、B 两相的全部边沿，并判断 `00→01→11→10` 等状态变化。
这部分不用于基础实验、TFT测速或PID程序，只供完成2倍频后继续研究。

### 中断知识讲义

`INTERRUPT_KNOWLEDGE.md` 汇总上升沿、下降沿、CHANGE、`volatile`、`IRAM_ATTR`、
中断标志位、共享变量和 ISR 禁止事项，并提供可直接复制的代码模板。

距离换算：

```text
车轮周长 = π × 轮径
距离 = 总计数 ÷ 每圈计数 × 车轮周长
```

速度换算：

```text
速度 = 一段时间内的距离变化 ÷ 时间变化
```

## 3 课时安排

### 第 1 课时：代码讲解（一）

1. `pinMode(INPUT_PULLUP)`。
2. `digitalPinToInterrupt()`。
3. `attachInterrupt()`。
4. `IRAM_ATTR` 中断函数。
5. `volatile` 计数变量。
6. 上升沿、下降沿和方向判断。

### 第 2 课时：代码讲解（二）+原理

1. A/B 两相波形。
2. 格雷码状态 `00→01→11→10→00`。
3. 1倍频与2倍频。
4. 非法跳变和机械抖动。
5. 复制中断计数时短暂关闭中断。
6. 轮径、PPR、距离和速度公式。

### 第 3 课时：实验

1. 正转和反转编码器，确认计数方向相反。
2. 分别运行 RISING、FALLING、CHANGE，比较相同一圈内的计数。
3. 在轮胎上做起点标记，手动转一整圈并校准每圈计数。
4. 修改轮径，检查 TFT 距离结果。
5. 快速和缓慢转动车轮，比较速度显示。
6. 交换 A/B 引脚，观察方向符号变化。
7. 学有余力时再运行4倍频扩展实验，对比每圈计数。

## 常见问题

- **计数方向相反：** 交换 A、B 接线，或交换加减号。
- **静止时计数跳动：** 检查共地、线缆、上拉和信号电压，必要时增加硬件滤波。
- **一圈计数不对：** 编码器手册的 PPR、CPR 定义可能不同，应以实测为准。
- **速度跳动：** 增大测速周期，或对多次速度结果求平均。
- **TFT 白屏：** 检查 CS47、DC21、MOSI20、SCK19 和 ST7789 库。

产品声明：本课程及示例程序为青潮智科产品配套教学内容。
