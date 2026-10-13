# 第二模块 实验

> 学习顺序建议：先做不接电机的实验1，看懂“追目标”；再做调速和位置；最后做巡线小车。电机上电前必须请老师检查接线，并先把车轮抬离桌面。

## 实验1 不接电机也能看的 PID 闭环

### 1. 实验1预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - `Target` 是目标，`Input` 是模拟出来的当前值，`Output` 是 PID 给出的控制力度。
  - 程序用一条简单公式假装有一台真实机器，因此只接 USB 线就能学习 PID。
  - 每 `100ms` 计算一次，当前值会一点点靠近目标，而不是瞬间跳到目标。
- 本实验新增的术语、模块或函数：
  - `PID` 结构体：像一个小书包，装着参数、过去的误差和上一次输出。
  - `PID_Init()`：给 PID 设置参数和输出范围。
  - `PID_Clear()`：清空旧记忆，但不改变 P、I、D 参数。
  - `PID_IncPIDCal()`：进行一次增量式 PID 计算并返回输出。
- 开始本实验前需要特别提醒的点：
  - `speed_pid.h`、`speed_pid.cpp` 和主程序必须放在同一个文件夹。
  - 串口设为 `115200`；可以用串口绘图器观察三条曲线。
  - 发送 `target 80` 或 `target 30` 时，行结尾选择 `Newline`。

### 2. 实验1接线
- 接线表：

| 设备 | 连接方式 | 说明 |
| --- | --- | --- |
| ESP32-S3 | USB 连接电脑 | 供电、上传和串口通信 |
| 电机和传感器 | 不连接 | 本实验全部由软件模拟 |

### 3. 完整程序及注释讲解

主程序 `01_pid_closed_loop.ino`：

```cpp
/*
  程序功能：用软件模拟对象演示 PID 闭环控制，不连接电机也能观察目标值跟随过程。
  使用方法：打开 115200 串口监视器或绘图器；可发送 target 80、target 30 改变目标。
  运行结果：Input 逐渐接近 Target，串口持续显示 Target、Input、Output 三条数据。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "speed_pid.h"

float targetValue = 80;
float processValue = 0;
unsigned long lastControlTime = 0;

// 创建一组PID数据
PID demoPid;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  // 参数顺序：PID、P、I、D、输出最大值、输出最小值
  PID_Init(&demoPid, 0.5, 0.05, 0.1, 100, 0);
  PID_Clear(&demoPid);
  lastControlTime = millis();

  Serial.println("输入 target 80 修改目标值");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("target ")) {
      targetValue = command.substring(7).toFloat();
      targetValue = constrain(targetValue, 0, 100);
      PID_Clear(&demoPid);
    }
  }

  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 100) {
    float output = PID_IncPIDCal(&demoPid, processValue, targetValue);

    // 用简单公式模拟真实对象响应：输出越大，过程值上升越快
    processValue = processValue + (output - processValue) * 0.08;

    Serial.print("Target:");
    Serial.print(targetValue);
    Serial.print(",Input:");
    Serial.print(processValue);
    Serial.print(",Output:");
    Serial.println(output);

    lastControlTime = currentTime;
  }
}
```

公共工具文件 `speed_pid.h`：

```cpp
#ifndef SPEED_PID_H
#define SPEED_PID_H

#include <Arduino.h>

// PID运行时保存的参数和历史数据
typedef struct {
  // PID参数
  float P;
  float I;
  float D;

  // 输出限幅
  float OutputMax;
  float OutputMin;

  // 历史误差
  float LastError;
  float PrevError;

  // 输出值
  float Output;
} PID;

// 函数声明
void PID_Init(PID *pid, float Kp, float Ki, float Kd,
              float maxValue, float minValue);
void PID_Clear(PID *pid);
float PID_IncPIDCal(PID *pid, float NowValue, float AimValue);

#endif
```

公共工具文件 `speed_pid.cpp`：

