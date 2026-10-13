# 第二模块实验——LEDC 电机驱动

本模块只完成三个简单实验：

1. 电机自动正转和反转。
2. 串口字符控制电机。
3. 电机与编码器距离计算。

统一参数：

```cpp
const int MOTOR_DIR_PIN = 1;
const int MOTOR_PWM_PIN = 42;
const int MOTOR_PWM_FREQUENCY = 20000;
const int MOTOR_PWM_RESOLUTION = 8;
const int MOTOR_PWM_CHANNEL = 0;
const int MOTOR_SPEED = 100;
```

- PWM 频率：20kHz。
- PWM 分辨率：8 位。
- LEDC 通道：0。
- Duty 范围：0～255。

## 实验1：电机正转、停止和反转

代码：`01_motor_basic/01_motor_basic.ino`

### 接线

| 功能 | ESP32-S3 |
|---|---:|
| 电机 A：PWMA | GPIO 42 |
| 电机 A：AN1 | GPIO 1 |
| GND | 与驱动板共地 |

### 核心代码

配置 LEDC：

```cpp
ledcAttachChannel(
  MOTOR_PWM_PIN,
  MOTOR_PWM_FREQUENCY,
  MOTOR_PWM_RESOLUTION,
  MOTOR_PWM_CHANNEL
);
```

控制电机：

```cpp
// 正转
digitalWrite(MOTOR_DIR_PIN, LOW);
ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
delay(3000);

// 停止
ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
delay(1000);

// 反转
digitalWrite(MOTOR_DIR_PIN, HIGH);
ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
```

### 实验现象

电机正转3秒、停止1秒、反转3秒、停止1秒，然后重复。

### 验收

- 电机能正转和反转。
- 换向前电机先停止。
- 上电时 PWM 初始值为0。

## 实验2：串口字符控制

代码：`02_motor_serial/02_motor_serial.ino`

### 串口命令

| 命令 | 功能 |
|:---:|---|
| `F` | 正转 |
| `B` | 反转 |
| `S` | 停止 |

### 核心代码

```cpp
if (Serial.available() > 0) {
  char command = Serial.read();

  if (command == 'F') {
    runMotor(LOW);
  } else if (command == 'B') {
    runMotor(HIGH);
  } else if (command == 'S') {
    ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  }
}
```

运行电机：

```cpp
void runMotor(int directionValue) {
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  delay(50);

  digitalWrite(MOTOR_DIR_PIN, directionValue);
  ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
}
```

### 实验现象

- 发送 `F`，电机正转。
- 发送 `B`，电机反转。
- 发送 `S`，电机停止。

### 验收

- 三个字符命令均有效。
- 换向前先停止50ms。
- 电机速度固定为100。

## 实验3：编码器距离计算

代码：`04_motor_encoder_distance_serial/04_motor_encoder_distance_serial.ino`

### 接线

| 功能 | ESP32-S3 |
|---|---:|
| 电机 A：PWMA | GPIO 42 |
| 电机 A：AN1 | GPIO 1 |
| 电机 A：编码器 A+ | GPIO 4 |
| 电机 A：编码器 A- | GPIO 5 |
| GND | 所有模块共地 |

### 编码器计数

```cpp
void IRAM_ATTR encoderRising() {
  int levelB = digitalRead(ENCODER_B_PIN);

  if (levelB == HIGH) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}
```

A 相使用 `RISING`，每圈按430次计算。

### 距离计算

```cpp
const float WHEEL_CIRCUMFERENCE_CM =
  PI * WHEEL_DIAMETER_MM / 10.0;

const float DISTANCE_PER_COUNT_CM =
  WHEEL_CIRCUMFERENCE_CM / COUNTS_PER_REVOLUTION;

float distanceCm =
  currentCount * DISTANCE_PER_COUNT_CM;
```

60mm轮子转动一圈的理论距离约为18.85cm。

### 串口命令

| 命令 | 功能 |
|:---:|---|
| `F` | 正转 |
| `B` | 反转 |
| `S` | 停止 |
| `R` | 编码器计数清零 |

### 验收

- 电机能正转、反转和停止。
- 编码器计数随转动变化。
- 发送 `R` 后计数回到0。
- 串口能够显示带两位小数的厘米距离。

## 安全要求

1. 接线和改线前断电。
2. 电机必须连接驱动板，不能直接连接 ESP32 GPIO。
3. ESP32、驱动板、电机电源和编码器必须共地。
4. 第一次测试时抬起车轮。
5. 电机堵转、驱动板过热或主控复位时立即断电。

## 常见问题

| 现象 | 检查内容 |
|---|---|
| 电机不转 | 电源、共地、DIR/PWM 接线、LEDC 配置 |
| 只能一个方向 | DIR 接线和方向电平 |
| 换向冲击大 | 换向前是否先写入 Duty 0 |
| 编码器没有计数 | A/B 是否接 GPIO 4/5 |
| 距离没有小数 | 距离变量是否使用 `float` |
| 正转计数为负 | 编码器方向约定相反，可对调加减逻辑 |
