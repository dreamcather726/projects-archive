#include "pca9685_servo.h"

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

static Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_ADDR);

void initPca9685Servo()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(500);
}

int clampPulseUs(int pulseUs)
{
  if (pulseUs < SERVO_MIN_US) return SERVO_MIN_US;
  if (pulseUs > SERVO_MAX_US) return SERVO_MAX_US;
  return pulseUs;
}

int convertPulseUsToPcaCount(int pulseUs)
{
  pulseUs = clampPulseUs(pulseUs);
  // 50Hz 周期是 20000us，PCA9685 一个周期有 4096 个计数。
  return (int)((long)pulseUs * 4096L / 20000L);
}

void writeServoPulseUs(int channel, int pulseUs)
{
  int pcaCount = convertPulseUsToPcaCount(pulseUs);
  pwm.setPWM(channel, 0, pcaCount);
}
