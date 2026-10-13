/*
  程序功能：使用编码器位置作为反馈，通过 PID 控制电机转到指定计数位置。
  使用方法：电机和编码器按 GPIO2/1、GPIO6/7 接线；串口发送 position 400、position -400 或 zero。
  运行结果：电机自动选择方向并接近目标计数，串口显示 Target、Position、Error、PWM。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "speed_pid.h"

const int MOTOR_DIR_PIN = 2;
const int MOTOR_PWM_PIN = 1;
const int ENCODER_A_PIN = 6;
const int ENCODER_B_PIN = 7;

volatile long encoderCount = 0;

long targetPosition = 0;
unsigned long lastControlTime = 0;
unsigned long lastPrintTime = 0;

// 保存位置环PID参数、历史误差和输出
PID positionPid;

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
  analogWrite(MOTOR_PWM_PIN, 0);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), countEncoder2x, CHANGE);

  // 位置控制需要正负输出，因此最大值255、最小值-255
  PID_Init(&positionPid, 2.0, 0.0, 0.08, 255, -255);
  PID_Clear(&positionPid);
  lastControlTime = millis();

  Serial.println("命令：position 400、position -400、zero、stop");
}

void loop() {
  readSerialCommand();

  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 20) {
    noInterrupts();
    long currentPosition = encoderCount;
    interrupts();

    float output = PID_IncPIDCal(
      &positionPid,
      currentPosition,
      targetPosition
    );
    float error = targetPosition - currentPosition;

    if (abs(error) <= 2) {
      analogWrite(MOTOR_PWM_PIN, 0);
    } else {
      int pwmValue = abs((int)output);

      // 小 PWM 可能无法克服电机静摩擦
      if (pwmValue > 0 && pwmValue < 40) {
        pwmValue = 40;
      }

      if (output >= 0) {
        digitalWrite(MOTOR_DIR_PIN, LOW);
      } else {
        digitalWrite(MOTOR_DIR_PIN, HIGH);
      }

      analogWrite(MOTOR_PWM_PIN, pwmValue);
    }

    if (currentTime - lastPrintTime >= 100) {
      Serial.print("Target:");
      Serial.print(targetPosition);
      Serial.print(",Position:");
      Serial.print(currentPosition);
      Serial.print(",Error:");
      Serial.print(error);
      Serial.print(",Output:");
      Serial.println(output);
      lastPrintTime = currentTime;
    }

    lastControlTime = currentTime;
  }
}

void readSerialCommand() {
  if (Serial.available() == 0) {
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.startsWith("position ")) {
    targetPosition = command.substring(9).toInt();
    PID_Clear(&positionPid);
  } else if (command == "zero") {
    noInterrupts();
    encoderCount = 0;
    interrupts();
    targetPosition = 0;
    PID_Clear(&positionPid);
  } else if (command == "stop") {
    noInterrupts();
    targetPosition = encoderCount;
    interrupts();
    PID_Clear(&positionPid);
    analogWrite(MOTOR_PWM_PIN, 0);
  }
}
