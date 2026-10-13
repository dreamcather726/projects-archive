/*
  程序功能：通过串口和 ESP32 LEDC PWM 控制直流电机，并使用编码器计算累计行驶距离。
  使用方法：电机 A 的 AN1/PWMA 接 GPIO 1/42，编码器 A+/A- 接 GPIO 4/5；
           串口设为 115200，发送 F、B、S 或 R。
  运行结果：电机按命令正转、反转或停止，串口每 200ms 输出编码器计数和距离(cm)。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

const int MOTOR_DIR_PIN = 1;
const int MOTOR_PWM_PIN = 42;
const int MOTOR_PWM_FREQUENCY = 20000;
const int MOTOR_PWM_RESOLUTION = 8;
const int MOTOR_PWM_CHANNEL = 0;
const int MOTOR_SPEED = 100;

const int ENCODER_A_PIN = 4;
const int ENCODER_B_PIN = 5;

// 实测：A 相上升沿每圈计数 430 次
const int COUNTS_PER_REVOLUTION = 430;

// 轮子直径，单位：mm
const float WHEEL_DIAMETER_MM = 60.0;

// 轮子周长，单位：cm
const float WHEEL_CIRCUMFERENCE_CM =
  PI * WHEEL_DIAMETER_MM / 10.0;

// 每次中断对应的行驶距离，单位：cm
const float DISTANCE_PER_COUNT_CM =
  WHEEL_CIRCUMFERENCE_CM / COUNTS_PER_REVOLUTION;

volatile long encoderCount = 0;

// 只检测编码器 A 相上升沿
void IRAM_ATTR encoderRising() {
  int levelB = digitalRead(ENCODER_B_PIN);

  if (levelB == HIGH) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_DIR_PIN, OUTPUT);

  ledcAttachChannel(
    MOTOR_PWM_PIN,
    MOTOR_PWM_FREQUENCY,
    MOTOR_PWM_RESOLUTION,
    MOTOR_PWM_CHANNEL
  );

  // 上电先停止，防止电机突然转动
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);

  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(ENCODER_A_PIN),
    encoderRising,
    RISING
  );

  Serial.println("F:forward B:back S:stop R:reset");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'F') {
      runMotor(LOW);
    } else if (command == 'B') {
      runMotor(HIGH);
    } else if (command == 'S') {
      stopMotor();
    } else if (command == 'R') {
      noInterrupts();
      encoderCount = 0;
      interrupts();
    }
  }

  noInterrupts();
  long currentCount = encoderCount;
  interrupts();

  float distanceCm =
    currentCount * DISTANCE_PER_COUNT_CM;

  Serial.print("Encoder:");
  Serial.print(currentCount);
  Serial.print("  Distance:");
  Serial.print(distanceCm, 2);
  Serial.println(" cm");

  delay(200);
}

void runMotor(int directionValue) {
  // 换向前先停止 50ms，减小电流冲击
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
  delay(50);

  digitalWrite(MOTOR_DIR_PIN, directionValue);
  ledcWriteChannel(MOTOR_PWM_CHANNEL, MOTOR_SPEED);
}

void stopMotor() {
  ledcWriteChannel(MOTOR_PWM_CHANNEL, 0);
}
