/*
  程序功能：读取力敏传感器数值，压力超过设定值时启动有源蜂鸣器报警。
  使用方法：力敏输出接 GPIO 15、蜂鸣器接 GPIO 21，打开 115200 串口后按压力敏片。
  运行结果：串口显示压力值；数值超过 1200 时蜂鸣器鸣叫，松开后停止。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 力敏传感器的分压输出连接 GPIO 15
#define PIN_FORCE 15

// 有源蜂鸣器模块的控制端连接 GPIO 21
#define PIN_BUZZER 21

// 压力超过这个数值时报警
// 可以根据串口显示的数值进行修改
#define FORCE_LIMIT 1200

void setup() {
  Serial.begin(115200);
  // 蜂鸣器引脚设置为输出模式
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  // 读取力敏传感器，范围通常是 0～4095
  int force = analogRead(PIN_FORCE);

  // 在串口监视器中观察压力数值
  Serial.println(force);

  // 压力超过设定值，蜂鸣器响
  if (force > FORCE_LIMIT) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  // 每隔 100 毫秒读取一次
  delay(100);
}