```cpp
#include "speed_pid.h"

// PID初始化：设置PID参数和输出范围
void PID_Init(PID *pid, float Kp, float Ki, float Kd,
              float maxValue, float minValue) {
  // 最大值和最小值相同，说明范围无效
  if (maxValue == minValue) {
    return;
  }

  pid->P = Kp;
  pid->I = Ki;
  pid->D = Kd;
  pid->OutputMax = maxValue;
  pid->OutputMin = minValue;
}

// 清除历史误差和输出，不改变PID参数
void PID_Clear(PID *pid) {
  pid->LastError = 0.0;
  pid->PrevError = 0.0;
  pid->Output = 0.0;
}

// 增量式PID计算
float PID_IncPIDCal(PID *pid, float NowValue, float AimValue) {
  // 当前误差 = 目标值 - 当前值
  float error = AimValue - NowValue;

  // 计算本次应该增加或减少多少输出
  float increment =
      pid->P * (error - pid->LastError)
    + pid->I * error
    + pid->D * (error - 2 * pid->LastError + pid->PrevError);

  // 把本次增量累加到原来的输出
  pid->Output = pid->Output + increment;

  // 输出限幅，避免输出超过执行器允许范围
  if (pid->Output > pid->OutputMax) {
    pid->Output = pid->OutputMax;
  }

  if (pid->Output < pid->OutputMin) {
    pid->Output = pid->OutputMin;
  }

  // 保存误差，供下一次计算使用
  pid->PrevError = pid->LastError;
  pid->LastError = error;

  return pid->Output;
}
```

> 后面三个实验目录中的 `speed_pid.h` 和 `speed_pid.cpp` 与上面完全相同。打开每个实验时，都要保留该目录里的这两个文件。

### 4. 程序主流程及执行过程
- 程序整体分几步执行：
  1. 创建一组 PID 数据，并设置初始目标 `80`。
  2. 初始化 P、I、D 和 `0～100` 的输出范围。
  3. 读取串口中的新目标。
  4. 每 `100ms` 计算一次 PID 输出。
  5. 用输出模拟当前值的变化，再打印三组数据。
- `setup()` 的作用：打开串口并准备 PID。
- `loop()` 的作用：读取命令，按固定周期计算、模拟和显示。
- 数据输入、处理、输出的执行过程：目标命令 → 误差 → PID 输出 → 模拟对象变化 → 新的当前值 → 下一轮反馈。

### 5. 核心函数及代码讲解
- `PID_Init(&demoPid, 0.5, 0.05, 0.1, 100, 0)`：P 为 `0.5`、I 为 `0.05`、D 为 `0.1`，输出限制在 `0～100`。
- `PID_Clear(&demoPid)`：目标改变时把旧误差和旧输出清零，让新任务重新开始。
- `PID_IncPIDCal(&demoPid, processValue, targetValue)`：参数依次是 PID 数据、当前值、目标值，返回本次输出。
- `processValue = processValue + (output - processValue) * 0.08`：让模拟值每次只向输出靠近一点，模仿真实机器有速度、不能瞬间到达。
- `pid->Output` 中的箭头 `->`：表示找到这个 PID 小书包里的 `Output` 数据。
- 为什么这样写：先用没有危险的模拟实验看懂数据之间的关系，再接真实电机时会更容易排查问题。

---

## 实验2 PID 电机调速

### 1. 实验2预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 编码器 A 相的上升沿和下降沿都会计数，所以是 `2` 倍频，每圈按 `860` 次计算。
  - RPM 表示每分钟转数。
  - 程序每 `100ms` 计算一次当前 RPM，再让 PID 调整 PWM。
- 本实验新增的术语、模块或函数：
  - `attachInterrupt()`：发生指定电平变化时调用计数函数。
  - `volatile`：提醒程序这个变量可能被中断随时改变。
  - `noInterrupts()` / `interrupts()`：复制计数时暂时关、开中断，避免数字读到一半被改变。
- 开始本实验前需要特别提醒的点：
  - 先抬起车轮，再发送低目标速度。
  - 若计数方向或电机方向不对，应先改方向，不能靠把 PID 参数调得很大来“硬顶”。

### 2. 实验2接线
- 接线表：

| 模块 | 信号 | ESP32-S3 |
| --- | --- | --- |
| 电机驱动 | DIR | `GPIO 2` |
| 电机驱动 | PWM | `GPIO 1` |
| 编码器 | A 相 | `GPIO 6` |
| 编码器 | B 相 | `GPIO 7` |
| 电机驱动 | 电机电源 | 独立合适电源 |
| 所有模块 | GND | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用编码器测量电机转速，通过 PID 自动调整 PWM，使转速接近目标 RPM。
  使用方法：电机 DIR/PWM 接 GPIO2/1、编码器 A/B 接 GPIO6/7；串口发送 speed 100 或 stop。
  运行结果：串口显示 Target、RPM、PWM，负载变化后 PID 会调整 PWM 恢复目标转速。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "speed_pid.h"

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;
// 实测单边沿每圈430次，双边沿2倍频后每圈860次
const int SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430;
const int COUNTS_PER_REVOLUTION =
  SINGLE_EDGE_COUNTS_PER_REVOLUTION * 2;

