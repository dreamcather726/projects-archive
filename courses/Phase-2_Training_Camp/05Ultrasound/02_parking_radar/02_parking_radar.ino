/*
  程序功能：使用超声波模块测量障碍物距离，距离越近有源蜂鸣器报警越急促。
  使用方法：TRIG 接 GPIO 12、ECHO 经分压接 GPIO 13、蜂鸣器接 GPIO 21，然后移动障碍物。
  运行结果：串口显示距离；障碍物靠近时蜂鸣器鸣叫间隔缩短，进入危险距离后持续报警。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// HC-SR04 超声波模块引脚
#define PIN_TRIG 12
#define PIN_ECHO 13

// 有源蜂鸣器模块的控制端连接 GPIO 21
#define PIN_BUZZER 21

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  float distance = measureDistance();

  if (distance == 0) {
    Serial.println("未收到回波，蜂鸣器关闭");
    digitalWrite(PIN_BUZZER, LOW);
    delay(200);
  } else {
    Serial.print("距离：");
    Serial.print(distance, 1);
    Serial.print(" cm，");

    if (distance <= 20) {
      Serial.println("危险，连续报警");
      digitalWrite(PIN_BUZZER, HIGH);
      delay(100);
    } else if (distance <= 50) {
      Serial.println("较近，快速报警");
      beepOnce(100, 150);
    } else if (distance <= 100) {
      Serial.println("注意，慢速报警");
      beepOnce(100, 600);
    } else {
      Serial.println("安全，蜂鸣器关闭");
      digitalWrite(PIN_BUZZER, LOW);
      delay(200);
    }
  }
}

float measureDistance() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);

  float distance = echoTime * 0.0343 / 2.0;
  return distance;
}

void beepOnce(int soundTime, int quietTime) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(soundTime);

  digitalWrite(PIN_BUZZER, LOW);
  delay(quietTime);
}
