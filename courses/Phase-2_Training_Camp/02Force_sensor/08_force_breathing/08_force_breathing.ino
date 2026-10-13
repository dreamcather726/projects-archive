/*
  程序功能：检测压力大小，超过设定值后启动 LED 呼吸灯效果。
  使用方法：力敏输出接 GPIO 15、LED 接 GPIO 2，上传程序后按压力敏片。
  运行结果：压力值超过 1200 时 LED 产生呼吸效果，压力不足时 LED 熄灭。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 力敏传感器连接 GPIO 15
#define PIN_FORCE 15

// 板载 LED_ON 连接 GPIO 2
#define PIN_LED 2

// 超过这个数值时启动呼吸灯
#define FORCE_LIMIT 1200

void setup() {

  // 把 LED 引脚设置为输出模式
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  int force = analogRead(PIN_FORCE);

  if (force > FORCE_LIMIT) {
    // PWM 从 0 增加到 255，LED 逐渐变亮
    for (int light = 0; light <= 255; light = light + 5) {
      analogWrite(PIN_LED, light);
      delay(20);
    }

    // PWM 从 255 减少到 0，LED 逐渐变暗
    for (int light = 255; light >= 0; light = light - 5) {
      analogWrite(PIN_LED, light);
      delay(20);
    }
  } else {
    // 没有达到设定压力时关闭 LED
    analogWrite(PIN_LED, 0);
  }
}
