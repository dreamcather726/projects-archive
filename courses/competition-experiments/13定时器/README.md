# ESP32-S3 定时方案课程

本课程共 4 课时，通过两个 LED 对比阻塞式 `delay()`、非阻塞 `millis()` 和 ESP32
Arduino 核心自带的 `Ticker`。

## 接线

| 元件 | ESP32-S3 |
|---|---:|
| LED1 | GPIO 4，串联限流电阻 |
| LED2 | GPIO 5，串联限流电阻 |
| LED 负极 | GND |

## 三个实验

### 1. delay() 阻塞式延时

文件：`01_delay_blocking/01_delay_blocking.ino`

程序按顺序执行：

1. LED1 点亮/熄灭各 0.5 秒。
2. LED2 点亮/熄灭各 1 秒。
3. LED1+LED2 点亮/熄灭各 1.8 秒。

执行 `delay()` 时，CPU 不能继续运行后面的按键、串口或传感器任务，适合最初级的
单一节拍演示，不适合多个任务同时运行。

### 2. millis() 非阻塞计时

文件：`02_millis_non_blocking/02_millis_non_blocking.ino`

核心判断：

```cpp
unsigned long currentTime = millis();

if (currentTime - lastTime >= interval) {
  lastTime = currentTime;
  // 执行一次任务
}
```

必须使用“当前时间减去上次时间”，不要写成 `millis() >= lastTime + interval`。减法写法
能够正确处理 `millis()` 数值回绕。

### 3. Ticker 定时器

文件：`03_ticker_timer/03_ticker_timer.ino`

```cpp
Ticker taskTicker;
taskTicker.attach_ms(100, runTimerTasks);
```

- `attach()` 的单位是秒。
- `attach_ms()` 的单位是毫秒。
- `detach()` 可以停止定时调用。
- 本实验只使用一个 Ticker，每100ms执行一次。
- 三个计数变量分别累计5次、10次和18次，得到0.5s、1s和1.8s任务。
- 回调函数必须短，不放 `delay()`、长循环、屏幕整刷或阻塞式串口读取。

## 三种方案对比

| 方案 | 优点 | 缺点 | 适合场景 |
|---|---|---|---|
| `delay()` | 最容易理解 | 阻塞其他任务 | 单一演示、短暂等待 |
| `millis()` | 灵活、直观、非阻塞 | 需要保存时间变量 | 主循环多任务 |
| `Ticker` | 周期任务写法简洁 | 回调必须很短 | 简单周期触发 |

## 4 课时安排

### 第 1 课时：阻塞式延时

- `delay()` 的执行过程。
- 顺序执行和阻塞现象。
- 三种 LED 节拍实验。

### 第 2 课时：millis() 代码讲解

- `unsigned long` 时间变量。
- 时间差判断。
- 三个任务各自保存上次执行时间。
- 状态变量和逻辑取反。

### 第 3 课时：Ticker+原理

- Ticker 对象、回调函数、`attach_ms()`、`detach()`。
- 为什么回调函数不能执行耗时任务。
- 软件定时、硬件定时和调度的基础区别。

### 第 4 课时：综合实验

1. 修改三个时间间隔。
2. 在 `millis()` 程序中加入串口字符读取。
3. 在 `delay()` 程序中加入按键，观察响应延迟。
4. 对比 millis 和 Ticker 的代码结构。
5. 使用 `detach()` 让 Ticker 在运行 10 秒后停止。

## 常见问题

- **LED 节拍不准确：** 检查单位是毫秒还是秒。
- **millis 任务只执行一次：** 检查是否更新了 `lastTime`。
- **Ticker 编译找不到：** 确认选择 ESP32 开发板；Ticker 随 ESP32 Arduino 核心提供。
- **Ticker 回调导致异常：** 删除回调中的 `delay()`、长循环、屏幕整刷和阻塞操作。
- **两个 LED 状态看起来被打乱：** 1.8 秒组合任务也会反转两灯，这是本实验故意叠加的第三节拍。

产品声明：本课程及示例程序为青潮智科产品配套教学内容。
