#include "button_control.h"
#include <Arduino.h>

void button_init()
{
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
  pinMode(buttonPin4, INPUT_PULLUP);
}

int button_read(int buttonPin)
{
  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    delay(50); // 消抖延时
    if (digitalRead(buttonPin) == LOW) {
      while (digitalRead(buttonPin) == LOW) {
        delay(1); // 等待按键释放
      }
      return 1; // 按键按下
    }
  }
  return 0; // 按键未按下
}
void button_print()
{
  Serial.printf("Button Pin1 (加速): %d\n", digitalRead(buttonPin1));
  Serial.printf("Button Pin2 (减速): %d\n", digitalRead(buttonPin2));
  Serial.printf("Button Pin3 (停止): %d\n", digitalRead(buttonPin3));
  Serial.printf("Button Pin4 (开始): %d\n", digitalRead(buttonPin4));
}