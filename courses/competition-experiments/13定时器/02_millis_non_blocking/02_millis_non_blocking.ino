/*
  程序功能：使用 millis() 同时执行 LED1 0.5s、LED2 1s、双灯 1.8s 三个非阻塞任务。
  使用方法：LED1、LED2 接 GPIO 4、5，上传后观察；也可打开 115200 串口查看任务触发时间。
  运行结果：三个定时判断互不等待，串口分别显示 LED1、LED2、BOTH 任务触发信息。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int LED1_PIN = 4;
const int LED2_PIN = 5;

unsigned long led1LastTime = 0;
unsigned long led2LastTime = 0;
unsigned long bothLastTime = 0;

bool led1State = false;
bool led2State = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - led1LastTime >= 500) {
    led1LastTime = currentTime;
    led1State = !led1State;
    digitalWrite(LED1_PIN, led1State);
    Serial.println("LED1 500ms task");
  }

  if (currentTime - led2LastTime >= 1000) {
    led2LastTime = currentTime;
    led2State = !led2State;
    digitalWrite(LED2_PIN, led2State);
    Serial.println("LED2 1000ms task");
  }

  // 每 1.8 秒让两个 LED 同时反转一次
  if (currentTime - bothLastTime >= 1800) {
    bothLastTime = currentTime;
    led1State = !led1State;
    led2State = !led2State;
    digitalWrite(LED1_PIN, led1State);
    digitalWrite(LED2_PIN, led2State);
    Serial.println("BOTH 1800ms task");
  }

  // 这里可以继续读取按键、串口或传感器，不会被 delay() 卡住
}
