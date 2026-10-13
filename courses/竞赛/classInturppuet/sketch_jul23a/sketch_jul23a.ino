#include <Wire.h>
// 电机驱动引脚定义
#define MOTOR_L_PWM 42
#define MOTOR_L_DIR 41
#define MOTOR_R_PWM 1
#define MOTOR_R_DIR 2
#define MAX_SPEED 120
#define BASE_SPEED 40
// 空白直行补偿：左轮天生更快，这里减少左轮速度
#define LEFT_OFFSET 12   // 范围0~20，越大左轮越慢

// --------------- 八路循迹模块参数 ---------------
const uint8_t SDA_PIN = 8;
const uint8_t SCL_PIN = 9;
const uint8_t LINE_SENSOR_ADDRESS = 0x27;
const uint8_t LINE_READ_COMMAND = 0x01;
const uint8_t LINE_DATA_LENGTH = 32;
const uint16_t LINE_THRESHOLD = 200;
const bool HIGH_VALUE_IS_ONE = false;

int trackWeight[8] = {-70, -50, -30, -10, 5, 15, 35, 55};
uint8_t lineStates[8] = {0};
int trackError = 0;

int speedL, speedR;

bool readLineStates(uint8_t lineStates[8]) {
  Wire.beginTransmission(LINE_SENSOR_ADDRESS);
  Wire.write(LINE_READ_COMMAND);
  uint8_t sendResult = Wire.endTransmission();
  if (sendResult != 0) return false;
  delay(10);
  uint8_t receivedBytes = Wire.requestFrom(LINE_SENSOR_ADDRESS, LINE_DATA_LENGTH);
  if (receivedBytes != LINE_DATA_LENGTH) {
    while (Wire.available() > 0) Wire.read();
    return false;
  }
  for (uint8_t i = 0; i < 16; i++) Wire.read();
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t highByte = Wire.read();
    uint8_t lowByte = Wire.read();
    uint16_t rawValue = highByte * 256 + lowByte;
    uint8_t state = 0;
    if (HIGH_VALUE_IS_ONE) {
      if (rawValue >= LINE_THRESHOLD) state = 1;
    } else {
      if (rawValue < LINE_THRESHOLD) state = 1;
    }
    lineStates[i] = state;
  }
  return true;
}

void calcTrackError() {
  trackError = 0;
  for(int i=0; i<8; i++){
    if(lineStates[i]==1) trackError += trackWeight[i];
  }
}

bool isAllWhite()
{
  for(int i = 0; i < 8; i++)
  {
    if(lineStates[i] == 1)
      return false;
  }
  return true;
}

void setMotor(int leftSpd, int rightSpd){
  leftSpd = constrain(leftSpd, 0, MAX_SPEED);
  rightSpd = constrain(rightSpd, 0, MAX_SPEED);
  digitalWrite(MOTOR_L_DIR, 1);
  digitalWrite(MOTOR_R_DIR, 0);
  analogWrite(MOTOR_L_PWM, leftSpd);
  analogWrite(MOTOR_R_PWM, rightSpd);
}

void trackCarRun() {
  readLineStates(lineStates);
  calcTrackError();

  if(isAllWhite())
  {
    // 空白路面：强制左轮减速抵消转速差
    speedL = BASE_SPEED - LEFT_OFFSET;
    speedR = BASE_SPEED;
  }
  else
  {
    int trackAdjust = trackError / 4;
    speedL = BASE_SPEED + trackAdjust;
    speedR = BASE_SPEED - trackAdjust;
  }

  speedL = constrain(speedL, 0, MAX_SPEED);
  speedR = constrain(speedR, 0, MAX_SPEED);

  setMotor(speedL, speedR);

  Serial.print("误差:");Serial.print(trackError);
  Serial.print(" L:");Serial.print(speedL);
  Serial.print(" R:");Serial.println(speedR);
}

void carTurnAround(){
  digitalWrite(MOTOR_L_DIR, 0);
  digitalWrite(MOTOR_R_DIR, 1);
  analogWrite(MOTOR_L_PWM, 30);
  analogWrite(MOTOR_R_PWM, 30);
  delay(600);
}

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_L_PWM, OUTPUT);
  pinMode(MOTOR_L_DIR, OUTPUT);
  pinMode(MOTOR_R_PWM, OUTPUT);
  pinMode(MOTOR_R_DIR, OUTPUT);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  Serial.println("直行补偿加大，空白路面修正跑偏");
}

void loop() {
  trackCarRun();
  delay(50);
}