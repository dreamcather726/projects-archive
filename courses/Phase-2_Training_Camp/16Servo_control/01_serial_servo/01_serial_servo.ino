/*
  程序功能：使用 ESP32 LEDC 产生 50Hz 舵机信号，通过串口命令设置 0～180°目标角度。
  使用方法：舵机信号接 GPIO4，串口设为 115200 和 Newline，发送 g0、g90 或 g180。
  运行结果：舵机转到指定目标角度，串口显示 Angle 和对应的 14 位 Duty 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#define SERVO_PIN 4
#define SERVO_CHANNEL 0

const int SERVO_FREQUENCY = 50;
const int SERVO_RESOLUTION = 14;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  ledcAttachChannel(
    SERVO_PIN,
    SERVO_FREQUENCY,
    SERVO_RESOLUTION,
    SERVO_CHANNEL
  );

  setServoAngle(90);
  Serial.println("命令：g0、g90、g180");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("g")) {
      int angle = command.substring(1).toInt();
      setServoAngle(angle);
    } else {
      Serial.println("格式错误，请输入 g0～g180");
    }
  }
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  int duty = map(angle, 0, 180, 410, 2048);
  ledcWriteChannel(SERVO_CHANNEL, duty);

  Serial.print("Angle:");
  Serial.print(angle);
  Serial.print("  Duty:");
  Serial.println(duty);
}
