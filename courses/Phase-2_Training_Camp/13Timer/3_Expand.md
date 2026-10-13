# 第三模块 拓展

## 1. 知识拓展

- 本实验只统计编码器 A 相上升沿，所以能够取得脉冲总数，但不能判断转动方向。
- 若要判断方向，可在 ISR 中读取 B 相电平；这属于编码器课程内容，不是本章必做要求。
- 两个任务当前都是 `1000ms`，仍然使用独立时间变量，方便以后把电机任务和采样任务改成
  不同周期。
- `currentTime - lastTime >= interval` 比比较两个绝对时刻更可靠，可正确处理长时间运行后的
  `millis()` 回绕。
- 共享计数复制完成后应立即恢复中断，不能把串口打印放进临界区。
- Ticker 回调只置标志，实际 PWM 更新和串口输出在主循环中完成。
- `ledc` 是 ESP32 PWM 外设的历史名称。本章通过它驱动电机，不涉及灯控。

## 2. 速查表

| 名称 | 作用或参数 |
|---|---|
| `MOTOR_DIR_PIN` | GPIO 1 |
| `MOTOR_PWM_PIN` | GPIO 41 |
| `ENCODER_A_PIN` | GPIO 4 |
| `ENCODER_B_PIN` | GPIO 5 |
| `PWM_FREQUENCY` | 10000Hz |
| `PWM_RESOLUTION` | 8 位 |
| `HIGH_SPEED` | 80 |
| `LOW_SPEED` | 0 |
| `lastMotorTime` | 上次执行电机任务的时刻 |
| `lastEncoderTime` | 上次读取编码器的时刻 |
| `motorRunning` | 电机当前启停状态 |
| `encoderCount` | 编码器累计上升沿计数 |
| `attach_ms()` | 按毫秒周期绑定 Ticker 回调 |
| `detach()` | 停止 Ticker 回调 |
