#include <Arduino.h>
#include "lineSensor.h"

void setup() {
  Serial.begin(115200);
  recoverI2CBus();
  Serial.println("八路循迹模块 软件I2C");
  if (sensorProbe()) {
    Serial.println("模块 0x27 在线");
  } else {
    Serial.println("未检测到模块");
  }
}

void loop() {
  uint16_t adc[8] = {0};
  uint8_t state[8] = {0};
  if (readLineSensor(adc, state)) {
    Serial.print("模拟值：");//阈值
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(adc[i]);
      Serial.print("\t");
    }
    Serial.println();
    Serial.print("开关量：");
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(state[i]);
      Serial.print(" ");
    }
    Serial.println("\n----------------");
  } else {
    Serial.println("读取失败，复位总线");
    recoverI2CBus();
  }
  delay(100);
}