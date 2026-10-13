# 双轮电机项目：英文单词、函数和参数中文说明

这份说明按照当前项目代码整理，适合刚开始学习 Arduino 和 C++ 的同学。

遇到英文名称时，不需要把它当成一个很长的单词。可以把它拆开理解。例如：

```text
targetSpeed = target（目标）+ speed（速度）= 目标速度
motor_set_pwm = motor（电机）+ set（设置）+ PWM = 设置电机PWM
encoder_update = encoder（编码器）+ update（更新）= 更新编码器数据
```

## 一、项目中每个文件的作用

| 文件 | 中文作用 |
|---|---|
| `sketch_jul27b.ino` | 主程序，负责把按键、编码器、PID和电机库组合起来 |
| `button_control.h` | 按键库头文件，声明按键引脚和按键函数 |
| `button_control.cpp` | 按键库实现文件，真正读取按键 |
| `encoder.h` | 编码器库头文件，声明编码器函数和速度、距离变量 |
| `encoder.cpp` | 编码器库实现文件，计数并计算速度和距离 |
| `motor_driver.h` | 电机库头文件，声明电机控制函数 |
| `motor_driver.cpp` | 电机库实现文件，设置方向和PWM |
| `speed_pid.h` | PID库头文件，定义PID保存的数据和函数 |
| `speed_pid.cpp` | PID库实现文件，计算PID输出 |

### `.h` 和 `.cpp` 的区别

- `.h` 读作“头文件”，主要写函数声明，告诉主程序“有这个函数”。
- `.cpp` 读作“C加加文件”，主要写函数实现，说明函数具体怎么工作。
- 主程序通过 `#include "文件名.h"` 使用这个库。

例如：

```cpp
// button_control.h：函数声明
void button_init();

// button_control.cpp：函数实现
void button_init()
{
  // 具体代码
}

// 主程序：调用函数
button_init();
```

## 二、程序从上电到运行的顺序

```text
开发板上电
  ↓
setup() 只运行一次
  ↓
初始化串口、按键、电机、编码器和PID
  ↓
loop() 不断重复运行
  ↓
读取按键
  ↓
每50ms读取编码器并计算PID
  ↓
把PID输出交给电机
  ↓
每500ms打印速度和PWM
```

## 三、最常见的程序英文单词

| 英文 | 中文 | 在程序中的意思 |
|---|---|---|
| `include` | 包含、引入 | 引入一个库的头文件 |
| `define` | 定义 | 定义头文件标记 |
| `if` | 如果 | 条件成立就执行代码 |
| `else` | 否则 | `if` 不成立时执行 |
| `while` | 当……期间 | 条件成立时一直重复 |
| `return` | 返回、结束 | 返回结果并结束当前函数 |
| `const` | 不变的 | 变量创建后不允许修改 |
| `extern` | 外部的 | 变量在其他 `.cpp` 文件中定义 |
| `volatile` | 随时可能变化 | 中断会修改这个变量，不能随意优化掉 |
| `struct` | 结构体 | 把多个相关变量放在一起 |
| `void` | 无返回值 | 函数执行后不返回结果 |
| `int` | 整数 | 例如 `1`、`10`、`150` |
| `long` | 长整数 | 可以保存比普通整数更大的数 |
| `float` | 小数 | 例如 `5.0`、`12.27` |
| `bool` | 真或假 | 只有 `true` 和 `false` |
| `unsigned` | 无符号 | 变量不保存负数 |
| `true` | 真、成立 | 条件成立 |
| `false` | 假、不成立 | 条件不成立 |
| `HIGH` | 高电平 | 通常表示数字1 |
| `LOW` | 低电平 | 通常表示数字0 |
| `INPUT` | 输入 | GPIO用于读取信号 |
| `OUTPUT` | 输出 | GPIO用于输出信号 |
| `PULLUP` | 上拉 | 默认把输入保持为高电平 |
| `RISING` | 上升沿 | 电平从LOW变成HIGH的瞬间 |
| `PIN` | 引脚 | 开发板上的GPIO |
| `MS` | 毫秒 | 1000毫秒等于1秒 |
| `Seconds` | 秒 | 时间单位“秒” |
| `A`、`B` | A路、B路 | 表示左右两个电机或编码器 |

