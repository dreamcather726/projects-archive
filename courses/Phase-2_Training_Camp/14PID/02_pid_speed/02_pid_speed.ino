/*
  程序功能：使用编码器测量电机转速，通过 PID 自动调整 PWM，使转速接近目标 RPM。
  使用方法：电机 DIR/PWM 接 GPIO2/1、编码器 A/B 接 GPIO6/7；串口发送 speed 100 或 stop。
  运行结果：串口显示 Target、RPM、PWM，负载变化后 PID 会调整 PWM 恢复目标转速。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "speed_pid.h"

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;
// 实测单边沿每圈430次，双边沿2倍频后每圈860次
const int SINGLE_EDGE_COUNTS_PER_REVOLUTION = 430;
const int COUNTS_PER_REVOLUTION =
  SINGLE_EDGE_COUNTS_PER_REVOLUTION * 2;

volatile long encoderCount = 0;

float targetRpm = 100;
long lastCount = 0;
unsigned long lastControlTime = 0;

// 保存速度环PID参数、历史误差和输出
PID speedPid;

// 只处理 A 相的上升沿和下降沿，即 2 倍频
void IRAM_ATTR countEncoder2x() {
  int levelA = digitalRead(ENCODER_A_PIN);
  int levelB = digitalRead(ENCODER_B_PIN);

  // 上升沿：读取B相，B为高记作正转
  if (levelA == HIGH) {
    if (levelB == HIGH) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }

  // 下降沿：同一正转方向下，B相应为低电平
  if (levelA == LOW) {
    if (levelB == LOW) {
      encoderCount = encoderCount + 1;
    } else {
      encoderCount = encoderCount - 1;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_PWM_PIN, OUTPUT);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);

  digitalWrite(MOTOR_DIR_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, 0);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder2x, CHANGE);

  // 使用speed_pid.cpp示例中的初始参数
  // 参数顺序：PID、P、I、D、输出最大值、输出最小值
  PID_Init(&speedPid, 0.5, 1.4, 0.2, 255, 0);
  PID_Clear(&speedPid);
  lastControlTime = millis();

  Serial.println("输入 speed 100 修改目标转速，输入 stop 停止");
}

void loop() {
  readSerialCommand();

  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 100) {
    noInterrupts();
    long currentCount = encoderCount;
    interrupts();

    long countChange = currentCount - lastCount;
    unsigned long timeChange = currentTime - lastControlTime;

    float currentRpm = abs(countChange) * 60000.0
                     / COUNTS_PER_REVOLUTION / timeChange;
    float pwmValue = PID_IncPIDCal(&speedPid, currentRpm, targetRpm);

    if (targetRpm <= 0) {
      pwmValue = 0;
    }

    analogWrite(MOTOR_PWM_PIN, (int)pwmValue);

    Serial.print("Target:");
    Serial.print(targetRpm);
    Serial.print(",RPM:");
    Serial.print(currentRpm);
    Serial.print(",PWM:");
    Serial.println(pwmValue);

    lastCount = currentCount;
    lastControlTime = currentTime;
  }
}

void readSerialCommand() {
  if (Serial.available() == 0) {
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.startsWith("speed ")) {
    targetRpm = command.substring(6).toFloat();
    targetRpm = constrain(targetRpm, 0, 300);
    PID_Clear(&speedPid);
  } else if (command == "stop") {
    targetRpm = 0;
    PID_Clear(&speedPid);
    analogWrite(MOTOR_PWM_PIN, 0);
  }
}
