/*
  程序功能：控制有源蜂鸣器周期性鸣叫，观察鸣叫次数与开关频率的关系。
  使用方法：有源蜂鸣器接 GPIO 21，上传后打开波特率为 115200 的串口监视器。
  运行结果：蜂鸣器依次以 1 Hz、2 Hz、5 Hz 鸣叫，串口显示当前频率。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// 有源蜂鸣器模块的控制端连接 GPIO 21
#define PIN_BUZZER 21

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  Serial.println("1 Hz：每秒鸣叫 1 次");
  beepAtFrequency(1, 3);
  delay(1000);

  Serial.println("2 Hz：每秒鸣叫 2 次");
  beepAtFrequency(2, 3);
  delay(1000);

  Serial.println("5 Hz：每秒鸣叫 5 次");
  beepAtFrequency(5, 3);
  delay(2000);
}

// frequency 表示每秒“响一次再停一次”的次数
// seconds 表示这一组鸣叫持续多少秒
void beepAtFrequency(int frequency, int seconds) {
  int halfPeriod = 500 / frequency;
  int beepCount = frequency * seconds;

  for (int i = 0; i < beepCount; i = i + 1) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(halfPeriod);

    digitalWrite(PIN_BUZZER, LOW);
    delay(halfPeriod);
  }
}