## 四、常用符号的意思

| 符号 | 中文作用 | 示例 |
|---|---|---|
| `=` | 把右边的值交给左边 | `speedA = 0` |
| `==` | 判断左右是否相等 | `buttonState == LOW` |
| `>=` | 大于或等于 | `pwmA >= 0` |
| `+=` | 在原来的值上增加 | `distanceA += 1` |
| `-=` | 在原来的值上减少 | `targetSpeed -= 5` |
| `++` | 增加1 | `countA++` |
| `--` | 减少1 | `countA--` |
| `!` | 取反、“不是” | `!isRunning` 表示没有运行 |
| `||` | 或者 | 其中一个条件成立即可 |
| `&` | 取得变量地址 | `&pidA` 把 pidA 的位置交给函数 |
| `*` | 指针 | `PID *pid` 表示接收一个PID变量的地址 |
| `->` | 访问指针里的成员 | `pid->kp` 表示这个PID里的kp |
| `? :` | 二选一 | 条件成立选前面，否则选后面 |

## 五、主程序中的英文变量

### 时间和速度常量

| 名称 | 单词拆分 | 中文作用 | 当前值 |
|---|---|---|---:|
| `CONTROL_TIME_MS` | control控制 + time时间 + ms毫秒 | PID计算间隔 | 50ms |
| `PRINT_TIME_MS` | print打印 + time时间 + ms毫秒 | 串口打印间隔 | 500ms |
| `MAX_TARGET_SPEED` | max最大 + target目标 + speed速度 | 最大目标速度 | 50cm/s |
| `SPEED_STEP` | speed速度 + step步进 | 每按一次改变的速度 | 5cm/s |
| `DEFAULT_TARGET_SPEED` | default默认 + target目标 + speed速度 | 开始时默认速度 | 5cm/s |

### PID参数

| 名称 | 中文 | 当前值 | 作用 |
|---|---|---:|---|
| `KP` | 比例参数 | 1.0 | 根据当前误差快速调整 |
| `KI` | 积分参数 | 0.5 | 累积误差，减小长期速度偏差 |
| `KD` | 微分参数 | 0.0 | 根据误差变化提前抑制波动，当前未使用 |
| `pidA` | A轮PID数据 | — | 保存A轮PID参数和历史误差 |
| `pidB` | B轮PID数据 | — | 保存B轮PID参数和历史误差 |

### 运行变量

| 名称 | 单词拆分 | 中文作用 | 单位 |
|---|---|---|---|
| `targetSpeed` | target目标 + speed速度 | 两个轮子的目标速度 | cm/s |
| `pwmA` | PWM + A路 | A电机的PID输出 | PWM数值 |
| `pwmB` | PWM + B路 | B电机的PID输出 | PWM数值 |
| `isRunning` | is是否 + running运行 | 程序是否允许电机运行 | 真或假 |
| `lastControlTime` | last上次 + control控制 + time时间 | 上一次计算PID的时间 | ms |
| `lastPrintTime` | last上次 + print打印 + time时间 | 上一次串口打印的时间 | ms |
| `currentTime` | current当前 + time时间 | 当前运行时间 | ms |
| `runningTargetSpeed` | running运行 + target目标 + speed速度 | 串口显示用的当前目标速度 | cm/s |

## 六、主程序自定义函数

### 1. `setup()`：初始化函数

英文：`setup` = 设置、准备。

```cpp
void setup()
```

- 参数：没有参数。
- 返回值：`void`，不返回结果。
- 调用次数：开发板每次上电或复位后只运行一次。
- 作用：初始化串口、按键、电机、编码器和PID。

### 2. `loop()`：循环函数

英文：`loop` = 循环。

```cpp
void loop()
```

- 参数：没有参数。
- 返回值：无。
- 调用次数：初始化完成后一直重复运行。
- 作用：读取按键、定时计算PID、定时打印数据。

