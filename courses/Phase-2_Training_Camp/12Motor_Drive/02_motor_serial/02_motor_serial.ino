/*
  程序功能：通过串口字符和 ESP32 LEDC PWM 控制直流电机正转、反转和停止。
  使用方法：电机 A 的 AN1/PWMA 接 GPIO 1/42；串口设为 115200，发送 F、B 或 S。
  运行结果：F 正转、B 反转、S 停止，电机速度固定为 100。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int MOTOR_DIR_PIN = 1;
const int MOTOR_PWM_PIN = 42;
const int MOTOR_PWM_FREQUENCY = 20000;
const int MOTOR_PWM_RESOLUTION = 8;
const int MOTOR_PWM_CHANNEL = 0;
const int MOTOR_SPEED = 100;

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_DIR_PIN, OUTPUT);

  ledcAttachChannel(
    MOTOR_PWM_PIN,
    MOTOR_PWM_FREQUENCY,
    MOTOR_PWM_RESOLUTION,
    MOTOR_PWM_CHANNEL
  );
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);

  Serial.println("F:forward B:back S:stop");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'F') {
      runMotor(LOW);
    } else if (command == 'B') {
      runMotor(HIGH);
    } else if (command == 'S') {
      ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
    }
  }
}

void runMotor(int directionValue) {
  // 换向前先停止 50ms，减小电流冲击
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  delay(50);
  digitalWrite(MOTOR_DIR_PIN, directionValue);
  ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
}
