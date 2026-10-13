/*
  程序功能：使用 delay() 阻塞式延时演示 LED1 0.5s、LED2 1s、双灯 1.8s 闪烁。
  使用方法：LED1、LED2 分别接 GPIO 4、5，上传后直接观察三个顺序执行的闪烁阶段。
  运行结果：LED1 闪烁一次，再由 LED2 闪烁一次，最后两个 LED 同时闪烁一次。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int LED1_PIN = 4;
const int LED2_PIN = 5;

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
}

void loop() {
  // 阶段一：LED1 每个状态保持 0.5 秒
  digitalWrite(LED1_PIN, HIGH);
  delay(500);
  digitalWrite(LED1_PIN, LOW);
  delay(500);

  // 阶段二：LED2 每个状态保持 1 秒
  digitalWrite(LED2_PIN, HIGH);
  delay(1000);
  digitalWrite(LED2_PIN, LOW);
  delay(1000);

  // 阶段三：两个 LED 同时闪烁，每个状态保持 1.8 秒
  digitalWrite(LED1_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);
  delay(1800);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  delay(1800);
}