### 3. `stopMotors()`：停止两个电机

英文：`stop` = 停止，`motors` = 多个电机。

```cpp
void stopMotors()
```

- 参数：没有参数。
- 返回值：无。
- 作用：清除A/B轮PID，把两个PWM设为0，然后停止电机。

### 4. `readKeys()`：读取按键

英文：`read` = 读取，`keys` = 多个按键。

```cpp
void readKeys()
```

- 参数：没有参数。
- 返回值：无。
- 作用：读取停止、开始、加速、减速按键，并修改运行状态或目标速度。

### 5. `controlMotors()`：控制电机

英文：`control` = 控制，`motors` = 多个电机。

```cpp
void controlMotors()
```

- 参数：没有参数。
- 返回值：无。
- 作用：更新编码器速度，计算两个PID，然后把PWM输出给电机。
- PID真正被使用的位置就在这个函数中。

## 七、按键库单词、函数和参数

### 当前按键引脚

| 名称 | GPIO | 功能 |
|---|---:|---|
| `buttonPin1` | 10 | 加速 |
| `buttonPin2` | 11 | 减速 |
| `buttonPin3` | 15 | 停止 |
| `buttonPin4` | 16 | 开始 |

单词：`button` = 按钮，`Pin` = 引脚。

### 1. `button_init()`：初始化按键

```cpp
void button_init()
```

- `button`：按钮。
- `init`：`initialize` 的缩写，意思是初始化。
- 参数：没有参数。
- 返回值：无。
- 作用：把4个GPIO设置成内部上拉输入。
- 使用位置：主程序的 `setup()`。

### 2. `button_read(int buttonPin)`：读取一个按键

```cpp
int button_read(int buttonPin)
```

参数：

| 参数 | 类型 | 中文定义 |
|---|---|---|
| `buttonPin` | `int`整数 | 要读取的按键GPIO编号 |

返回值：

- 返回 `1`：按键已经按下并松开。
- 返回 `0`：按键没有按下。

当前写法是阻塞式：检测到按下后先等待50ms消抖，再一直等待按键松开。

调用示例：

```cpp
button_read(buttonPin3);
```

意思是读取停止按钮。

### 3. `button_print()`：打印按键电平

```cpp
void button_print()
```

- 参数：没有参数。
- 返回值：无。
- 作用：打印4个按键当前是高电平还是低电平。
- 当前状态：这个函数写在 `button_control.cpp` 中，但没有在头文件中声明，主程序也没有调用它。

### 按键库内部变量

| 名称 | 中文作用 |
|---|---|
| `buttonState` | 当前读取到的按键状态 |
| `buttonPin` | 本次需要读取的按键引脚 |

## 八、编码器库单词、函数和参数

### 当前编码器引脚

| 名称 | GPIO | 中文作用 |
|---|---:|---|
| `encoderA_PINA` | 4 | A轮编码器A相信号 |
| `encoderB_PINA` | 5 | A轮编码器B相信号，用于判断方向 |
| `encoderA_PINB` | 6 | B轮编码器A相信号 |
| `encoderB_PINB` | 7 | B轮编码器B相信号，用于判断方向 |

这里名称中的最后一个 `A` 或 `B` 表示轮子，前面的 `encoderA`、`encoderB` 表示编码器的A相和B相。

### 编码器变量

| 名称 | 单词拆分 | 中文作用 | 单位 |
|---|---|---|---|
| `WHEEL_TOTAL_PULSE` | wheel轮子 + total总数 + pulse脉冲 | 轮子一圈的脉冲数 | 个 |
| `DISTANCE_PER_PULSE` | distance距离 + per每个 + pulse脉冲 | 每个脉冲对应的距离 | cm |
| `countA` | count计数 + A轮 | A轮中断累计脉冲 | 个 |
| `countB` | count计数 + B轮 | B轮中断累计脉冲 | 个 |
| `pulseA` | pulse脉冲 + A轮 | 本次计算取出的A轮脉冲 | 个 |
| `pulseB` | pulse脉冲 + B轮 | 本次计算取出的B轮脉冲 | 个 |
| `speedA` | speed速度 + A轮 | A轮实际速度 | cm/s |
| `speedB` | speed速度 + B轮 | B轮实际速度 | cm/s |
| `distanceA` | distance距离 + A轮 | A轮累计距离 | cm |
| `distanceB` | distance距离 + B轮 | B轮累计距离 | cm |

