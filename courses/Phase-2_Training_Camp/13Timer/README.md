# ESP32-S3 定时方案课程

本课程使用电机和编码器演示阻塞与非阻塞定时，不使用灯。两个核心任务均以 `1000ms`
为周期：电机在 PWM `80` 和 `0` 之间切换，编码器累计计数通过串口输出。

## 接线与参数

| 功能 | 参数 |
|---|---:|
| 电机方向 | GPIO 1 |
| 电机 PWM | GPIO 41 |
| 编码器 A 相 | GPIO 4 |
| 编码器 B 相 | GPIO 5 |
| PWM 频率 | 10000Hz |
| PWM 分辨率 | 8 位 |
| PWM 通道 | 0 |
| 运行速度 | 80 |
| 停止速度 | 0 |
| 两个任务周期 | 1000ms |
| 串口波特率 | 115200 |

电机必须经过驱动板并使用合适的电机电源。ESP32、驱动板、编码器和电机电源必须共地。
第一次测试时抬起车轮；发现堵转、异响、发热或开发板复位应立即断电。

> `ledc...` 是 ESP32 PWM 外设的 API 名称，本章使用它输出电机 PWM，并不控制灯。

## 两个定时任务

### 电机控制任务

每隔 `1000ms` 执行一次：

```cpp
motorRunning = !motorRunning;

if (motorRunning == false) {
  writeMotorPwm(LOW_SPEED);
} else {
  writeMotorPwm(HIGH_SPEED);
}
```

### 编码器读取任务

编码器 A 相上升沿由外部中断累计。每隔 `1000ms`，主循环在短临界区内复制计数，再到
临界区外输出：

```cpp
noInterrupts();
long countCopy = encoderCount;
interrupts();

Serial.println(countCopy);
```

本实验只累计 A 相上升沿，不使用 B 相判断方向。

## 三个实验

### 1. delay() 阻塞式执行

文件：`01_delay_blocking/01_delay_blocking.ino`

电机运行、等待、读取编码器、电机停止、等待、再次读取编码器均按顺序执行。等待期间，
主循环不能处理后面的任务。

### 2. millis() 非阻塞双任务

文件：`02_millis_non_blocking/02_millis_non_blocking.ino`

```cpp
unsigned long currentTime = millis();

if (currentTime - lastMotorTime >= 1000) {
  lastMotorTime = currentTime;
  // 执行电机任务
}

if (currentTime - lastEncoderTime >= 1000) {
  lastEncoderTime = currentTime;
  // 执行编码器任务
}
```

两个任务使用各自的上次执行时间，互不阻塞。时间变量使用 `unsigned long`，时间判断采用
差值写法，可正确处理 `millis()` 回绕。

### 3. Ticker 定时触发

文件：`03_ticker_timer/03_ticker_timer.ino`

Ticker 每 `1000ms` 调用一次 `onTimerTick()`。回调只设置两个任务标志，主循环负责更新
电机 PWM 和打印串口，避免在回调中执行较慢操作。

## 三种方案对比

| 方案 | 特点 | 适用场景 |
|---|---|---|
| `delay()` | 简单，但会阻塞后续任务 | 单一、短时间等待 |
| `millis()` | 主循环按时间差检查，非阻塞 | 多个周期任务 |
| `Ticker` | 自动产生周期回调 | 固定节拍和任务通知 |

## 常见问题

- 电机不转：检查 GPIO 1/41、驱动电源、共地和 PWM 配置。
- 编码器无计数：检查 GPIO 4/5、编码器供电及 3.3V 电平兼容性。
- 计数一直增加但不能判断方向：本实验只统计 A 相上升沿，属于预期现象。
- `millis()` 分支持续触发：检查触发后是否更新对应的 `last...Time`。
- Ticker 不稳定：回调只保留置标志，不加入 `delay()` 或串口输出。

产品声明：本课程及示例程序为青潮智科产品配套教学内容。
