/*
  程序功能：通过串口文字命令控制直流电机的方向、速度和停止。
  使用方法：串口设为 115200 和 Newline，发送 forward 100、reverse 80 或 stop。
  运行结果：电机按命令正转、反转或停止，串口显示当前方向和 0～255 的 PWM 数值。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  analogWrite(MOTOR_PWM_PIN, 0);

  Serial.println("命令：forward 100、reverse 80、stop");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("forward ")) {
      int speedValue = command.substring(8).toInt();
      runMotor(LOW, speedValue);
      Serial.print("FORWARD PWM:");
      Serial.println(constrain(speedValue, 0, 255));
    } else if (command.startsWith("reverse ")) {
      int speedValue = command.substring(8).toInt();
      runMotor(HIGH, speedValue);
      Serial.print("REVERSE PWM:");
      Serial.println(constrain(speedValue, 0, 255));
    } else if (command == "stop") {
      analogWrite(MOTOR_PWM_PIN, 0);
      Serial.println("MOTOR STOP");
    } else {
      Serial.println("格式错误，请输入 forward 100、reverse 80 或 stop");
    }
  }
}

void runMotor(int directionValue, int speedValue) {
  speedValue = constrain(speedValue, 0, 255);

  // 换向前先停止 50ms，减小电流冲击
  analogWrite(MOTOR_PWM_PIN, 0);
  delay(50);
  digitalWrite(MOTOR_DIR_PIN, directionValue);
  analogWrite(MOTOR_PWM_PIN, speedValue);
}
