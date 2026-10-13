/*
  程序功能：使用 DIR 和 PWM 引脚控制直流电机正转、停止、反转和调速。
  使用方法：电机驱动 DIR 接 GPIO 2、PWMA 接 GPIO 1；抬起车轮后上传程序观察。
  运行结果：电机正转 3 秒、停止 1 秒、反转 3 秒、停止 1 秒，并不断重复。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;
const int MOTOR_SPEED = 100;

void setup() {
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);

  // 上电先停止，避免电机突然转动
  analogWrite(MOTOR_PWM_PIN, 0);
}

void loop() {
  // 正转
  digitalWrite(MOTOR_DIR_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, MOTOR_SPEED);
  delay(3000);

  // 换向前先停止
  analogWrite(MOTOR_PWM_PIN, 0);
  delay(1000);

  // 反转
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  analogWrite(MOTOR_PWM_PIN, MOTOR_SPEED);
  delay(3000);

  analogWrite(MOTOR_PWM_PIN, 0);
  delay(1000);
}
