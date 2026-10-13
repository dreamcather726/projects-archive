/*
  程序功能：让舵机自动往返扫描，并通过串口输出时间、命令角度和 PWM Duty 数据。
  使用方法：舵机信号接 GPIO4，打开 115200 串口绘图器或串口工具记录 CSV 数据。
  运行结果：舵机在 0～180 度之间往返，串口每 100ms 输出 Time,Angle,Duty。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

int servoAngle = 0;
int angleStep = 5;
unsigned long lastMoveTime = 0;

void setup() {
  Serial.begin(115200);
  ledcAttachChannel(SERVO_PIN, 50, 14, SERVO_CHANNEL);
  setServoAngle(servoAngle);

  Serial.println("Time,Angle,Duty");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMoveTime >= 100) {
    servoAngle = servoAngle + angleStep;

    if (servoAngle >= 180) {
      servoAngle = 180;
      angleStep = -5;
    }

    if (servoAngle <= 0) {
      servoAngle = 0;
      angleStep = 5;
    }

    int duty = setServoAngle(servoAngle);

    Serial.print(currentTime);
    Serial.print(",");
    Serial.print(servoAngle);
    Serial.print(",");
    Serial.println(duty);

    lastMoveTime = currentTime;
  }
}

int setServoAngle(int angle) {
  int duty = map(angle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);
  return duty;
}
