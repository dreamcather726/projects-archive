#include "motor_driver.h"

const int PWMA_PIN = 1;
const int PWMB_PIN = 42;
const int DIRA_PIN = 2;
const int DIRB_PIN = 41;

void motor_init()
{
  pinMode(PWMA_PIN, OUTPUT);
  pinMode(PWMB_PIN, OUTPUT);
  pinMode(DIRA_PIN, OUTPUT);
  pinMode(DIRB_PIN, OUTPUT);

  // 上电时停止两个电机
  analogWrite(PWMA_PIN, 0);
  analogWrite(PWMB_PIN, 0);
}

void motor_set_pwm(int pwmA, int pwmB)
{
  pwmA = constrain(pwmA, -MOTOR_MAX_PWM, MOTOR_MAX_PWM);
  pwmB = constrain(pwmB, -MOTOR_MAX_PWM, MOTOR_MAX_PWM);

  // 正数正转，负数反转；B电机的安装方向与A电机相反
  digitalWrite(DIRA_PIN, pwmA >= 0 ? HIGH : LOW);
  digitalWrite(DIRB_PIN, pwmB >= 0 ? LOW : HIGH);

  analogWrite(PWMA_PIN, abs(pwmA));
  analogWrite(PWMB_PIN, abs(pwmB));
}
