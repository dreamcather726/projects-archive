#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "WifiControl.h"

Adafruit_PWMServoDriver pwm(0x40);

#define SERVO1 0
#define SERVO2 1
#define SERVO3 2
#define SERVO4 3
#define SERVO5 4
const int count = 5;

#define MIN_PULSE 75
#define MAX_PULSE 510

const char* WIFI_SSID = "REDACTED_CONFIGURE_LOCALLY";
const char* WIFI_PASSWORD = "REDACTED_CONFIGURE_LOCALLY";

int currentAngle[count] = {90, 0, 180, 130, 90};
int targetAngle[count]  = {90, 0, 180, 130, 90};

const int STEP_SIZE = 1;

const int positionA[count] = {90, 0, 180, 130, 90};
const int positionB[count] = {120, 30, 90, 90, 90};

bool stopFlag = false;

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

  startWifiControl(
    WIFI_SSID,
    WIFI_PASSWORD,
    count,
    currentAngle,
    targetAngle,
    positionA,
    positionB,
    &stopFlag
  );
  Serial.println("PCA9685 JXB ready");
}

void loop() {
  handleWifiControlClient();

  if (!stopFlag) {
    for (int i = 0; i < count; i++) {
      smoothMoveServo(i, targetAngle[i], STEP_SIZE);
    }
  }

  delay(10);
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
  } else if (now > tar) {
    now = max(now - stepSize, tar);
  }

  setAngle(ch, now);
  currentAngle[ch] = now;
  return now == tar;
}