每个脉冲的距离计算：

```text
每个脉冲距离 = 圆周率 × 轮子直径6cm ÷ 一圈430个脉冲
```

### 1. `encoderA_count()`：A轮脉冲计数

```cpp
void IRAM_ATTR encoderA_count()
```

- `encoder`：编码器。
- `count`：计数。
- 参数：没有参数。
- 返回值：无。
- 作用：A轮编码器A相出现上升沿时自动执行，根据B相电平决定加1还是减1。
- `IRAM_ATTR`：把中断函数放在ESP32内部快速存储区域，适合中断调用。

### 2. `encoderB_count()`：B轮脉冲计数

```cpp
void IRAM_ATTR encoderB_count()
```

- 参数：没有参数。
- 返回值：无。
- 作用：B轮编码器A相出现上升沿时自动执行，根据B相电平决定方向。

### 3. `encoder_init()`：初始化编码器

```cpp
void encoder_init()
```

- 参数：没有参数。
- 返回值：无。
- 作用：设置4个编码器输入引脚，并给两路A相信号安装中断。
- 使用位置：主程序 `setup()`。

### 4. `encoder_update(float intervalSeconds)`：更新速度和距离

```cpp
void encoder_update(float intervalSeconds)
```

参数：

| 参数 | 类型 | 中文定义 |
|---|---|---|
| `intervalSeconds` | `float`小数 | 本次测速使用的时间间隔，单位是秒 |

主程序调用：

```cpp
encoder_update(CONTROL_TIME_MS / 1000.0f);
```

`CONTROL_TIME_MS` 是50毫秒，除以1000后得到 `0.05` 秒。

速度公式：

```text
速度 = 本次脉冲数 × 每个脉冲距离 ÷ 时间间隔
```

## 九、电机库单词、函数和参数

### 电机引脚和变量

| 名称 | GPIO/数值 | 中文作用 |
|---|---:|---|
| `PWMA_PIN` | 1 | A电机PWM引脚 |
| `PWMB_PIN` | 42 | B电机PWM引脚 |
| `DIRA_PIN` | 2 | A电机方向引脚 |
| `DIRB_PIN` | 41 | B电机方向引脚 |
| `MOTOR_MAX_PWM` | 150 | 电机允许的最大PWM绝对值 |

单词：

- `motor`：电机。
- `driver`：驱动器。
- `PWM`：脉冲宽度调制，用来控制电机功率。
- `DIR`：`direction` 的缩写，意思是方向。
- `MAX`：最大。

### 1. `motor_init()`：初始化电机

```cpp
void motor_init()
```

- 参数：没有参数。
- 返回值：无。
- 作用：把PWM和方向引脚设置成输出，并在上电时将PWM设为0。
- 使用位置：主程序 `setup()`。

### 2. `motor_set_pwm(int pwmA, int pwmB)`：设置电机PWM

```cpp
void motor_set_pwm(int pwmA, int pwmB)
```

参数：

| 参数 | 类型 | 中文定义 |
|---|---|---|
| `pwmA` | `int`整数 | A电机PWM，范围限制为-150～150 |
| `pwmB` | `int`整数 | B电机PWM，范围限制为-150～150 |

- 正数：正转。
- 负数：反转。
- 0：停止。
- B电机安装方向和A电机相反，所以B方向引脚的HIGH/LOW也相反。

调用示例：

```cpp
motor_set_pwm(100, 100);
```

表示两个轮子按相同运动方向输出100大小的PWM。

## 十、PID库单词、变量、函数和参数

### PID是什么意思

PID通过比较“目标速度”和“实际速度”，自动改变PWM。

```text
目标速度 - 实际速度 = 误差
误差较大 → PID改变PWM
实际速度逐渐接近目标速度
```

