#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// 舵机对应 PCA9685 通道
#define SERVO1 0
#define SERVO2 1
#define SERVO3 2
#define SERVO4 3
const int count = 4;
const int servoInitAngle[count] = {90, 0, 180, 130}; // 舵机初始角度
// 舵机参数
#define MIN_PULSE 75
#define MAX_PULSE 510

void setup() {
  Serial.begin(115200);
  Wire.begin();  
  pwm.begin();
  pwm.setPWMFreq(50);

  delay(500);


  for (int i = 0; i < count; i++)
  {
    /* code */
    setAngle(i, servoInitAngle[i]);
    delay(500);

  }
  

  Serial.println("PCA9685 JXB 已完成初始化");
}

void loop() {
    
}

// 角度转PWM
void setAngle(int ch, int angle) {
  int pulse = map(angle, 0, 180, MIN_PULSE, MAX_PULSE);
  pwm.setPWM(ch, 0, pulse);
}
