/*
  程序功能：使用 Ticker 每 30ms 触发一次八路灰度循迹控制。
  使用方法：电机 A 作为左轮，电机 B 作为右轮；巡线模块接 GPIO8/9。
  安全提示：定时回调只设置任务标志，I2C、串口和电机操作放在 loop() 中执行。
*/

#include <Wire.h>
#include <Ticker.h>

// 电机 A：左轮
#define LEFT_PWM_PIN 42
#define LEFT_DIR_PIN 1
#define LEFT_CHANNEL 0

// 电机 B：右轮
#define RIGHT_PWM_PIN 41
#define RIGHT_DIR_PIN 2
#define RIGHT_CHANNEL 1

// 八路巡线模块
#define SDA_PIN 8
#define SCL_PIN 9
#define SENSOR_ADDRESS_LINE 0x27

const uint8_t READ_COMMAND = 0x01;
const int DATA_LENGTH = 32;

// 实测白底约 84～91、黑线约 479～539
const int LINE_THRESHOLD = 120;

// 8 位 PWM 最大为 255，76 约等于 30% 占空比
const int MAX_SPEED = 125;
const int BASE_SPEED = 50;

const float KP = 0.8;
const int CONTROL_INTERVAL_MS = 30;
const int PRINT_TIMES = 10;

int lineWeight[8] = {-70, -50, -30, -10, 10, 30, 50, 70};
int sensorValues[8];

int lineError = 0;
bool lineFound = false;
bool sensorReadOk = false;
int printCounter = 0;

// Ticker 回调和 loop() 都会访问此变量，所以使用 volatile
volatile bool controlTaskReady = false;
Ticker controlTicker;

void onControlTimer() {
  // 回调中只置位，不直接读取 I2C、控制电机或打印串口
  controlTaskReady = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);

  // 两路电机分别使用独立的 LEDC 通道
  ledcAttachChannel(LEFT_PWM_PIN, 10000, 8, LEFT_CHANNEL);
  ledcAttachChannel(RIGHT_PWM_PIN, 10000, 8, RIGHT_CHANNEL);

  digitalWrite(LEFT_DIR_PIN, HIGH);
  digitalWrite(RIGHT_DIR_PIN, LOW);
  stopMotors();

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // 每 30ms 调用一次回调，通知主循环执行循迹任务
  controlTicker.attach_ms(CONTROL_INTERVAL_MS, onControlTimer);
}

void loop() {
  if (controlTaskReady) {
    controlTaskReady = false;
    runLineControl();
  }
}

void runLineControl() {
  readLine();

  if (lineFound == false) {
    stopMotors();
    printDebugData(0, 0);
    return;
  }

  int correction = KP * lineError;
  int leftSpeed = BASE_SPEED - correction;
  int rightSpeed = BASE_SPEED + correction;

  // 两侧都必须限幅，保证最大占空比不超过约 30%
  leftSpeed = constrain(leftSpeed, 0, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, 0, MAX_SPEED);

  setMotors(leftSpeed, rightSpeed);
  printDebugData(leftSpeed, rightSpeed);
}

void readLine() {
  sensorReadOk = false;

  Wire.beginTransmission(SENSOR_ADDRESS_LINE);
  Wire.write(READ_COMMAND);

  if (Wire.endTransmission() != 0) {
    lineFound = false;
    return;
  }

  delay(10);
  int receivedBytes = Wire.requestFrom(SENSOR_ADDRESS_LINE, DATA_LENGTH);

  if (receivedBytes != DATA_LENGTH) {
    while (Wire.available() > 0) {
      Wire.read();
    }

    lineFound = false;
    return;
  }

  // 按模块协议跳过前 16 字节
  for (int i = 0; i < 16; i = i + 1) {
    Wire.read();
  }

  int weightSum = 0;
  int lineCount = 0;

  // 后 16 字节组成八路灰度，每一路由高、低两个字节组成
  for (int i = 0; i < 8; i = i + 1) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    int sensorValue = highByte * 256 + lowByte;
    sensorValues[i] = sensorValue;

    // 本车实测黑线数值大于白底数值
    if (sensorValue >= LINE_THRESHOLD) {
      weightSum = weightSum + lineWeight[i];
      lineCount = lineCount + 1;
    }
  }

  sensorReadOk = true;

  if (lineCount == 0) {
    lineFound = false;
    return;
  }

  lineError = weightSum / lineCount;
  lineFound = true;
}

void setMotors(int leftSpeed, int rightSpeed) {
  digitalWrite(LEFT_DIR_PIN, HIGH);
  digitalWrite(RIGHT_DIR_PIN, LOW);
  ledcWriteChannel(LEFT_CHANNEL, leftSpeed);
  ledcWriteChannel(RIGHT_CHANNEL, rightSpeed);
}

void stopMotors() {
  ledcWriteChannel(LEFT_CHANNEL, 0);
  ledcWriteChannel(RIGHT_CHANNEL, 0);
}

void printDebugData(int leftSpeed, int rightSpeed) {
  printCounter = printCounter + 1;

  if (printCounter < PRINT_TIMES) {
    return;
  }

  printCounter = 0;

  if (sensorReadOk == false) {
    Serial.println("SENSOR READ FAILED");
    return;
  }

  for (int i = 0; i < 8; i = i + 1) {
    Serial.print("S");
    Serial.print(i + 1);
    Serial.print(":");
    Serial.print(sensorValues[i]);
    Serial.print("  ");
  }

  Serial.print("Threshold:");
  Serial.print(LINE_THRESHOLD);
  Serial.print("  Error:");

  if (lineFound) {
    Serial.print(lineError);
  } else {
    Serial.print("LOST");
  }

  Serial.print("  Left:");
  Serial.print(leftSpeed);
  Serial.print("  Right:");
  Serial.println(rightSpeed);
}
