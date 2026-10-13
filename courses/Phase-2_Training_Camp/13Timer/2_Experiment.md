# 第二模块实验——电机与编码器定时任务

## 实验1：delay() 阻塞式执行

代码：`01_delay_blocking/01_delay_blocking.ino`

程序先让电机以 PWM `80` 运行并等待 1 秒，再读取编码器；随后让电机停止并等待 1 秒，
再读取一次编码器。

```cpp
writeMotorPwm(HIGH_SPEED);
delay(1000);
printEncoderCount();

writeMotorPwm(LOW_SPEED);
delay(1000);
printEncoderCount();
```

检查点：能说明两个 `delay(1000)` 期间后续代码不能执行。

## 实验2：millis() 非阻塞双任务

代码：`02_millis_non_blocking/02_millis_non_blocking.ino`

### 电机任务

```cpp
if (currentTime - lastMotorTime >= 1000) {
  lastMotorTime = currentTime;
  motorRunning = !motorRunning;

  if (motorRunning == false) {
    writeMotorPwm(LOW_SPEED);
  } else {
    writeMotorPwm(HIGH_SPEED);
  }
}
```

### 编码器任务

```cpp
if (currentTime - lastEncoderTime >= 1000) {
  lastEncoderTime = currentTime;

  noInterrupts();
  long countCopy = encoderCount;
  interrupts();

  Serial.print("Encoder count: ");
  Serial.println(countCopy);
}
```

两个任务周期虽然相同，仍使用两个独立时间变量，便于以后把周期改为不同数值。

### 操作步骤

1. 断电完成接线并抬起车轮。
2. 上传程序，打开 115200 串口监视器。
3. 观察电机运行 1 秒、停止 1 秒并不断重复。
4. 检查串口每约 1 秒输出一次累计计数。
5. 确认电机运行时计数增加，停止后计数基本不变。

## 实验3：Ticker 双任务通知

代码：`03_ticker_timer/03_ticker_timer.ino`

```cpp
void onTimerTick() {
  motorTaskReady = true;
  encoderTaskReady = true;
}
```

```cpp
taskTicker.attach_ms(1000, onTimerTick);
```

主循环分别消费两个标志：

```cpp
if (motorTaskReady) {
  motorTaskReady = false;
  runMotorTask();
}

if (encoderTaskReady) {
  encoderTaskReady = false;
  runEncoderTask();
}
```

不要在 `onTimerTick()` 中直接使用串口打印或 `delay()`。

## 实验验收

- 电机方向引脚 GPIO 1、PWM 引脚 GPIO 41 接线正确。
- 编码器 A/B 接 GPIO 4/5，计数能够增加。
- 电机每 1 秒切换启停状态。
- 串口每 1 秒输出一次编码器计数。
- `millis()` 程序中没有 `delay()`。
- Ticker 回调只设置任务标志。

## 故障排查

| 现象 | 可能原因 | 处理 |
|---|---|---|
| 电机不转 | PWM 引脚或驱动电源错误 | 检查 GPIO 41、供电和共地 |
| 编码器不计数 | A 相接线或中断配置错误 | 检查 GPIO 4 和 `RISING` |
| 电机一直高速 | 未更新状态或未写入低速 | 检查 `motorRunning` 与 `LOW_SPEED` |
| 串口连续刷屏 | 未更新 `lastEncoderTime` | 触发后记录当前时间 |
| Ticker 异常 | 回调中加入耗时操作 | 回调只置标志 |

产品声明：本课程及示例程序为青潮智科产品配套教学内容。
