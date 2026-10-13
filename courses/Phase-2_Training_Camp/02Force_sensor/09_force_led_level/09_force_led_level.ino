/*
  程序功能：把力敏传感器的压力值转换成 LED 亮度，实现压力越大灯越亮。
  使用方法：力敏输出接 GPIO 1、LED 接 GPIO 2，打开 115200 串口后改变按压力度。
  运行结果：LED 随压力增大而变亮，串口同时显示 force 和 light 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 力敏传感器连接 GPIO 1
#define PIN_FORCE 1

// 板载 LED_ON 连接 GPIO 2
#define PIN_LED 2

// 达到这个压力数值时，LED 为最亮
#define FORCE_MAX 3000

void setup() {
  Serial.begin(115200);


  // 把 LED 引脚设置为输出模式
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  int force = analogRead(PIN_FORCE);

  // 防止压力数值超过设定范围
  force = constrain(force, 0, FORCE_MAX);

  // 把压力转换为 0～255 的亮度
  int light = map(force, 0, FORCE_MAX, 0, 255);

  // 输出 PWM，控制 LED 亮度
  analogWrite(PIN_LED, light);

  Serial.print("force: ");
  Serial.print(force);
  Serial.print("  light: ");
  Serial.println(light);

  delay(50);
}