volatile long encoderCount = 0;

float targetRpm = 100;
long lastCount = 0;
unsigned long lastControlTime = 0;

// 保存速度环PID参数、历史误差和输出
PID speedPid;

// 只处理 A 相的上升沿和下降沿，即 2 倍频
void IRAM_ATTR countEncoder2x() {
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 上升沿：读取B相，B为高记作正转
  if (levelA == HIGH) {
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 下降沿：同一正转方向下，B相应为低电平
  if (levelA == LOW) {
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  digitalWrite(MOTOR_DIR_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, 0);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder2x, CHANGE);

  // 使用speed_pid.cpp示例中的初始参数
  // 参数顺序：PID、P、I、D、输出最大值、输出最小值
  PID_Init(&speedPid, 0.5, 1.4, 0.2, 255, 0);
  PID_Clear(&speedPid);
  lastControlTime = millis();

  Serial.println("输入 speed 100 修改目标转速，输入 stop 停止");
}

void loop() {
  readSerialCommand();

  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 100) {
    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    long countChange = currentCount - lastCount;
    unsigned long timeChange = currentTime - lastControlTime;

    float currentRpm = abs(countChange) * 60000.0
                     / COUNTS_PER_REVOLUTION / timeChange;
    float pwmValue = PID_IncPIDCal(&speedPid, currentRpm, targetRpm);

    if (targetRpm <= 0) {
      pwmValue = 0;
    }

    analogWrite(MOTOR_PWM_PIN, (int)pwmValue);

    Serial.print("Target:");
    Serial.print(targetRpm);
    Serial.print(",RPM:");
    Serial.print(currentRpm);
    Serial.print(",PWM:");
    Serial.println(pwmValue);

    lastCount = currentCount;
    lastControlTime = currentTime;
  }
}