### `PID` 结构体中的成员

| 名称 | 单词拆分 | 中文定义 |
|---|---|---|
| `kp` | K + proportional比例 | 比例参数 |
| `ki` | K + integral积分 | 积分参数 |
| `kd` | K + derivative微分 | 微分参数 |
| `maxOutput` | max最大 + output输出 | 最大输出 |
| `minOutput` | min最小 + output输出 | 最小输出 |
| `lastError` | last上一次 + error误差 | 上一次误差 |
| `previousError` | previous更早一次 + error误差 | 上上次误差 |
| `output` | 输出 | 当前PID输出 |

### 1. `PID_Init(...)`：初始化PID

英文：`Init` = 初始化。

```cpp
void PID_Init(PID *pid,
              float kp,
              float ki,
              float kd,
              float maxOutput,
              float minOutput);
```

参数：

| 参数 | 类型 | 中文定义 |
|---|---|---|
| `pid` | `PID *`指针 | 要初始化哪一个PID，例如A轮或B轮 |
| `kp` | `float`小数 | 比例参数 |
| `ki` | `float`小数 | 积分参数 |
| `kd` | `float`小数 | 微分参数 |
| `maxOutput` | `float`小数 | PID允许的最大输出 |
| `minOutput` | `float`小数 | PID允许的最小输出 |

主程序中的调用：

```cpp
PID_Init(&pidA, KP, KI, KD, MOTOR_MAX_PWM, -MOTOR_MAX_PWM);
```

中文理解：初始化A轮PID，使用KP、KI、KD，并把输出限制在-150～150。

### 2. `PID_Clear(PID *pid)`：清除PID历史

英文：`Clear` = 清除。

```cpp
void PID_Clear(PID *pid)
```

参数：

| 参数 | 类型 | 中文定义 |
|---|---|---|
| `pid` | `PID *`指针 | 要清除的PID，例如 `&pidA` 或 `&pidB` |

返回值：无。

作用：把上一次误差、上上次误差和输出清零。停止或重新开始时清除，可以避免旧误差影响下一次运行。

### 3. `PID_IncPIDCal(...)`：计算增量式PID

名称拆分：

- `Inc`：`incremental` 的缩写，增量式。
- `PID`：PID控制。
- `Cal`：`calculate` 的缩写，计算。

```cpp
float PID_IncPIDCal(PID *pid,
                    float currentSpeed,
                    float targetSpeed);
```

参数：

| 参数 | 类型 | 中文定义 | 单位 |
|---|---|---|---|
| `pid` | `PID *`指针 | 使用哪个轮子的PID数据 | — |
| `currentSpeed` | `float`小数 | 编码器测到的当前实际速度 | cm/s |
| `targetSpeed` | `float`小数 | 希望电机达到的目标速度 | cm/s |

返回值：`float` 小数，表示计算后的PWM控制量。

主程序中的调用：

```cpp
pwmA = PID_IncPIDCal(&pidA, speedA, targetSpeed);
pwmB = PID_IncPIDCal(&pidB, speedB, targetSpeed);
```

第一行中文意思：使用A轮PID，把A轮实际速度和目标速度进行比较，计算结果保存到 `pwmA`。

### PID函数在什么地方使用

| 程序位置 | PID函数 | 作用 |
|---|---|---|
| `setup()` | `PID_Init()` | 上电后初始化两个PID |
| `controlMotors()` | `PID_IncPIDCal()` | 每50ms计算一次PWM |
| `stopMotors()` | `PID_Clear()` | 停止时清除历史误差 |
| 开始按钮处理 | `PID_Clear()` | 重新开始前清除旧误差 |

## 十一、Arduino自带函数和参数

### `pinMode(pin, mode)`：设置GPIO工作方式

| 参数 | 中文定义 |
|---|---|
| `pin` | GPIO编号 |
| `mode` | 工作方式，例如 `INPUT_PULLUP` 或 `OUTPUT` |

### `digitalRead(pin)`：读取数字电平

