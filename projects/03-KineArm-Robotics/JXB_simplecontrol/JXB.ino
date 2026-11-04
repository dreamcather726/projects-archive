#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm(0x40);

#define SERVO1 0
#define SERVO2 1
#define SERVO3 2
#define SERVO4 3
const int count = 4;

#define MIN_PULSE 75
#define MAX_PULSE 510

int currentAngle[count] = {90, 0, 180, 130};
int targetAngle[count]  = {90, 0, 180, 130};

const int STEP_SIZE = 1;

// 测试次数
int testCount = 0;
const int maxTest = 3;
bool stopFlag = false;

// ======================
// 【不用角度判断】独立状态标志
// ======================
// true = 初始位置  false = 目标位置
bool isPositionA = true;  
void setup() {
  Serial.begin(115200);
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  for (int i = 0; i < count; i++) {
    setAngle(i, currentAngle[i]);
    delay(100);
  }
  Serial.println("PCA9685 已准备就绪");
}

void loop() {
  if (!stopFlag) {
    // smoothMoveServo(0, targetAngle[0], STEP_SIZE);
    smoothMoveServo(1, targetAngle[1], STEP_SIZE);
    // smoothMoveServo(2, targetAngle[2], STEP_SIZE);
    // smoothMoveServo(3, targetAngle[3], STEP_SIZE);
  }

  // 2秒切换一次位置
  static unsigned long t = millis();
  if (millis() - t > 2000 && !stopFlag) {
    t = millis();
    
    // ======================
    // 完全根据状态切换，不判断角度
    // ======================
    if (isPositionA) {
      // 去目标位置
      targetAngle[0] = 120;
      targetAngle[1] = 30;
      targetAngle[2] = 90;
      targetAngle[3] = 90;
      if (testCount >= maxTest) {
        stopFlag = true;
        Serial.println("===== 3次测试完成，已停止 =====");
      }
    } else {
      // 回初始位置
      targetAngle[0] = 90;
      targetAngle[1] = 0;
      targetAngle[2] = 180;
      targetAngle[3] = 130;

      // 完成一次来回
      testCount++;
      Serial.print("已完成测试：");
      Serial.println(testCount);

      // 到达3次停止
      
    }

    // 切换状态
    isPositionA = !isPositionA;
  }
}

void setAngle(int ch, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, MIN_PULSE, MAX_PULSE);
  pwm.setPWM(ch, 0, pulse);
}

bool smoothMoveServo(int ch, int tar, int stepSize) {
  int now = currentAngle[ch];

  if (now < tar) {
    now = min(now + stepSize, tar);
  } 
  else if (now > tar) {
    now = max(now - stepSize, tar);
  }

  setAngle(ch, now);
  currentAngle[ch] = now;
  return (now == tar);
}