void readSerialCommand() {
  if (Serial.available() == 0) {
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.startsWith("speed ")) {
    targetRpm = command.substring(6).toFloat();
    targetRpm = constrain(targetRpm, 0, 300);
    PID_Clear(&speedPid);
  } else if (command == "stop") {
    targetRpm = 0;
    PID_Clear(&speedPid);
    analogWrite(MOTOR_PWM_PIN, 0);
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：初始化电机、编码器和 PID；中断持续计数；读取速度命令；每 `100ms` 计算 RPM；PID 计算 PWM；输出并打印。
- `setup()` 的作用：让电机先保持停止，再建立编码器中断和速度 PID。
- `loop()` 的作用：处理命令，并按稳定周期完成“测速—计算—调速”。
- 数据输入、处理、输出的执行过程：编码器计数 → RPM → 与目标速度比较 → PID PWM → 电机 → 新计数。

### 5. 核心函数及代码讲解
- `attachInterrupt(..., CHANGE)`：A 相每次从低变高或从高变低都调用 `countEncoder2x()`。
- RPM 公式：`计数变化 × 60000 ÷ 每圈计数 ÷ 毫秒变化`。
- `PID_Init(&speedPid, 0.5, 1.4, 0.2, 255, 0)`：速度输出只能是 `0～255`。
- `speed 100`：把目标限制在 `0～300 RPM`；`stop`：目标和 PWM 都变为 `0`。
- 为什么这样写：测速和控制使用同一个 `100ms` 周期，PID 每次计算的时间间隔比较稳定。

---

## 实验3 PID 电机位置控制

### 1. 实验3预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 速度控制关心“每分钟转多快”，位置控制关心“累计走到了哪里”。
  - PID 输出可以是正数或负数：正负号决定方向，绝对值决定 PWM 大小。
  - 离目标只差 `2` 个计数以内时停下，避免在目标附近不停来回纠正。
- 本实验新增的术语、模块或函数：
  - `position`：位置。
  - `zero`：把当前位置当作新的零点。
  - `dead band`：停止小范围，可理解为“已经足够接近”。
- 开始本实验前需要特别提醒的点：
  - 先发送较小的位置，例如 `position 100`。
  - 若电机越走离目标越远，要立即发送 `stop` 或断电，请老师检查反馈方向。

### 2. 实验3接线
- 接线与实验2相同：DIR `GPIO 2`、PWM `GPIO 1`、编码器 A `GPIO 6`、B `GPIO 7`，电机独立供电，所有 GND 共地。

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：使用编码器位置作为反馈，通过 PID 控制电机转到指定计数位置。
  使用方法：电机和编码器按 GPIO2/1、GPIO6/7 接线；串口发送 position 400、position -400 或 zero。
  运行结果：电机自动选择方向并接近目标计数，串口显示 Target、Position、Error、PWM。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "speed_pid.h"

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

volatile long encoderCount = 0;

long targetPosition = 0;
unsigned long lastControlTime = 0;
unsigned long lastPrintTime = 0;

// 保存位置环PID参数、历史误差和输出
PID positionPid;

// 只处理 A 相的上升沿和下降沿，即 2 倍频
void IRAM_ATTR countEncoder2x() {
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 上升沿：读取B相，B为高记作正转
  if (levelA == HIGH) {
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 下降沿：同一正转方向下，B相应为低电平
  if (levelA == LOW) {
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  analogWrite(MOTOR_PWM_PIN, 0);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder2x, CHANGE);

  // 位置控制需要正负输出，因此最大值255、最小值-255
  PID_Init(&positionPid, 2.0, 0.0, 0.08, 255, -255);
  PID_Clear(&positionPid);
  lastControlTime = millis();

  Serial.println("命令：position 400、position -400、zero、stop");
}

void loop() {
  readSerialCommand();

  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 20) {
    noInterrupts();
    long currentPosition = encoderCount;
    interrupts();

    float output = PID_IncPIDCal(
      &positionPid,
      currentPosition,
      targetPosition
    );
    float error = targetPosition - currentPosition;

    if (abs(error) <= 2) {
      analogWrite(MOTOR_PWM_PIN, 0);
    } else {
      int pwmValue = abs((int)output);

      // 小 PWM 可能无法克服电机静摩擦
      if (pwmValue > 0 && pwmValue < 40) {
        pwmValue = 40;
      }

      if (output >= 0) {
        digitalWrite(MOTOR_DIR_PIN, LOW);
      } else {
        digitalWrite(MOTOR_DIR_PIN, HIGH);
      }

      analogWrite(MOTOR_PWM_PIN, pwmValue);
    }

    if (currentTime - lastPrintTime >= 100) {
      Serial.print("Target:");
      Serial.print(targetPosition);
      Serial.print(",Position:");
      Serial.print(currentPosition);
      Serial.print(",Error:");
      Serial.print(error);
      Serial.print(",Output:");
      Serial.println(output);
      lastPrintTime = currentTime;
    }

    lastControlTime = currentTime;
  }
}

void readSerialCommand() {
  if (Serial.available() == 0) {
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.startsWith("position ")) {
    targetPosition = command.substring(9).toInt();
    PID_Clear(&positionPid);
  } else if (command == "zero") {
    noInterrupts();
    encoderCount = 0;
    interrupts();
    targetPosition = 0;
    PID_Clear(&positionPid);
  } else if (command == "stop") {
    noInterrupts();
    targetPosition = encoderCount;
    interrupts();
    PID_Clear(&positionPid);
    analogWrite(MOTOR_PWM_PIN, 0);
  }
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：编码器计数；读取位置命令；每 `20ms` 计算位置 PID；选方向和 PWM；每 `100ms` 打印数据。
- `setup()` 的作用：初始化电机、编码器和可输出正负值的位置 PID。
- `loop()` 的作用：反复比较目标位置与当前位置，并决定下一小步怎样走。
- 数据输入、处理、输出的执行过程：目标位置 + 编码器位置 → 位置误差 → 正负 PID 输出 → 方向与 PWM → 电机新位置。

### 5. 核心函数及代码讲解
- `PID_Init(..., 255, -255)`：位置输出范围是 `-255～255`，负数用来表示反方向。
- `abs(error) <= 2`：离目标非常近就停止，这是允许的小误差范围。
- `pwmValue < 40` 时改为 `40`：太小的 PWM 可能推不动静止的电机。
- `zero`：安全地清空编码器计数并把目标设为零。
- `stop`：把当前计数设为新目标，相当于告诉电机“就停在这里”。
- 为什么这样写：把“方向”和“力度”分别处理，孩子可以清楚看到输出正负号与 PWM 大小各自负责什么。

---

## 实验4 PID 八路巡线小车

### 1. 实验4预备知识
- 本实验在共用预备知识基础上，额外需要知道的内容：
  - 八个通道从左到右的权重是 `-70、-50、-30、-10、10、30、50、70`。
  - 黑线在中间时，左右权重平均后接近 `0`；偏向一侧时会得到正或负误差。
  - PID 输出是左右轮的修正量：一边减，另一边加，小车就会转向。
- 本实验新增的术语、模块或函数：
  - `LINE_THRESHOLD`：判断是不是黑线的阈值。
  - `weight`：权重，表示每只“眼睛”离中心有多远。
  - `correction`：PID 算出的转向修正量。
  - `LINE LOST`：八路都没找到线时的提示。
- 开始本实验前需要特别提醒的点：
  - 先运行 I2C 扫描器，确认模块地址是 `0x27`。
  - 不同赛道和光照下，`1000` 不一定是合适阈值，要先记录传感器数值。
  - 必须抬起车轮确认左右轮方向和修正方向，正确后才能放上赛道。

### 2. 实验4接线
- 接线表：

| 模块 | 信号 | ESP32-S3 |
| --- | --- | --- |
| 左电机驱动 | PWM | `GPIO 42` |
| 左电机驱动 | DIR | `GPIO 41` |
| 右电机驱动 | PWM | `GPIO 1` |
| 右电机驱动 | DIR | `GPIO 2` |
| 巡线模块 | SDA | `GPIO 8` |
| 巡线模块 | SCL | `GPIO 9` |
| 巡线模块 | 地址 | `0x27` |
| 所有模块 | GND | 必须共地 |

### 3. 完整程序及注释讲解

```cpp
/*
  程序功能：读取八路 I2C 巡线模块，用 PID 计算左右轮速度差，实现巡线小车控制。
  使用方法：巡线模块接 GPIO8/9，左右电机按代码引脚接线；抬起车轮确认方向后放到赛道。
  运行结果：小车根据黑线位置连续修正左右轮速度；丢失黑线时停止，串口显示 Error 和速度。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "speed_pid.h"

const int MOTOR_L_PWM_PIN = 42;
const int MOTOR_L_DIR_PIN = 1;
const int MOTOR_R_PWM_PIN = 41;
const int MOTOR_R_DIR_PIN = 2;

const int SDA_PIN = 8;
const int SCL_PIN = 9;
const uint8_t LINE_SENSOR_ADDRESS = 0x27;
const uint8_t LINE_READ_COMMAND = 0x01;
const uint8_t LINE_DATA_LENGTH = 32;
const uint16_t LINE_THRESHOLD = 1000;
const bool HIGH_VALUE_IS_LINE = true;

const int BASE_SPEED = 60;
const int MAX_SPEED = 120;

int lineWeight[8] = {-70, -50, -30, -10, 10, 30, 50, 70};
bool lineDetected = false;
unsigned long lastControlTime = 0;

// 保存巡线PID参数、历史误差和输出
PID linePid;

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_L_PWM_PIN, OUTPUT);
  pinMode(MOTOR_L_DIR_PIN, OUTPUT);
  pinMode(MOTOR_R_PWM_PIN, OUTPUT);
  pinMode(MOTOR_R_DIR_PIN, OUTPUT);
  stopMotors();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // 目标误差为 0，表示黑线位于小车中央
  // 巡线修正量允许正负输出，范围为-60～60
  PID_Init(&linePid, 0.8, 0.0, 0.15, 60, -60);
  PID_Clear(&linePid);
  lastControlTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 50) {
    float lineError = readLineError();
    if (lineDetected == false) {
      stopMotors();
      PID_Clear(&linePid);
      Serial.println("LINE LOST");
    } else {
      // 当前值是巡线误差，目标值是0
      float correction = PID_IncPIDCal(&linePid, lineError, 0);
      int leftSpeed = BASE_SPEED - correction;
      int rightSpeed = BASE_SPEED + correction;

      leftSpeed = constrain(leftSpeed, 0, MAX_SPEED);
      rightSpeed = constrain(rightSpeed, 0, MAX_SPEED);
      setMotors(leftSpeed, rightSpeed);

      Serial.print("Error:");
      Serial.print(lineError);
      Serial.print("  Left:");
      Serial.print(leftSpeed);
      Serial.print("  Right:");
      Serial.println(rightSpeed);
    }

    lastControlTime = currentTime;
  }
}

float readLineError() {
  Wire.beginTransmission(LINE_SENSOR_ADDRESS);
  Wire.write(LINE_READ_COMMAND);

  if (Wire.endTransmission() != 0) {
    lineDetected = false;
    return 0;
  }

  delay(10);
  int receivedBytes = Wire.requestFrom(LINE_SENSOR_ADDRESS, LINE_DATA_LENGTH);

  if (receivedBytes != LINE_DATA_LENGTH) {
    while (Wire.available() > 0) {
      Wire.read();
    }

    lineDetected = false;
    return 0;
  }

  // 前16字节为参考值，本实验使用统一阈值，直接跳过
  for (int i = 0; i < 16; i = i + 1) {
    Wire.read();
  }

  long weightSum = 0;
  int lineCount = 0;

  for (int i = 0; i < 8; i = i + 1) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    int sensorValue = highByte * 256 + lowByte;
    bool isLine;

    if (HIGH_VALUE_IS_LINE) {
      isLine = sensorValue >= LINE_THRESHOLD;
    } else {
      isLine = sensorValue < LINE_THRESHOLD;
    }

    if (isLine) {
      weightSum = weightSum + lineWeight[i];
      lineCount = lineCount + 1;
    }
  }

  if (lineCount == 0) {
    lineDetected = false;
    return 0;
  }

  lineDetected = true;
  return (float)weightSum / lineCount;
}

void setMotors(int leftSpeed, int rightSpeed) {
  // 左右电机安装方向相反，因此方向电平不同
  digitalWrite(MOTOR_L_DIR_PIN, HIGH);
  digitalWrite(MOTOR_R_DIR_PIN, LOW);
  analogWrite(MOTOR_L_PWM_PIN, leftSpeed);
  analogWrite(MOTOR_R_PWM_PIN, rightSpeed);
}

void stopMotors() {
  analogWrite(MOTOR_L_PWM_PIN, 0);
  analogWrite(MOTOR_R_PWM_PIN, 0);
}
```

### 4. 程序主流程及执行过程
- 程序整体分几步执行：初始化电机和 I2C；每 `50ms` 读取八路数据；计算黑线位置；丢线就停止；找到线就计算 PID 修正量；输出左右轮速度。
- `setup()` 的作用：先停住电机，再准备 I2C 和巡线 PID。
- `loop()` 的作用：不断重复“看线—算偏差—改轮速”。
- 数据输入、处理、输出的执行过程：八路传感器值 → 阈值判断 → 权重平均误差 → PID 修正量 → 左右轮差速 → 小车位置改变。

### 5. 核心函数及代码讲解
- `Wire.begin(8, 9)`：在 `GPIO 8/9` 上启动 I2C。
- `Wire.requestFrom(0x27, 32)`：向地址 `0x27` 的模块请求 `32` 字节。
- `sensorValue = highByte * 256 + lowByte`：把高、低两个字节拼成一个传感器数值。
- `weightSum / lineCount`：若多只眼睛同时看到线，就求它们位置权重的平均值。
- `leftSpeed = BASE_SPEED - correction`、`rightSpeed = BASE_SPEED + correction`：一边减、一边加，形成转向。
- `constrain(..., 0, MAX_SPEED)`：把左右轮速度限制在安全范围。
- 为什么这样写：把读传感器、控制电机、停止电机分成独立函数，出现问题时可以一块一块检查。

---

## 总结
- 实验目标：看懂 PID 闭环，完成软件模拟、电机调速、电机位置控制和八路巡线。
- 单词：`target` 目标、`input` 当前值、`error` 误差、`output` 输出、`feedback` 反馈、`speed` 速度、`position` 位置、`correction` 修正。
- 接线：调速/位置使用 DIR `2`、PWM `1`、编码器 A `6`、B `7`；巡线使用左轮 `42/41`、右轮 `1/2`、I2C `8/9`；电机独立供电并共地。
- 关键函数和参数：`PID_Init()`、`PID_Clear()`、`PID_IncPIDCal()`、`attachInterrupt()`、`analogWrite()`、`Wire.requestFrom()`。
- 关键程序：固定周期控制、编码器反馈、正负位置输出、八路权重误差、左右轮差速。
- 本课重点：
  - 闭环是“行动后看结果，再继续修正”。
  - 误差等于目标值减当前值。
  - P 先管当前差多少，D 帮助减小冲过头，I 最后处理长期小偏差。
  - 调参推荐顺序：先只调 P，再加少量 D，最后需要时才加少量 I。
  - 每次只改一点，记录现象；失控、堵转或发热时立即断电。
