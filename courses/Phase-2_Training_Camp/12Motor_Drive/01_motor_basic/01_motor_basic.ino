/*
  程序功能：使用 DIR 和 ESP32 LEDC PWM 控制直流电机正转、停止、反转和调速。
  使用方法：电机 A 的 AN1 接 GPIO 1、PWMA 接 GPIO 42；抬起车轮后上传程序观察。
  运行结果：电机正转 3 秒、停止 1 秒、反转 3 秒、停止 1 秒，并不断重复。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int MOTOR_DIR_PIN = 1;
const int MOTOR_PWM_PIN = 42;
const int MOTOR_PWM_FREQUENCY = 20000;
const int MOTOR_PWM_RESOLUTION = 8;
const int MOTOR_PWM_CHANNEL = 0;
const int MOTOR_SPEED = 100;





void setup() {
  pinMode(MOTOR_DIR_PIN, OUTPUT);

  ledcAttachChannel(
    MOTOR_PWM_PIN,
    MOTOR_PWM_FREQUENCY,
    MOTOR_PWM_RESOLUTION,
    MOTOR_PWM_CHANNEL
  );

  // 上电先停止，避免电机突然转动
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  delay(1000);
}

void loop() {
  // 正转
  digitalWrite(MOTOR_DIR_PIN, LOW);
  ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
  delay(3000);

  // 换向前先停止
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  delay(1000);

  // 反转
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
  delay(3000);

  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  delay(1000);
}
