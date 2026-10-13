/*
  程序功能：读取八路 I2C 巡线模块，控制两个电机进行黑线循迹。
  接线说明：巡线模块 SDA 接 GPIO8、SCL 接 GPIO9；电机引脚见下方定义。
  安全提示：第一次测试时架空车轮，电机最大占空比限制为 30%。
*/

#include <Wire.h>

// 电机 A 作为左轮：AN1 控制方向，PWMA 控制速度
const int LEFT_PWM_PIN = 42;
const int LEFT_DIR_PIN = 1;

// 电机 B 作为右轮：BN1 控制方向，PWMB 控制速度
const int RIGHT_PWM_PIN = 41;
const int RIGHT_DIR_PIN = 2;

// 八路巡线模块的 I2C 参数
const int SDA_PIN = 8;
const int SCL_PIN = 9;
const uint8_t SENSOR_ADDRESS = 0x27;
const uint8_t READ_COMMAND = 0x01;
const int DATA_LENGTH = 32;

// 大于等于此数值时，认为探头检测到了黑线
// 实际使用前，应根据黑线和白底的测量结果调整
const int LINE_THRESHOLD = 350;

// 8 位 PWM 的最大值是 255，76 约等于 30% 占空比
const int MAX_SPEED = 125;
const int BASE_SPEED = 50;

// 每循环 10 次打印一次，避免串口输出太快
const int PRINT_EVERY_N_LOOPS = 10;

// Kp 越大，转向修正越明显
const float KP = 0.8;

// 八个探头从左到右对应的位置权重
int lineWeight[8] = {-70, -50, -30, -10, 10, 30, 50, 70};
int sensorValues[8];

int lineError = 0;
bool lineFound = false;
bool sensorReadOk = false;
int printCounter = 0;

void setup() {
  Serial.begin(115200);

  // 设置两个电机的控制引脚
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  stopMotors();

  // 启动巡线模块使用的 I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
}

void loop() {
  // 第一步：读取八路探头并计算黑线位置误差
  readLine();

  // 没有检测到黑线，立即停车
  if (lineFound == false) {
    stopMotors();
    printDebugData(0, 0);
    delay(30);
    return;
  }

  // 第二步：根据误差计算左右轮的速度差
  int correction = KP * lineError;
  int leftSpeed = BASE_SPEED - correction;
  int rightSpeed = BASE_SPEED + correction;

  // 第三步：把两侧速度限制在 0～76，保证占空比不超过 30%
  leftSpeed = constrain(leftSpeed, 0, MAX_SPEED);
  rightSpeed = constrain(rightSpeed, 0, MAX_SPEED);

  // 第四步：把计算后的速度输出给左右电机
  setMotors(leftSpeed, rightSpeed);
  printDebugData(leftSpeed, rightSpeed);

  delay(30);
}

void readLine() {
  sensorReadOk = false;

  // 向地址 0x27 的模块发送读取命令 0x01
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(READ_COMMAND);

  if (Wire.endTransmission() != 0) {
    lineFound = false;
    return;
  }

  delay(10);

  // 每次请求完整的 32 字节数据
  int receivedBytes = Wire.requestFrom(SENSOR_ADDRESS, DATA_LENGTH);

  if (receivedBytes != DATA_LENGTH) {
    // 数据不完整时清空缓冲区，避免影响下一次读取
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

  // 后 16 字节组成八路数据，每一路由高、低两个字节组成
  for (int i = 0; i < 8; i = i + 1) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    int sensorValue = highByte * 256 + lowByte;
    sensorValues[i] = sensorValue;

    // 检测到黑线时，累加该探头的位置权重
    if (sensorValue >= LINE_THRESHOLD) {
      weightSum = weightSum + lineWeight[i];
      lineCount = lineCount + 1;
    }
  }

  sensorReadOk = true;

  // 八路都没有检测到黑线，表示丢线
  if (lineCount == 0) {
    lineFound = false;
    return;
  }

  // 多个探头同时检测到黑线时，使用平均权重作为位置误差
  lineError = weightSum / lineCount;
  lineFound = true;
}

void setMotors(int leftSpeed, int rightSpeed) {
  // 两侧电机镜像安装，前进时方向电平相反
  digitalWrite(LEFT_DIR_PIN, HIGH);
  digitalWrite(RIGHT_DIR_PIN, LOW);

  analogWrite(LEFT_PWM_PIN, leftSpeed);
  analogWrite(RIGHT_PWM_PIN, rightSpeed);
}

void stopMotors() {
  analogWrite(LEFT_PWM_PIN, 0);
  analogWrite(RIGHT_PWM_PIN, 0);
}

void printDebugData(int leftSpeed, int rightSpeed) {
  printCounter = printCounter + 1;

  if (printCounter < PRINT_EVERY_N_LOOPS) {
    return;
  }

  printCounter = 0;

  if (sensorReadOk == false) {
    Serial.println("SENSOR READ FAILED");
    return;
  }

  // 打印八路灰度值，用于比较黑线和白底并调整阈值
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
