/*
  程序功能：使用 GPIO 12 和 GPIO 13 进行超声波测距，并在串口显示距离。
  使用方法：TRIG 接 GPIO 12、ECHO 经分压后接 GPIO 13，打开 115200 串口并移动障碍物。
  运行结果：串口持续显示障碍物距离，单位为 cm；无有效回波时显示测量失败。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

// HC-SR04 超声波模块引脚
#define PIN_TRIG 12
#define PIN_ECHO 13

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // TRIG 默认保持低电平
  digitalWrite(PIN_TRIG, LOW);
}

void loop() {
  float distance = measureDistance();

  if (distance == 0) {
    Serial.println("未收到回波");
  } else {
    Serial.print("距离：");
    Serial.print(distance, 1);
    Serial.println(" cm");
  }

  // 两次测量之间留出时间，避免上一束回波影响下一次测量
  delay(200);
}

float measureDistance() {
  // 先产生一个至少 10 微秒的触发脉冲
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 测量 ECHO 高电平持续时间，30 ms 后仍无回波就退出
  unsigned long echoTime = pulseIn(PIN_ECHO, HIGH, 30000);

  // 声速约为 0.0343 cm/us；声音走了一个来回，所以除以 2
  float distance = echoTime * 0.0343 / 2.0;
  return distance;
}
