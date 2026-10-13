/*
  程序功能：只使用一个 ESP32 Ticker，实现 0.5s、1s 和 1.8s 三个闪烁任务。
  使用方法：LED1、LED2 接 GPIO 4、5；Ticker 每100ms运行一次，通过计数控制不同任务。
  运行结果：一个 Ticker 分别累计5次、10次和18次，触发三个不同周期的闪烁任务。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Ticker.h>

const int LED1_PIN = 4;
const int LED2_PIN = 5;

// 整个程序只创建一个 Ticker 定时器
Ticker taskTicker;

bool led1State = false;
bool led2State = false;

int led1Count = 0;
int led2Count = 0;
int bothCount = 0;

// 这个函数每100ms执行一次
void runTimerTasks() {
  led1Count = led1Count + 1;
  led2Count = led2Count + 1;
  bothCount = bothCount + 1;

  // 100ms × 5 = 500ms
  if (led1Count >= 5) {
    led1Count = 0;
    led1State = !led1State;
    digitalWrite(LED1_PIN, led1State);
  }

  // 100ms × 10 = 1000ms
  if (led2Count >= 10) {
    led2Count = 0;
    led2State = !led2State;
    digitalWrite(LED2_PIN, led2State);
  }

  // 100ms × 18 = 1800ms
  if (bothCount >= 18) {
    bothCount = 0;
    led1State = !led1State;
    led2State = !led2State;
    digitalWrite(LED1_PIN, led1State);
    digitalWrite(LED2_PIN, led2State);
  }
}

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // 只启动一个定时器，每100ms调用一次 runTimerTasks
  taskTicker.attach_ms(100, runTimerTasks);
}

void loop() {
  // Ticker 会定时调用函数，主循环可以继续执行其他任务
}
