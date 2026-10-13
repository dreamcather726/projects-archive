#include "encoder.h"

// 编码器引脚，保持当前左右轮接线不变
const int encoderA_PINA = 4;
const int encoderB_PINA = 5;
const int encoderA_PINB = 6;
const int encoderB_PINB = 7;

const int WHEEL_TOTAL_PULSE = 430;
const float DISTANCE_PER_PULSE = PI * 6.0f / WHEEL_TOTAL_PULSE;

volatile long countA = 0;
volatile long countB = 0;

float speedA = 0;
float speedB = 0;
float distanceA = 0;
float distanceB = 0;

void IRAM_ATTR encoderA_count()
{
  if (digitalRead(encoderB_PINA) == HIGH) countA++;
  else countA--;
}

  void IRAM_ATTR encoderB_count()
  {
    if (digitalRead(encoderB_PINB) == LOW) countB++;
    else countB--;
  }

void encoder_init()
{
  pinMode(encoderA_PINA, INPUT_PULLUP);
  pinMode(encoderB_PINA, INPUT_PULLUP);
  pinMode(encoderA_PINB, INPUT_PULLUP);
  pinMode(encoderB_PINB, INPUT_PULLUP);

  attachInterrupt(encoderA_PINA, encoderA_count, RISING);
  attachInterrupt(encoderA_PINB, encoderB_count, RISING);
}

void encoder_update(float intervalSeconds)
{
  // 取出本次脉冲后清零，下一次重新计数
  noInterrupts();
  long pulseA = countA;
  long pulseB = countB;
  countA = 0;
  countB = 0;
  interrupts();

  speedA = pulseA * DISTANCE_PER_PULSE / intervalSeconds;
  speedB = pulseB * DISTANCE_PER_PULSE / intervalSeconds;
  distanceA += abs(pulseA) * DISTANCE_PER_PULSE;
  distanceB += abs(pulseB) * DISTANCE_PER_PULSE;
}
