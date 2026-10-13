/*
  程序功能：读取八路 I2C 巡线模块，用 PID 计算左右轮速度差，实现巡线小车控制。
  使用方法：巡线模块接 GPIO8/9，左右电机按代码引脚接线；抬起车轮确认方向后放到赛道。
  运行结果：小车根据黑线位置连续修正左右轮速度；丢失黑线时停止，串口显示 Error 和速度。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include <Wire.h>
#include "speed_pid.h"

const int MOTOR_L_PWM_PIN = 42;
const int MOTOR_L_DIR_PIN = 1;
const int MOTOR_R_PWM_PIN = 41;
const int MOTOR_R_DIR_PIN = 2;

const int SDA_PIN = 8;
const int SCL_PIN = 9;
const uint8_t LINE_SENSOR_ADDRESS = 0x27;
const uint8_t LINE_READ_COMMAND = 0x01;
const uint8_t LINE_DATA_LENGTH = 32;
const uint16_t LINE_THRESHOLD = 1000;
const bool HIGH_VALUE_IS_LINE = true;

const int BASE_SPEED = 60;
const int MAX_SPEED = 120;

int lineWeight[8] = {-70, -50, -30, -10, 10, 30, 50, 70};
bool lineDetected = false;
unsigned long lastControlTime = 0;

// 保存巡线PID参数、历史误差和输出
PID linePid;

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_L_PWM_PIN, OUTPUT);
  pinMode(MOTOR_L_DIR_PIN, OUTPUT);
  pinMode(MOTOR_R_PWM_PIN, OUTPUT);
  pinMode(MOTOR_R_DIR_PIN, OUTPUT);
  stopMotors();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // 目标误差为 0，表示黑线位于小车中央
  // 巡线修正量允许正负输出，范围为-60～60
  PID_Init(&linePid, 0.8, 0.0, 0.15, 60, -60);
  PID_Clear(&linePid);
  lastControlTime = millis();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastControlTime >= 50) {
    float lineError = readLineError();
    if (lineDetected == false) {
      stopMotors();
      PID_Clear(&linePid);
      Serial.println("LINE LOST");
    } else {
      // 当前值是巡线误差，目标值是0
      float correction = PID_IncPIDCal(&linePid, lineError, 0);
      int leftSpeed = BASE_SPEED - correction;
      int rightSpeed = BASE_SPEED + correction;

      leftSpeed = constrain(leftSpeed, 0, MAX_SPEED);
      rightSpeed = constrain(rightSpeed, 0, MAX_SPEED);
      setMotors(leftSpeed, rightSpeed);

      Serial.print("Error:");
      Serial.print(lineError);
      Serial.print("  Left:");
      Serial.print(leftSpeed);
      Serial.print("  Right:");
      Serial.println(rightSpeed);
    }

    lastControlTime = currentTime;
  }
}

float readLineError() {
  Wire.beginTransmission(LINE_SENSOR_ADDRESS);
  Wire.write(LINE_READ_COMMAND);

  if (Wire.endTransmission() != 0) {
    lineDetected = false;
    return 0;
  }

  delay(10);
  int receivedBytes = Wire.requestFrom(LINE_SENSOR_ADDRESS, LINE_DATA_LENGTH);

  if (receivedBytes != LINE_DATA_LENGTH) {
    while (Wire.available() > 0) {
      Wire.read();
    }

    lineDetected = false;
    return 0;
  }

  // 前16字节为参考值，本实验使用统一阈值，直接跳过
  for (int i = 0; i < 16; i = i + 1) {
    Wire.read();
  }

  long weightSum = 0;
  int lineCount = 0;

  for (int i = 0; i < 8; i = i + 1) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    int sensorValue = highByte * 256 + lowByte;
    bool isLine;

    if (HIGH_VALUE_IS_LINE) {
      isLine = sensorValue >= LINE_THRESHOLD;
    } else {
      isLine = sensorValue < LINE_THRESHOLD;
    }

    if (isLine) {
      weightSum = weightSum + lineWeight[i];
      lineCount = lineCount + 1;
    }
  }

  if (lineCount == 0) {
    lineDetected = false;
    return 0;
  }

  lineDetected = true;
  return (float)weightSum / lineCount;
}

void setMotors(int leftSpeed, int rightSpeed) {
  // 左右电机安装方向相反，因此方向电平不同
  digitalWrite(MOTOR_L_DIR_PIN, HIGH);
  digitalWrite(MOTOR_R_DIR_PIN, LOW);
  analogWrite(MOTOR_L_PWM_PIN, leftSpeed);
  analogWrite(MOTOR_R_PWM_PIN, rightSpeed);
}

void stopMotors() {
  analogWrite(MOTOR_L_PWM_PIN, 0);
  analogWrite(MOTOR_R_PWM_PIN, 0);
}