| 参数 | 中文定义 |
|---|---|
| `pin` | 要读取的GPIO编号 |

返回 `HIGH` 或 `LOW`。

### `digitalWrite(pin, state)`：输出数字电平

| 参数 | 中文定义 |
|---|---|
| `pin` | 要控制的GPIO编号 |
| `state` | 输出 `HIGH` 或 `LOW` |

### `analogWrite(pin, value)`：输出PWM

| 参数 | 中文定义 |
|---|---|
| `pin` | PWM输出GPIO |
| `value` | PWM大小，本项目传入0～150 |

### `attachInterrupt(pin, function, mode)`：安装中断

| 参数 | 中文定义 |
|---|---|
| `pin` | 检测信号的GPIO |
| `function` | 信号出现时自动执行的函数 |
| `mode` | 触发方式，本项目使用 `RISING` 上升沿 |

### `delay(milliseconds)`：等待

| 参数 | 中文定义 |
|---|---|
| `milliseconds` | 等待多少毫秒 |

例如 `delay(50)` 表示等待50毫秒。

### `millis()`：获取开机时间

- 参数：没有参数。
- 返回值：开发板从启动到现在经过的毫秒数。

### `noInterrupts()`：暂时关闭中断

- 参数：没有参数。
- 用途：读取编码器计数时，避免读取一半被中断修改。

### `interrupts()`：重新打开中断

- 参数：没有参数。
- 用途：读取并清零计数后恢复编码器中断。

### `constrain(value, minimum, maximum)`：限制范围

| 参数 | 中文定义 |
|---|---|
| `value` | 要限制的值 |
| `minimum` | 最小允许值 |
| `maximum` | 最大允许值 |

例如：

```cpp
constrain(pwmA, -150, 150);
```

表示 `pwmA` 不能小于-150，也不能大于150。

### `abs(value)`：取绝对值

- 参数 `value`：需要去掉正负号的数。
- 例如 `abs(-100)` 的结果是 `100`。

### `Serial.begin(baudRate)`：初始化串口

| 参数 | 中文定义 |
|---|---|
| `baudRate` | 串口波特率，本项目为115200 |

### `Serial.println(content)`：打印一行

| 参数 | 中文定义 |
|---|---|
| `content` | 需要打印的文字或数值 |

打印完成后自动换行。

### `Serial.printf(format, values...)`：按格式打印

| 参数 | 中文定义 |
|---|---|
| `format` | 输出格式，例如保留几位小数 |
| `values` | 按顺序填入格式中的变量 |

常见格式：

| 格式 | 中文作用 |
|---|---|
| `%.1f` | 打印小数并保留1位 |
| `%.2f` | 打印小数并保留2位 |
| `\n` | 换行 |

## 十二、串口数据怎么看

主程序打印顺序：

```text
A轮目标速度,A轮实际速度,A轮PWM,B轮目标速度,B轮实际速度,B轮PWM
```

例如：

```text
15.0,12.27,80.0,15.0,11.40,85.0
```

表示：

- A轮目标速度：15cm/s。
- A轮实际速度：12.27cm/s。
- A轮PWM：80。
- B轮目标速度：15cm/s。
- B轮实际速度：11.40cm/s。
- B轮PWM：85。

如果出现：

```text
15.0,-12.27,150.0,15.0,0.00,150.0
```

表示A轮编码器方向可能相反，B轮可能没有编码器反馈。PID因为误差很大，把PWM增加到了最大值150。

## 十三、最需要记住的内容

1. `.h` 声明函数，`.cpp` 实现函数，主程序通过 `#include` 使用库。
2. `setup()` 只运行一次，`loop()` 一直重复。
3. `button_read()` 负责读取按键。
4. `encoder_update()` 负责计算实际速度。
5. `PID_IncPIDCal()` 比较实际速度和目标速度，得到PWM。
6. `motor_set_pwm()` 把PWM真正输出给电机。
7. 电机正转时，编码器速度也必须是正数，PID才能正常工作。
8. 第一次实车测试要架空车轮，防止方向或反馈错误导致电机突然满速。
