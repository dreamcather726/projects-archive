# 第一模块 引言

## 1. 学习目标

- 使用电机控制和编码器读取作为两个真实定时任务。
- 认识 `delay()` 的阻塞问题。
- 使用 `millis()` 同时管理两个 `1000ms` 非阻塞任务。
- 使用 Ticker 回调通知主循环执行任务。
- 正确保护由中断更新的编码器共享计数。

## 2. 实验现象

- 电机每 1 秒在 PWM `80` 与 `0` 之间切换。
- 串口每 1 秒输出一次编码器累计计数。
- `delay()` 版本只能顺序运行任务。
- `millis()` 和 Ticker 版本不需要在主循环中阻塞等待。

## 3. 已学与新增知识

### 已学知识

- 电机驱动板的方向引脚和 PWM 调速。
- 编码器外部中断与 `RISING` 上升沿。
- `volatile`、`noInterrupts()` 和 `interrupts()`。

### 新增知识

- `millis()` 返回开机后的毫秒数，时间变量应使用 `unsigned long`。
- 两个任务要分别保存 `lastMotorTime` 和 `lastEncoderTime`。
- 两个独立任务应使用两个 `if` 判断，不能用一个共享的上次时间。
- Ticker 回调应保持简短，只负责设置 `motorTaskReady` 和 `encoderTaskReady`。

## 4. 重点单词

- `timer`：定时器
- `motor`：电机
- `encoder`：编码器
- `count`：计数
- `pulse`：脉冲
- `interrupt`：中断
- `current`：当前的
- `last`：上一次的
- `running`：正在运行
- `blocking`：阻塞的
- `non-blocking`：非阻塞的
- `callback`：回调
- `ready`：已就绪

## 5. 器材与安全

- ESP32-S3 开发板、带编码器的电机、电机驱动板和合适电源。
- 电机 DIR/PWM 接 GPIO 1/41，编码器 A/B 接 GPIO 4/5。
- 电机不能直接连接 ESP32 GPIO，所有设备必须共地。
- 编码器输出应兼容 3.3V 输入电平。
- 初次上传程序时必须抬起车轮。
