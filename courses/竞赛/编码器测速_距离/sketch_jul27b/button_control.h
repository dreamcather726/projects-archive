#ifndef BUTTON_CONTROL_H
#define BUTTON_CONTROL_H

const int buttonPin1 = 10; // 加速
const int buttonPin2 = 11; // 减速
const int buttonPin3 = 15; // 停止
const int buttonPin4 = 16; // 开始

void button_init(); // 初始化按键
int button_read(int buttonPin); // 读取指定按键

#endif
