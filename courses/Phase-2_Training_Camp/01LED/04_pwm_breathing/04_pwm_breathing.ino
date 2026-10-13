/*
  程序功能：使用 PWM 逐渐改变 LED 亮度，实现呼吸灯效果。
  使用方法：把 LED 通过限流电阻连接 GPIO 4，上传程序后直接观察。
  运行结果：LED 从暗逐渐变亮，再从亮逐渐变暗，并不断循环。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// LED 正极通过电阻连接 GPIO 4
const int LED_PIN = 4;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // PWM 数值从 0 增加到 255，LED 逐渐变亮
  // 每次增加 5，数字越小，亮度变化越平滑
  for (int light = 0; light <= 255; light = light + 5) {
    // 0 表示关闭，255 表示最亮
    analogWrite(LED_PIN, light);

    // 延时越长，呼吸变化越慢
    delay(30);
  }

  // PWM 数值从 255 减少到 0，LED 逐渐变暗
  for (int light = 255; light >= 0; light = light - 5) {
    analogWrite(LED_PIN, light);
    delay(30);
  }
}
