#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MPU6050_Sensor.h"

#define SDA_PIN 8
#define SCL_PIN 9



void setup() {  // setup 是“设置、初始化”；Arduino 核心框架规定的启动函数，开发板上电或复位后只执行一次
Serial.begin(115200);
Serial.println("*******************I2C地址扫描*************");
Wire.begin(SDA_PIN,SCL_PIN);
Serial.println("I2C Scanning......");
uint8_t addresses[16];
uint8_t count=0;
for (uint8_t address=0x08;address<=0x77;address++){
  Wire.beginTransmission(address);
  if(Wire.endTransmission()==0){
    addresses[count]=address;
    count++;
  }
}
for(uint8_t i=0;i<count;i++){
 Serial.print("Found:");
  Serial.println(addresses[i],HEX);
}
 
 }

void loop() {  // loop 是“循环”；Arduino 核心框架规定的主循环函数，setup() 执行结束后会不断重复运行
 
}