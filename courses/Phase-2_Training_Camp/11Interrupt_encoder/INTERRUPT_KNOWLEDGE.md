# ESP32 外部中断常用知识与代码模板

本讲义配合 `00_rising_edge_interrupt`、`01_falling_edge_interrupt` 和
`02_dual_edge_count` 使用，分别对应上升沿、下降沿和双边沿；4倍频只作为扩展内容。

## 1. 什么是外部中断

普通轮询是主程序不断询问“引脚变化了吗”；外部中断是在指定边沿到来时，CPU 暂停
当前普通代码，先执行一个很短的中断函数，完成后再返回原来的位置。

适合中断的信号：

- 编码器脉冲。
- 按键或限位开关的紧急事件。
- 传感器的数据就绪信号。
- 需要及时捕获的上升沿或下降沿。

## 2. 最基本的上升沿中断

```cpp
const int SIGNAL_PIN = 6;
volatile long pulseCount = 0;

void IRAM_ATTR signalRising() {
  pulseCount = pulseCount + 1;
}

void setup() {
  pinMode(SIGNAL_PIN, INPUT_PULLUP);
  attachInterrupt(
    digitalPinToInterrupt(SIGNAL_PIN),
    signalRising,
    RISING
  );
}
```

`RISING` 表示引脚从 `LOW` 变为 `HIGH` 时触发。

## 3. 常用触发方式

| 模式 | 触发条件 | 编码器用途 |
|---|---|---|
| `RISING` | LOW→HIGH | A 相 1×计数 |
| `FALLING` | HIGH→LOW | 单独研究下降沿 |
| `CHANGE` | 电平发生变化 | 基础实验使用A相2倍频 |

示例：

```cpp
attachInterrupt(digitalPinToInterrupt(6), onRise, RISING);
attachInterrupt(digitalPinToInterrupt(6), onFall, FALLING);
attachInterrupt(digitalPinToInterrupt(6), onChange, CHANGE);
```

同一个引脚同一时间只保留最后一次配置，不要把这三行同时用于一个引脚。需要比较时，
一次选择一种模式重新上传。

## 4. 在 CHANGE 中区分上升沿和下降沿

```cpp
volatile long risingCount = 0;
volatile long fallingCount = 0;

void IRAM_ATTR signalChange() {
  if (digitalRead(6) == HIGH) {
    risingCount = risingCount + 1;
  } else {
    fallingCount = fallingCount + 1;
  }
}
```

进入中断后引脚已经是 `HIGH`，说明刚发生上升沿；已经是 `LOW`，说明刚发生下降沿。

## 5. 使用 B 相判断编码器方向

```cpp
void IRAM_ATTR encoderRising() {
  if (digitalRead(7) == HIGH) {
    encoderCount = encoderCount + 1;
  } else {
    encoderCount = encoderCount - 1;
  }
}
```

A 相上升沿到来时，B 相的高低电平代表两个相反旋转方向。如果实物方向与程序正负号
相反，可以交换 A/B 接线，或交换加减号。

这里的“读取B相电压”实际是用 `digitalRead()` 读取逻辑高低电平，不是测量具体电压值。
ESP32-S3 GPIO输入必须使用兼容的3.3V逻辑电平。

双边沿计数时需要先判断A相当前电平：

```cpp
if (levelA == HIGH) {
  // 刚发生上升沿：B为高记正转
  if (levelB == HIGH) encoderCount = encoderCount + 1;
  else encoderCount = encoderCount - 1;
} else {
  // 刚发生下降沿：B为低才记同一个正转方向
  if (levelB == LOW) encoderCount = encoderCount + 1;
  else encoderCount = encoderCount - 1;
}
```

如果双边沿始终使用“B高加1、B低减1”，同一方向的一次上升沿和一次下降沿会相互抵消。

## 6. 为什么使用 volatile

```cpp
volatile long encoderCount = 0;
```

中断函数可能随时修改变量。`volatile` 告诉编译器每次都从真实存储位置读取，不要假设
该变量没有变化。中断与 `loop()` 共同使用的计数、标志位通常要加 `volatile`。

## 7. 中断函数只置标志，复杂工作放在 loop

```cpp
volatile bool eventFlag = false;

void IRAM_ATTR signalRising() {
  eventFlag = true;
}

void loop() {
  if (eventFlag == true) {
    noInterrupts();
    eventFlag = false;
    interrupts();

    // 串口、屏幕和复杂计算放在这里
    Serial.println("RISING EVENT");
  }
}
```

这种“中断置标志、主循环处理”的写法非常常见。

## 8. 安全读取中断共享计数

```cpp
noInterrupts();
long countCopy = encoderCount;
interrupts();
```

关闭中断的时间必须尽量短，只复制变量，不在中间打印串口、刷新屏幕或延时。

## 9. IRAM_ATTR

```cpp
void IRAM_ATTR encoderRising() {
  // 快速代码
}
```

ESP32 示例常把中断函数标记为 `IRAM_ATTR`，使关键中断代码放在内部 RAM 中。课堂中断
函数统一保留这个标记。

## 10. 中断函数中的禁止事项

中断函数中不要执行：

- `delay()`。
- `Serial.print()`。
- `String` 拼接和动态内存操作。
- TFT/OLED 整屏刷新。
- I2C、SPI 长通信。
- 很长的 `for`、`while` 循环。
- 电机 PID 完整计算。

中断函数只做计数、读取少量 GPIO 或设置标志。

## 11. 按键消抖与编码器抗干扰

- 机械按键一次动作可能产生多次跳变，优先在 `loop()` 中做消抖。
- 编码器高速脉冲不能简单增加很长的消抖时间，否则会漏计数。
- 编码器应检查上拉、共地、电平、电缆长度和屏蔽。
- 扩展的4倍频解码应忽略 `00→11` 这类非法跨越状态。
- 噪声严重时使用硬件 RC、施密特触发器或带差分输出的编码器接口。

## 12. 常见错误

- 忘记把共享变量写成 `volatile`。
- 在中断函数里打印串口，导致运行异常。
- A/B 没有共地，计数随机跳动。
- 输入 5V 信号直接进入 ESP32-S3 3.3V GPIO。
- ISR 太长，造成其他中断延迟或系统复位。
- 修改倍频后忘记同步修改每圈计数。

产品声明：本讲义为青潮智科产品配套教学内容。
