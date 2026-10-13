/*
  程序功能：控制普通 LED 周期性点亮和熄灭，实现闪烁效果。
  使用方法：把 LED 通过限流电阻连接 GPIO 4，上传程序后直接观察 LED。
  运行结果：LED 点亮 1 秒、熄灭 1 秒，并不断重复。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// LED 正极通过电阻连接 GPIO 4
   int LED_PIN = 4;

// setup 只运行一次，用来完成初始化
void setup() {
  // 把 LED 引脚设置为输出模式
  pinMode(LED_PIN, OUTPUT);
}

// loop 中的代码会不断重复
void loop() {
  // HIGH 表示输出高电平，LED 点亮
  digitalWrite(LED_PIN, HIGH);

  // 保持点亮 1000 毫秒，也就是 1 秒
  delay(1000);

  // LOW 表示输出低电平，LED 熄灭
  digitalWrite(LED_PIN, LOW);

  // 保持熄灭 1 秒
  delay(1000);
}
